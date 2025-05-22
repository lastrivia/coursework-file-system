#ifndef FS_BLOCK_STRUCTURE_H
#define FS_BLOCK_STRUCTURE_H

#include <cstdint>

namespace cs2313 {

    class bit_proxy {
    public:
        bit_proxy(char *ch, const uint8_t digit):
            ch_(ch), digit_(digit) {}

        operator bool() const {
            return (*ch_ >> digit_) & 1;
        }

        bit_proxy &operator=(const bool b) {
            if (b)
                *ch_ |= (1 << digit_);
            else
                *ch_ &= ~(1 << digit_);
            return *this;
        }

    private:
        char *ch_;
        uint8_t digit_;
    };

    struct file_extent_header {
        uint16_t magic;
        uint16_t entries;
        uint16_t entries_capacity;
        uint16_t tree_depth;
        uint32_t checksum;
        char unused_0a[4];
    };

    struct extent_entry {
        uint64_t disk_addr;
        uint32_t file_block_no;
        uint32_t len;
    };

    struct extent_index_entry {
        uint64_t disk_addr; // point to extent blocks
        uint32_t file_block_no;
        char unused_0c[4];
    };

    struct directory_node {
        // 00
        uint16_t magic;
        char attrib_bits_l;
        char attrib_bits_h;
        uint32_t meta_checksum;
        uint32_t size_blocks;
        uint32_t size_offset;

        // 10
        char name[0x20];
        // todo indirect uint64_t name_block

        // 30
        int64_t timestamp;
        uint8_t owner_id;
        char access_bits;
        char unused_meta_2a[0x36];

        // 70
        file_extent_header header;

        // 80
        char tree_data[0x80];
        // folder: 16 file pointers
        // file: 8 extent entries (index entries if tree_depth > 0)

        bit_proxy folder_bit() { return {&attrib_bits_l, 0}; }
        // bit_proxy indirect_name_bit() { return {&attrib_bits_l, 1}; } // todo

        uint64_t &file_pointer(const int index) {
            return *reinterpret_cast<uint64_t *>(tree_data + index * sizeof(uint64_t));
        }

        extent_entry &extent(const int index) {
            return *reinterpret_cast<extent_entry *>(tree_data + index * sizeof(extent_entry));
        }

        extent_index_entry &extent_index(const int index) {
            return *reinterpret_cast<extent_index_entry *>(tree_data + index * sizeof(extent_index_entry));
        }
    };

    static_assert(sizeof(directory_node) == 0x100);

    struct intermediate_node {
        file_extent_header header;
        char tree_data[0xF0];
        // folder: 30 node pointers
        // file: 15 extent entries (index entries if tree_depth > 0)

        uint64_t &file_pointer(const int index) {
            return *reinterpret_cast<uint64_t *>(tree_data + index * sizeof(uint64_t));
        }

        extent_entry &extent(const int index) {
            return *reinterpret_cast<extent_entry *>(tree_data + index * sizeof(extent_entry));
        }

        extent_index_entry &extent_index(const int index) {
            return *reinterpret_cast<extent_index_entry *>(tree_data + index * sizeof(extent_index_entry));
        }
    };

    static_assert(sizeof(intermediate_node) == 0x100);

    struct alloc_extent_header {
        uint16_t magic;
        uint16_t entries;
        uint16_t entries_capacity;
        uint16_t tree_depth;
        uint32_t checksum;
        char unused_0a[4];
        uint64_t free_blocks;
        uint64_t max_cont_blocks;
    };

    struct alloc_extent_entry {
        uint64_t disk_addr;
        uint32_t len;
        char unused_0c[4];
    };

    struct alloc_node {
        alloc_extent_header header;
        char tree_data[0xE0];
        // 28 alloc node pointers (if tree_depth > 0)
        // 14 extent entries (if tree_depth = 0)

        uint64_t &alloc_pointer(const int index) {
            return *reinterpret_cast<uint64_t *>(tree_data + index * sizeof(uint64_t));
        }

        alloc_extent_entry &extent(const int index) {
            return *reinterpret_cast<alloc_extent_entry *>(tree_data + index * sizeof(alloc_extent_entry));
        }
    };

    static_assert(sizeof(alloc_node) == 0x100);
}

#endif
