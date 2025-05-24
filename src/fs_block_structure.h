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

    struct extent_token {
        uint64_t disk_block_no;
        uint64_t len;
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
        uint64_t disk_addr; // start from
        uint32_t file_block_no; // start from
        uint32_t len;
    };

    struct extent_index_entry {
        uint64_t node_addr; // point to extent block
        uint32_t file_block_no; // start from
        char unused_0c[4];
    };

    static_assert(sizeof(extent_entry) == sizeof(extent_index_entry));

    struct directory_node {
        // 00
        uint16_t magic;
        char attrib_bits_l;
        char attrib_bits_h;
        uint32_t meta_checksum; // unused yet
        uint32_t size_blocks;
        uint32_t size_offset;

        // 10
        char name[0x40];
        // todo indirect uint64_t name_block

        // 50
        int64_t timestamp;
        uint64_t parent_addr;
        // uint8_t owner_id;
        // char access_bits;
        // char unused_meta_2a[0x36];
        char unused_meta_2a[0x10];

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

        static directory_node default_folder() {
            directory_node node;

            node.magic = 0x0909;
            node.attrib_bits_h = 0;
            node.attrib_bits_l = 0;
            node.folder_bit() = true;
            node.name[0] = 0;
            node.timestamp = time(nullptr);
            node.parent_addr = 0;

            node.header.magic = 0x0909;
            node.header.entries = 0;
            node.header.entries_capacity = 8;
            node.header.tree_depth = 0;

            return node;
        }

        static directory_node default_file() {
            directory_node node;

            node.magic = 0x0909;
            node.attrib_bits_h = 0;
            node.attrib_bits_l = 0;
            node.folder_bit() = false;
            node.size_blocks = 0;
            node.size_offset = 0;
            node.name[0] = 0;
            node.timestamp = time(nullptr);
            node.parent_addr = 0;

            node.header.magic = 0x0909;
            node.header.entries = 0;
            node.header.entries_capacity = 8;
            node.header.tree_depth = 0;

            return node;
        }
    };

    static_assert(sizeof(directory_node) == 0x100);

    struct intermediate_node {
        file_extent_header header;
        char tree_data[0xF0];
        // folder: 30 node pointers // todo name hash?
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
        uint16_t magic; // 0x0909
        uint16_t entries;
        uint16_t entries_capacity;
        uint16_t tree_depth;
        uint32_t checksum; // unused yet
        char unused_0a[4];
        uint64_t free_blocks;
        uint64_t max_cont_blocks;
    };

    struct alloc_extent_entry {
        uint64_t disk_block_no; // start from
        uint64_t len;
    };

    struct alloc_extent_index_entry {
        uint64_t node_addr; // point to extent block
        uint64_t disk_block_no; // start from
    };

    static_assert(sizeof(alloc_extent_entry) == sizeof(alloc_extent_index_entry));

    struct allocator_node {
        alloc_extent_header header;
        char tree_data[0xE0];
        // 14 extent entries (index entries if tree_depth > 0)

        alloc_extent_entry &extent(const int index) {
            return *reinterpret_cast<alloc_extent_entry *>(tree_data + index * sizeof(alloc_extent_entry));
        }

        alloc_extent_index_entry &extent_index(const int index) {
            return *reinterpret_cast<alloc_extent_index_entry *>(tree_data + index * sizeof(alloc_extent_index_entry));
        }

        static allocator_node default_root(const extent_token &init_extent) {
            allocator_node node;

            node.header.magic = 0x0909;
            node.header.entries = 1;
            node.header.entries_capacity = 15;
            node.header.tree_depth = 0;
            node.header.free_blocks = init_extent.len;
            node.header.max_cont_blocks = init_extent.len;
            node.extent(0).disk_block_no = init_extent.disk_block_no;
            node.extent(0).len = init_extent.len;

            return node;
        }
    };

    static_assert(sizeof(allocator_node) == 0x100);
}

#endif
