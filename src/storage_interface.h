#ifndef STORAGE_INTERFACE_H
#define STORAGE_INTERFACE_H

#include <cstdint>

namespace cs2313 {

    struct disk_description {
        uint64_t cylinders;
        uint64_t sectors_per_cylinder;
        uint64_t bytes_per_sector;
    };

    class storage_interface {
    public:
        virtual void read(uint64_t sector_addr, char *data) = 0;

        virtual void write(uint64_t sector_addr, const char *data) = 0;

        virtual disk_description get_description() = 0;

        virtual void shutdown() = 0;

        virtual ~storage_interface() = default;
    };

    typedef unsigned char io_instr;
    inline static constexpr io_instr IO_INSTR_GET_DESC = 0, // only in constructor, not as an transaction
                                     IO_INSTR_READ = 1,
                                     IO_INSTR_WRITE = 2,
                                     IO_INSTR_SHUTDOWN = 3;
}


#endif
