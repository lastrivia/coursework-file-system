#ifndef IO_PROTOCOL_H
#define IO_PROTOCOL_H

#include <thread>
#include <atomic>
#include <mutex>
#include <semaphore>
#include <unordered_map>

#include "disk_interface.h"
#include "utils/socket.h"

namespace cs2313 {

    struct client_transaction {
        std::binary_semaphore sig_wake_;
        char *writeback_data_;

        explicit client_transaction(char *writeback_data = nullptr) :
            sig_wake_(0),
            writeback_data_(writeback_data) {}

        client_transaction(const client_transaction &) = delete;

        client_transaction &operator=(const client_transaction &) = delete;
    };

    class disk_storage_client : public disk_interface {

    public:
        disk_storage_client(const std::string &server_addr, const uint16_t server_port) :
            tid_counter_(0),
            client_socket_(server_addr, server_port),
            handler_loop_(false),
            initiative_shutdown_(false) {

            client_socket_.send(IO_INSTR_GET_DESC);
            client_socket_.send(tid_counter_);
            ++tid_counter_;

            char discard[sizeof(io_instr) + sizeof(tid_counter_)];
            client_socket_.recv_raw(discard, sizeof(io_instr) + sizeof(tid_counter_));
            client_socket_.recv(description_);
        }

        void start_handler() {
            if (!handler_loop_.load()) {
                handler_loop_.store(true);
                handler_thread_ = std::thread(&disk_storage_client::response_handler, this); // TODO except
            }
        }

        // TODO timeout & heartbeat

        void read(const uint64_t sector_addr, char *data) override {
            uint32_t tid = tid_step();
            client_transaction transaction(data);
            waiting_list_add(tid, &transaction); //
            {
                std::lock_guard<std::mutex> lock(socket_write_mutex_);
                client_socket_.send(IO_INSTR_READ);
                client_socket_.send(tid);
                client_socket_.send(sector_addr);
            }
            transaction.sig_wake_.acquire();
        }

        void write(const uint64_t sector_addr, const char *data) override {
            uint32_t tid = tid_step();
            client_transaction transaction;
            waiting_list_add(tid, &transaction); //
            {
                std::lock_guard<std::mutex> lock(socket_write_mutex_);
                client_socket_.send(IO_INSTR_WRITE);
                client_socket_.send(tid);
                client_socket_.send(sector_addr);
                client_socket_.send_raw(data, description_.bytes_per_sector);
            }
            transaction.sig_wake_.acquire();
        }

        void shutdown() override {
            uint32_t tid = tid_step();
            client_transaction transaction;
            waiting_list_add(tid, &transaction); //
            {
                std::lock_guard<std::mutex> lock(socket_write_mutex_);
                initiative_shutdown_.store(true);
                client_socket_.send(IO_INSTR_SHUTDOWN);
                client_socket_.send(tid);
            }
            transaction.sig_wake_.acquire();
        }

        disk_description get_description() override {
            return description_;
        }

        ~disk_storage_client() override {
            if (handler_loop_.load()) {
                handler_loop_.store(false);
                if (handler_thread_.joinable())
                    handler_thread_.join();
            }
        }

    private:
        uint32_t tid_counter_;
        std::mutex counter_mutex_;

        uint32_t tid_step() {
            std::lock_guard<std::mutex> lock(counter_mutex_);
            uint32_t tid = tid_counter_;
            ++tid_counter_;
            return tid;
        }

        std::unordered_map<int, client_transaction *> waiting_list_;
        std::mutex list_mutex_;

        void waiting_list_add(const uint32_t tid, client_transaction *transaction) {
            std::lock_guard<std::mutex> lock(list_mutex_);
            waiting_list_[tid] = transaction;
        }

        client_socket_handle client_socket_;
        std::mutex socket_write_mutex_;

        disk_description description_;

        std::atomic<bool> handler_loop_;
        std::thread handler_thread_;

        std::atomic<bool> initiative_shutdown_;

        void response_handler() {
            while (handler_loop_.load(std::memory_order_acquire)) {
                try {
                    uint8_t instr;
                    uint32_t tid;
                    client_socket_.recv(instr);
                    client_socket_.recv(tid);
                    client_transaction *transaction = nullptr; //

                    {
                        std::lock_guard<std::mutex> lock(list_mutex_);
                        auto it = waiting_list_.find(tid);
                        if (it != waiting_list_.end()) {
                            transaction = it->second;
                            waiting_list_.erase(it);
                        }
                    }

                    if (transaction) {
                        if (instr == IO_INSTR_READ)
                            client_socket_.recv_raw(transaction->writeback_data_, description_.bytes_per_sector);
                        transaction->sig_wake_.release();
                    }
                } catch (except &e) {
                    if (e.error_code() == ERROR_SOCKET_CLOSED_BY_REMOTE) {
                        if (initiative_shutdown_.load(std::memory_order_acquire))
                            return;
                        // todo warn disconnection
                        return;
                    }
                    throw;
                }
            }
        }
    };
}

#endif
