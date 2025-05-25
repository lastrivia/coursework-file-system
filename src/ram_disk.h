#ifndef RAM_DISK_H
#define RAM_DISK_H

#include <cstring>
#include "storage_interface.h"
#include "utils/except.h"

namespace cs2313 {

    class ram_disk : public storage_interface {
    public:
        explicit ram_disk(uint64_t cylinders, uint64_t sectors_per_cylinder, uint64_t bytes_per_sector)
            : cylinders_(cylinders),
              sectors_per_cylinder_(sectors_per_cylinder),
              bytes_per_sector_(bytes_per_sector) {

            storage_ = new char[cylinders_ * sectors_per_cylinder_ * bytes_per_sector_];
            memset(storage_, 0, cylinders_ * sectors_per_cylinder_ * bytes_per_sector_);
        }

        void read(const uint64_t addr, char *data) override {
            if(!is_valid_addr(addr))
                throw except(ERROR_DISK_ADDR_INVALID, "Invalid disk address");
            memcpy(data, &storage_[addr * bytes_per_sector_], bytes_per_sector_);
        }

        void write(const uint64_t addr, const char *data) override {
            if(!is_valid_addr(addr))
                throw except(ERROR_DISK_ADDR_INVALID, "Invalid disk address");
            memcpy(&storage_[addr * bytes_per_sector_], data, bytes_per_sector_);
        }

        disk_description get_description() override {
            return {cylinders_, sectors_per_cylinder_, bytes_per_sector_};
        }

        void shutdown() override {}

        ~ram_disk() override {
            delete[] storage_;
        }

    private:
        uint64_t cylinders_;
        uint64_t sectors_per_cylinder_;
        uint64_t bytes_per_sector_;
        char *storage_;

        bool is_valid_addr(const uint64_t addr) const {
            return addr < cylinders_ * sectors_per_cylinder_;
        }
    };

}


#endif
