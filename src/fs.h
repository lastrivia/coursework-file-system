#ifndef FS_H
#define FS_H

#include <cstdint>
#include <cstring>
#include <mutex>

#include "disk_view.h"
#include "fs_allocator.h"
#include "utils/except.h"

namespace cs2313 {

    class fs_handle;

    class fs_file_handle;

    class fs_folder_handle;

    class file_system {
    public:
        file_system(disk_interface &disk) :
            disk_(disk),
            allocator_(disk_, ALLOC_ROOT),
            description_(disk.get_description()),
            max_blocks_(description_.cylinders * description_.sectors_per_cylinder) {

            directory_node file_root = disk_[FILE_ROOT];
            allocator_node alloc_root = disk_[ALLOC_ROOT];
            if (file_root.magic == 0x0909 && alloc_root.header.magic == 0x0909)
                formatted_ = true;
            else formatted_ = false;
        }

        void format() {
            // todo init checksum
            std::lock_guard<std::mutex> lock(data_mutex_);
            disk_[FILE_ROOT] = directory_node::default_folder();
            disk_[ALLOC_ROOT] = allocator_node::default_root({RESERVED_BLOCKS, max_blocks_ - RESERVED_BLOCKS});
            formatted_ = true;
        }

        bool formatted() { return formatted_; }

        fs_folder_handle root_folder();

    private:
        friend class fs_handle;
        friend class fs_file_handle;
        friend class fs_folder_handle;

        static constexpr uint64_t FILE_ROOT = 0, ALLOC_ROOT = 1, RESERVED_BLOCKS = 2;
        disk_view disk_;
        disk_description description_;
        uint64_t max_blocks_;
        fs_allocator allocator_;
        bool formatted_;
        std::mutex data_mutex_; // todo segmented mutex

        std::unordered_map<uint64_t, uint32_t> handle_instance_count_;
        std::mutex count_mutex_;

        void handle_construction(uint64_t addr) {
            std::lock_guard<std::mutex> lock(count_mutex_);
            auto it = handle_instance_count_.find(addr);
            if (it != handle_instance_count_.end())
                ++it->second;
            else
                handle_instance_count_[addr] = 1;
        }

        void handle_destruction(uint64_t addr) {
            std::lock_guard<std::mutex> lock(count_mutex_);
            auto it = handle_instance_count_.find(addr);
            if (it != handle_instance_count_.end()) {
                if (it->second == 1)
                    handle_instance_count_.erase(it);
                else
                    --it->second;
            }
        }

        static bool is_name_valid(const std::string &name) {
            return !name.empty()
                   && name != "."
                   && name != ".."
                   && name.find('/') == std::string::npos;
        }
    };

    class fs_handle {
    public:
        fs_handle(file_system &fs, uint64_t addr, bool abandoned = false) :
            fs_(fs), addr_(addr), abandoned_(abandoned) {
            if (!abandoned_)
                fs_.handle_construction(addr_);
        }

        virtual ~fs_handle() {
            if (!abandoned_)
                fs_.handle_destruction(addr_);
        }

        fs_handle(const fs_handle &other) : fs_handle(other.fs_, other.addr_, other.abandoned_) {}

        fs_handle &operator=(const fs_handle &other) {
            if (&other != this) {
                // if (&other.fs_ != &fs_)
                //     throw except(ERROR_FS_HANDLE_INVALID);
                if (!abandoned_)
                    fs_.handle_destruction(addr_);
                addr_ = other.addr_;
                abandoned_ = other.abandoned_;
                if (!abandoned_)
                    fs_.handle_construction(addr_);
            }
            return *this;
        }

        fs_handle(fs_handle &&other) noexcept : fs_handle(other.fs_, other.addr_, true) {
            if (!other.abandoned_) {
                abandoned_ = false;
                other.abandoned_ = true;
            }
        }

        fs_handle &operator=(fs_handle &&other) noexcept {
            if (!abandoned_)
                fs_.handle_destruction(addr_);
            addr_ = other.addr_;
            abandoned_ = other.abandoned_;
            other.abandoned_ = true;
            return *this;
        }

    protected:
        file_system &fs_;
        uint64_t addr_;
        bool abandoned_;
    };

    class fs_file_handle : public fs_handle {
    public:
        fs_file_handle(file_system &fs, uint64_t addr) : fs_handle(fs, addr) {} // todo validate

        fs_file_handle(const fs_file_handle &) = default;

        fs_file_handle &operator=(const fs_file_handle &) = default;

        fs_file_handle(fs_file_handle &&) = default;

        fs_file_handle &operator=(fs_file_handle &&) = default;

        std::string read_all() {
            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            if (!node.size_blocks)
                return "";
            std::string ret;
            ret.resize((node.size_blocks - 1) * 0x100 + node.size_offset);
            // assert size_blocks <= header.entries
            // todo len > 1
            for (uint32_t i = 0; i < node.header.entries - 1; ++i)
                fs_.disk_[node.extent(i).disk_addr].read_raw(ret.data() + i * 0x100);
            char buf[256];
            fs_.disk_[node.extent(node.header.entries - 1).disk_addr].read_raw(buf);
            memcpy(ret.data() + (node.header.entries - 1) * 0x100, buf, node.size_offset);
            return ret;
        }

        void write_all(const char *data) {
            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            uint64_t new_size = strlen(data),
                     new_blocks = (new_size + 0xFF) / 0x100,
                     new_offset = ((new_size + 0xFF) & 0xFF) + 1;
            if (new_blocks > node.header.entries)
                for (uint32_t i = node.header.entries; i < new_blocks; ++i) {
                    node.extent(i).disk_addr = fs_.allocator_.new_block();
                    node.extent(i).len = 1; // todo allocate extents
                }
            node.header.entries = new_blocks;
            node.size_blocks = new_blocks;
            node.size_offset = new_offset;

            for (uint32_t i = 0; i < node.header.entries - 1; ++i)
                fs_.disk_[node.extent(i).disk_addr].write_raw(data + i * 0x100);
            char buf[256];
            memcpy(buf, data + (new_blocks - 1) * 0x100, new_offset);
            fs_.disk_[node.extent(node.header.entries - 1).disk_addr].write_raw(buf);
            fs_.disk_[addr_] = node;
        }

        void insert(uint64_t pos, const char *data) {
            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            // todo
        }

        void erase(uint64_t pos, uint64_t len) {
            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            // todo
        }

    private:
    };

    class fs_folder_handle : public fs_handle {
    public:
        fs_folder_handle(file_system &fs, uint64_t addr) : fs_handle(fs, addr) {} // todo validate

        fs_folder_handle(const fs_folder_handle &) = default;

        fs_folder_handle &operator=(const fs_folder_handle &) = default;

        fs_folder_handle(fs_folder_handle &&) = default;

        fs_folder_handle &operator=(fs_folder_handle &&) = default;

        fs_file_handle open(const char *file_name) {
            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            for (uint32_t i = 0; i < node.header.entries; ++i) {
                directory_node child = fs_.disk_[node.file_pointer(i)];
                if (child.folder_bit() == false && strcmp(child.name, file_name) == 0)
                    return {fs_, node.file_pointer(i)};
            }
            throw except(ERROR_FS_NAME_NOT_EXIST);
        }

        fs_folder_handle open_folder(const char *folder_name) {
            if (strcmp(folder_name, ".") == 0)
                return *this; // copy

            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            if (strcmp(folder_name, "..") == 0) {
                if (addr_ != fs_.FILE_ROOT)
                    return {fs_, node.parent_addr};
                return *this;
            }

            for (uint32_t i = 0; i < node.header.entries; ++i) {
                directory_node child = fs_.disk_[node.file_pointer(i)];
                if (child.folder_bit() == true && strcmp(child.name, folder_name) == 0)
                    return {fs_, node.file_pointer(i)};
            }
            throw except(ERROR_FS_NAME_NOT_EXIST);
        }

        void create(const char *name, bool is_folder) {
            if (!file_system::is_name_valid(name))
                throw except(ERROR_FS_NAME_INVALID);
            if (strlen(name) >= 0x40)
                throw except(ERROR_FS_NAME_TOO_LONG);

            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            if (node.header.entries == node.header.entries_capacity)
                throw except(ERROR_FS_CAPACITY_EXCEEDED); // todo bp tree split
            for (uint32_t i = 0; i < node.header.entries; ++i) {
                directory_node child = fs_.disk_[node.file_pointer(i)];
                if (strcmp(child.name, name) == 0)
                    throw except(ERROR_FS_NAME_ALREADY_EXIST);
            }

            directory_node new_node = is_folder ? directory_node::default_folder() : directory_node::default_file();
            strcpy(new_node.name, name);
            new_node.parent_addr = addr_;
            uint64_t new_addr = fs_.allocator_.new_block();
            node.file_pointer(node.header.entries) = new_addr;
            ++node.header.entries;
            fs_.disk_[new_addr] = new_node;
            fs_.disk_[addr_] = node;
        }

        void remove(const char *name, bool is_folder) {
            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            uint32_t index = 0;
            bool found = false;
            for (uint32_t i = 0; i < node.header.entries; ++i) {
                directory_node child = fs_.disk_[node.file_pointer(i)];
                if (child.folder_bit() == is_folder && strcmp(child.name, name) == 0) {
                    index = i;
                    found = true;
                    break;
                }
            }
            if (!found)
                throw except(ERROR_FS_NAME_NOT_EXIST);
            uint32_t recycle_addr = node.file_pointer(index); //
            {
                std::lock_guard<std::mutex> count_lock(fs_.count_mutex_);
                if (fs_.handle_instance_count_.contains(recycle_addr))
                    throw except(ERROR_FS_BUSY_HANDLE);
            }

            if (index != node.header.entries - 1)
                memmove(
                    node.tree_data + index * sizeof(uint64_t),
                    node.tree_data + (index + 1) * sizeof(uint64_t),
                    (node.header.entries - 1 - index) * sizeof(uint64_t) // todo hashed pointer
                );
            --node.header.entries;
            fs_.allocator_.delete_block(recycle_addr);
            fs_.disk_[addr_] = node;
        }

        std::vector<std::string> list() {
            std::lock_guard<std::mutex> lock(fs_.data_mutex_);
            directory_node node = fs_.disk_[addr_];
            std::vector<std::string> ret;
            for (uint32_t i = 0; i < node.header.entries; ++i) {
                directory_node child = fs_.disk_[node.file_pointer(i)];
                ret.emplace_back(child.name);
                if (child.folder_bit())
                    ret.back().append("/");
            }
            return ret;
        }

    private:
    };

    inline fs_folder_handle file_system::root_folder() {
        return {*this, FILE_ROOT};
    }

}

#endif
