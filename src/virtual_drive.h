#ifndef VIRTUAL_DRIVE_H
#define VIRTUAL_DRIVE_H

#include <vector>
#include <map>
#include <cstring>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "disk_interface.h"
#include "utils/except.h"
#include "utils/socket.h"
#include "utils/misc.h"

namespace cs2313 {

    enum drive_scheduler {
        DRIVE_SCHEDULER_SSTF,
        DRIVE_SCHEDULER_SCAN,
        DRIVE_SCHEDULER_CSCAN,
        DRIVE_SCHEDULER_LOOK,
        DRIVE_SCHEDULER_CLOOK
    };

    struct drive_sector_transaction {
        uint64_t sector_offset_;
        char *data_;
        uint32_t tid_;
        io_instr instr_;

        drive_sector_transaction(const io_instr instr, const uint32_t tid, const uint64_t sector_offset, const size_t data_size):
            sector_offset_(sector_offset),
            data_(new char[data_size]),
            tid_(tid),
            instr_(instr) {}

        ~drive_sector_transaction() { delete[] data_; }

        drive_sector_transaction(drive_sector_transaction &&other) noexcept:
            sector_offset_(other.sector_offset_),
            data_(other.data_),
            tid_(other.tid_),
            instr_(other.instr_) {
            other.data_ = nullptr;
        }

        drive_sector_transaction &operator=(drive_sector_transaction &&other) noexcept {
            if (this != &other) {
                delete[] data_;
                sector_offset_ = other.sector_offset_;
                data_ = other.data_;
                tid_ = other.tid_;
                instr_ = other.instr_;
                other.data_ = nullptr;
            }
            return *this;
        }

        drive_sector_transaction(const drive_sector_transaction &) = delete;

        drive_sector_transaction &operator=(const drive_sector_transaction &) = delete;
    };

    class virtual_drive {
    public:
        virtual_drive(const uint64_t cylinders,
                      const uint64_t sectors_per_cylinder,
                      const uint64_t bytes_per_sector,
                      //const drive_scheduler scheduler,
                      const uint64_t sim_move_cost_us,
                      const char *path,
                      const uint16_t bind_port) :
            cylinders_(cylinders),
            sectors_per_cylinder_(sectors_per_cylinder),
            bytes_per_sector_(bytes_per_sector),
            scheduler_(DRIVE_SCHEDULER_SSTF), // unused yet
            sim_move_cost_us_(sim_move_cost_us),
            server_socket_(bind_port),
            receiver_loop_(false)
        // magnetic_head_sig_continue_(false),
        // magnetic_head_sig_term_(false)
        {

            if (power_of_2(bytes_per_sector_) == -1)
                throw except(ERROR_VIRTUAL_DRIVE_INVALID_ARGS, "Invalid virtual drive arguments: sector size not power-of-2 aligned");

            if (power_of_2(sectors_per_cylinder_) == -1)
                throw except(ERROR_VIRTUAL_DRIVE_INVALID_ARGS, "Invalid virtual drive arguments: cylinder size not power-of-2 aligned");

            disk_size_ = bytes_per_sector_ * sectors_per_cylinder_ * cylinders_;
            if (disk_size_ / bytes_per_sector_ / sectors_per_cylinder_ != cylinders_)
                throw except(ERROR_VIRTUAL_DRIVE_INVALID_ARGS, "Invalid virtual drive arguments: size limit exceeded (16EiB)");

            // The program runs on a Linux environment (usually with ext4 FS)
            // so the size of the drive should be no more than 16TiB

            // if(disk_size >= (1LL << 44))
            //     throw except(ERROR_VIRTUAL_DRIVE_INVALID_ARGS, "Invalid virtual drive arguments: size limit exceeded (16TiB)");

            addr_size_ = sectors_per_cylinder_ * cylinders_;
            sector_addr_bitmask_ = sectors_per_cylinder_ - 1;
            sector_addr_bits_ = power_of_2(sectors_per_cylinder_);

            file_fd_ = open(path, O_RDWR | O_CREAT, 0666);
            if (file_fd_ < 0) {
                throw except(errno, ERROR_VIRTUAL_DRIVE_FILE_CREATE, "Failed to create virtual drive file");
            }

            if (lseek(file_fd_, disk_size_ - 1, SEEK_SET) == -1) {
                close(file_fd_);
                throw except(errno, ERROR_VIRTUAL_DRIVE_FILE_CREATE, "Failed to create virtual drive file");
            }

            char char_empty = 0;
            if (write(file_fd_, &char_empty, 1) != 1) {
                close(file_fd_);
                throw except(errno, ERROR_VIRTUAL_DRIVE_FILE_CREATE, "Failed to create virtual drive file");
            }

            file_data_ = static_cast<char *>(mmap(nullptr, disk_size_, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd_, 0));

            if (file_data_ == MAP_FAILED) {
                close(file_fd_);
                throw except(errno, ERROR_VIRTUAL_DRIVE_MMAP, "Failed to map virtual drive file to memory");
            }

            // todo paging & lazy expansion
        }

        void start() {
            if (!receiver_loop_.load()) {
                receiver_loop_.store(true);
                receiver_thread_ = std::thread(&virtual_drive::request_receiver, this); // TODO except
            }
        }

        void wait() {
            if (receiver_thread_.joinable())
                receiver_thread_.join();
        }

        ~virtual_drive() {
            if (receiver_loop_.load()) {
                receiver_loop_.store(false);
                if (receiver_thread_.joinable())
                    receiver_thread_.join();
            }
            munmap(file_data_, disk_size_);
            close(file_fd_);
        }

    private:
        uint64_t cylinders_, sectors_per_cylinder_, bytes_per_sector_;
        uint64_t disk_size_, addr_size_;

        uint32_t sector_addr_bits_;
        uint64_t sector_addr_bitmask_;
        uint64_t cylinder_no(const uint64_t addr) const { return addr >> sector_addr_bits_; }
        uint64_t sector_no(const uint64_t addr) const { return addr & sector_addr_bitmask_; }

        drive_scheduler scheduler_;
        uint64_t sim_move_cost_us_;

        int file_fd_;
        char *file_data_;

        server_socket_handle server_socket_;
        server_connection_socket_handle connection_socket_;
        std::mutex socket_write_mutex_;

        std::atomic<bool> receiver_loop_;
        std::thread receiver_thread_;

        // std::mutex magnetic_head_wake_mutex_;
        // std::condition_variable magnetic_head_wake_cv_;
        // bool magnetic_head_sig_continue_;
        // bool magnetic_head_sig_term_;
        // std::thread magnetic_head_thread_;

        // std::map<uint64_t, std::vector<drive_sector_transaction> > waiting_list_;
        // std::mutex list_mutex_;

        void request_receiver() {

            uint8_t instr;
            uint32_t tid;
            uint64_t addr;

            uint64_t prev_cylinder_no = 0;

            auto dist = [](const uint64_t a, const uint64_t b)-> uint64_t {
                if (a > b)
                    return a - b;
                return b - a;
            };

            while (receiver_loop_.load(std::memory_order_acquire)) {
                try {
                    connection_socket_ = server_socket_.accept();
                } catch (except &e) {
                    if (e.error_code() == ERROR_SOCKET_TERMINATED) {
                        receiver_loop_.store(false);
                        return;
                    }
                    throw;
                }

                while (receiver_loop_.load(std::memory_order_acquire)) {
                    try {
                        connection_socket_.recv(instr);
                        connection_socket_.recv(tid);
                        switch (instr) {
                            case IO_INSTR_READ: {
                                connection_socket_.recv(addr);
                                // drive_sector_transaction transaction(instr, tid, sector_no(addr), bytes_per_sector_); //
                                // {
                                //     std::lock_guard<std::mutex> lock(list_mutex_);
                                //     waiting_list_[cylinder_no(addr)].emplace_back(std::move(transaction));
                                // } todo async
                                if (prev_cylinder_no != cylinder_no(addr))
                                    usleep(dist(prev_cylinder_no, cylinder_no(addr)) * sim_move_cost_us_);
                                prev_cylinder_no = cylinder_no(addr); //
                                {
                                    std::lock_guard<std::mutex> lock(socket_write_mutex_);
                                    connection_socket_.send(instr);
                                    connection_socket_.send(tid);
                                    connection_socket_.send_raw(file_data_ + addr * bytes_per_sector_, bytes_per_sector_);
                                }
                                // todo addr check
                            }
                            break;
                            case IO_INSTR_WRITE: {
                                connection_socket_.recv(addr);
                                // drive_sector_transaction transaction(instr, tid, sector_no(addr), bytes_per_sector_);
                                // connection_socket_.recv_raw(transaction.data_, bytes_per_sector_); //
                                // {
                                //     std::lock_guard<std::mutex> lock(list_mutex_);
                                //     waiting_list_[cylinder_no(addr)].emplace_back(std::move(transaction));
                                // } todo async
                                if (prev_cylinder_no != cylinder_no(addr))
                                    usleep(dist(prev_cylinder_no, cylinder_no(addr)) * sim_move_cost_us_);
                                connection_socket_.recv_raw(file_data_ + addr * bytes_per_sector_, bytes_per_sector_);
                                prev_cylinder_no = cylinder_no(addr); //
                                {
                                    std::lock_guard<std::mutex> lock(socket_write_mutex_);
                                    connection_socket_.send(instr);
                                    connection_socket_.send(tid);
                                }
                            }
                            break;
                            case IO_INSTR_GET_DESC: {
                                std::lock_guard<std::mutex> lock(socket_write_mutex_);
                                connection_socket_.send(instr);
                                connection_socket_.send(tid);
                                connection_socket_.send(disk_description{cylinders_, sectors_per_cylinder_, bytes_per_sector_});
                            }
                            break;
                            case IO_INSTR_SHUTDOWN: {
                                // todo async
                                receiver_loop_.store(false); {
                                    std::lock_guard<std::mutex> lock(socket_write_mutex_);
                                    connection_socket_.send(instr);
                                    connection_socket_.send(tid);
                                }
                            }
                            break;
                            default:
                                break; // todo throw
                        }
                    } catch (except &e) {
                        if (e.error_code() == ERROR_SOCKET_CLOSED_BY_REMOTE) {
                            // std::lock_guard<std::mutex> lock(list_mutex_);
                            // waiting_list_.clear();
                            break;
                        }
                        throw;
                    }
                }
            }
        }

        // todo async scheduler

        // void virtual_magnetic_head() {
        //     uint64_t cylinder_pos = 0;
        //     enum { MOVE_UP, MOVE_DOWN } current_direction = MOVE_UP; // for SCAN and LOOK only
        //     while (true) {
        //         {
        //             std::unique_lock<std::mutex> lock(magnetic_head_wake_mutex_);
        //             if (magnetic_head_sig_term_)
        //                 break;
        //             magnetic_head_sig_continue_ = false;
        //             bool idle_flag; //
        //             {
        //                 std::lock_guard<std::mutex> list_lock(list_mutex_);
        //                 idle_flag = waiting_list_.empty();
        //             }
        //             if (idle_flag)
        //                 magnetic_head_wake_cv_.wait(lock, [this] { return magnetic_head_sig_continue_ || magnetic_head_sig_term_; });
        //             if (magnetic_head_sig_term_)
        //                 break;
        //         }
        //
        //         std::vector<drive_sector_transaction> transaction_list;
        //         uint64_t move_dist = 0; //
        //         {
        //             std::lock_guard<std::mutex> list_lock(list_mutex_);
        //             if (waiting_list_.empty())
        //                 continue;
        //             auto it = waiting_list_.lower_bound(cylinder_pos);
        //             if (it != waiting_list_.end() && it->first == cylinder_pos) {
        //                 move_dist = 0;
        //             } else {
        //                 switch (scheduler_) {
        //                     case DRIVE_SCHEDULER_SSTF:
        //                         if (it == waiting_list_.begin()) {
        //                             move_dist = it->first - cylinder_pos;
        //                         } else if (it == waiting_list_.end()) {
        //                             it = std::prev(it);
        //                             move_dist = cylinder_pos - it->first;
        //                         } else {
        //                             auto prev = std::prev(it);
        //                             if (cylinder_pos - prev->first < it->first - cylinder_pos) {
        //                                 it = prev;
        //                                 move_dist = cylinder_pos - it->first;
        //                             } else {
        //                                 move_dist = it->first - cylinder_pos;
        //                             }
        //                         }
        //                         break;
        //                     case DRIVE_SCHEDULER_SCAN:
        //                     case DRIVE_SCHEDULER_LOOK:
        //                         if (current_direction == MOVE_UP) {
        //                             if (it == waiting_list_.end()) {
        //                                 current_direction = MOVE_DOWN;
        //                                 it = std::prev(it);
        //                                 if (scheduler_ == DRIVE_SCHEDULER_SCAN)
        //                                     move_dist = cylinders_ * 2LL - 2LL - cylinder_pos - it->first;
        //                                 else
        //                                     move_dist = cylinder_pos - it->first;
        //                             } else {
        //                                 move_dist = it->first - cylinder_pos;
        //                             }
        //                         } else { // MOVE_DOWN
        //                             if (it == waiting_list_.begin()) {
        //                                 current_direction = MOVE_UP;
        //                                 if (scheduler_ == DRIVE_SCHEDULER_SCAN)
        //                                     move_dist = cylinder_pos + it->first;
        //                                 else
        //                                     move_dist = it->first - cylinder_pos;
        //                             } else {
        //                                 it = std::prev(it);
        //                                 move_dist = cylinder_pos - it->first;
        //                             }
        //                         }
        //                         break;
        //                     case DRIVE_SCHEDULER_CSCAN:
        //                     case DRIVE_SCHEDULER_CLOOK:
        //                         if (it == waiting_list_.end()) {
        //                             it = waiting_list_.begin();
        //                             if (scheduler_ == DRIVE_SCHEDULER_CSCAN)
        //                                 move_dist = cylinders_ * 2LL - 2LL - cylinder_pos + it->first;
        //                             else
        //                                 move_dist = cylinder_pos - it->first;
        //                         } else {
        //                             move_dist = it->first - cylinder_pos;
        //                         }
        //                         break;
        //                 }
        //             }
        //             cylinder_pos = it->first;
        //             transaction_list = std::move(it->second);
        //             waiting_list_.erase(it);
        //         }
        //         if (move_dist)
        //             usleep(move_dist * sim_move_cost_us_);
        //
        //         // The connection socket is initialized when transactions can be received
        //
        //         try {
        //             for (auto &t: transaction_list) { // todo thread pool
        //                 switch (t.instr_) {
        //                     case IO_INSTR_READ:
        //                         uint64_t offset = ((cylinder_pos << sector_addr_bits_) | t.sector_offset_) * bytes_per_sector_;
        //                         memcpy(t.data_, file_data_ + offset, bytes_per_sector_); // todo will be used. don't optimize
        //                         {
        //                             std::lock_guard<std::mutex> socket_lock(socket_write_mutex_);
        //
        //                         }
        //                         break;
        //                     case IO_INSTR_WRITE:
        //                         break;
        //                     default:
        //                         break;
        //                 }
        //             }
        //         } catch (except &e) {
        //             if (e.error_code() == ERROR_SOCKET_CLOSED_BY_REMOTE) {
        //                 std::lock_guard<std::mutex> lock(list_mutex_);
        //                 waiting_list_.clear();
        //                 continue;
        //             }
        //             if (e.error_code() == ERROR_SOCKET_TERMINATED) {
        //                 std::lock_guard<std::mutex> lock(list_mutex_);
        //                 waiting_list_.clear();
        //                 break;
        //             }
        //             throw;
        //         }
        //
        //
        //     }
        //
        // }
    };
}

#endif
