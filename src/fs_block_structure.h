#ifndef FS_BLOCK_STRUCTURE_H
#define FS_BLOCK_STRUCTURE_H

#include <cstdint>
#include <sodium.h>

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

        uint64_t parent_addr;
        uint64_t left_addr;
        uint64_t right_addr;
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

    static constexpr uint32_t FILE_NAME_LENGTH_MAX = 0x48;

    struct directory_child_list_header {
        uint8_t child_count;
        uint8_t child_capacity;
        bool is_tail;
        char unused_03[5];
        uint64_t next_addr;
    };

    struct directory_child_entry {
        bool empty;
        char unused_01[3];
        uint32_t name_hash;
        uint64_t node_addr;
    };

    struct directory_node {
        // 00
        uint16_t magic;
        char attrib_bits;
        char access_bits;
        uint32_t owner_id;
        uint32_t size_blocks;
        uint32_t size_offset;

        // 10
        int64_t timestamp;

        // 18
        char name[FILE_NAME_LENGTH_MAX];
        // todo indirect uint64_t name_block

        // 60
        file_extent_header header;

        // 80
        char data[0x80];
        // folder: 16 file pointers
        // file: 8 extent entries (index entries if tree_depth > 0)

        bit_proxy folder_bit() { return {&attrib_bits, 0}; }
        // bit_proxy indirect_name_bit() { return {&attrib_bits_l, 1}; } // todo

        uint64_t &file_pointer(const int index) {
            return *reinterpret_cast<uint64_t *>(data + index * sizeof(uint64_t));
        }

        extent_entry &extent(const int index) {
            return *reinterpret_cast<extent_entry *>(data + index * sizeof(extent_entry));
        }

        extent_index_entry &extent_index(const int index) {
            return *reinterpret_cast<extent_index_entry *>(data + index * sizeof(extent_index_entry));
        }

        static directory_node default_folder() {
            directory_node node;

            node.magic = 0x0909;
            node.attrib_bits = 0;
            node.folder_bit() = true;
            node.name[0] = 0;
            node.timestamp = time(nullptr);

            node.header.magic = 0x0909;
            node.header.entries = 0;
            node.header.entries_capacity = 16; // todo hashed ptr
            node.header.tree_depth = 0;

            node.header.parent_addr = 0;
            node.header.left_addr = 0;
            node.header.right_addr = 0;
            // O marks empty pointer, since node #0 is reserved for allocator

            return node;
        }

        static directory_node default_file() {
            directory_node node;

            node.magic = 0x0909;
            node.attrib_bits = 0;
            node.folder_bit() = false;
            node.size_blocks = 0;
            node.size_offset = 0;
            node.name[0] = 0;
            node.timestamp = time(nullptr);

            node.header.magic = 0x0909;
            node.header.entries = 0;
            node.header.entries_capacity = 8;
            node.header.tree_depth = 0;

            node.header.parent_addr = 0;
            node.header.left_addr = 0;
            node.header.right_addr = 0;
            // O marks empty pointer, since node #0 is reserved for allocator

            return node;
        }
    };

    static_assert(sizeof(directory_node) == 0x100);

    struct intermediate_node {
        file_extent_header header;
        char tree_data[0xE0];
        // file: 14 extent entries (index entries if tree_depth > 0)

        // uint64_t &file_pointer(const int index) {
        //     return *reinterpret_cast<uint64_t *>(tree_data + index * sizeof(uint64_t));
        // }

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

        uint64_t parent_addr;
        uint64_t left_addr;
        uint64_t right_addr;

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
        char tree_data[0xD0];
        // 13 extent entries (index entries if tree_depth > 0)

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
            node.header.entries_capacity = 13;
            node.header.tree_depth = 0;
            node.header.free_blocks = init_extent.len;
            node.header.max_cont_blocks = init_extent.len;
            node.extent(0).disk_block_no = init_extent.disk_block_no;
            node.extent(0).len = init_extent.len;

            return node;
        }
    };

    static_assert(sizeof(allocator_node) == 0x100);

    static constexpr uint32_t USER_ENTRIES_PER_NODE = 15;

    struct user_list_header {
        uint32_t id_offset;
        uint8_t user_count;
        uint8_t user_capacity;
        bool is_tail;
        char unused_07[1];
        uint64_t next_addr;
    };

    struct user_entry {
        bool empty;
        char unused_01[3];
        uint32_t username_hash;
        uint64_t info_addr;
    };

    struct user_list_block {
        user_list_header header;
        user_entry entry[USER_ENTRIES_PER_NODE];

        static user_list_block default_block(uint16_t init_id_offset) {
            user_list_block block;

            block.header.id_offset = init_id_offset;
            block.header.user_count = 0;
            block.header.user_capacity = USER_ENTRIES_PER_NODE;
            block.header.is_tail = true;
            block.header.next_addr = 0;
            for (auto &e: block.entry)
                e.empty = true;

            return block;
        }
    };

    static_assert(sizeof(user_list_block) == 0x100);

    static constexpr uint32_t USER_NAME_LENGTH_MAX = 0xd0;

    struct user_info_block {
        char user_name[USER_NAME_LENGTH_MAX];
        char salt[crypto_pwhash_SALTBYTES];
        char key[crypto_auth_hmacsha256_KEYBYTES];
    };

    static_assert(sizeof(user_info_block) == 0x100);
}

#endif
