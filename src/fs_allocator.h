#ifndef FS_ALLOCATOR_H
#define FS_ALLOCATOR_H

#include <mutex>
#include <unordered_map>
#include <vector>

#include "fs_block_structure.h"
#include "disk_view.h"

namespace cs2313 {

    class fs_allocator {
    public:
        fs_allocator(disk_view &disk, uint64_t alloc_root):
            disk_(disk), alloc_root_(alloc_root) {}

        fs_allocator(const fs_allocator &other) = delete;

        fs_allocator operator=(const fs_allocator &other) = delete;

        uint64_t new_block() {
            std::lock_guard<std::mutex> lock(global_mutex_);
            uint64_t ret = new_block_i();
            sync();
            return ret;
        }

        void delete_block(uint64_t addr) {
            std::lock_guard<std::mutex> lock(global_mutex_);
            // todo recycle
            // delete_block_internal(addr);
            // sync();
        }

        // std::vector<extent_allocated> new_extent(uint32_t len) {
        //     std::lock_guard<std::mutex> lock(global_mutex_);
        //     // todo
        //     sync();
        //     // todo return
        // }

        void delete_extent(extent_token token) {
            std::lock_guard<std::mutex> lock(global_mutex_);
            // todo recycle
            // sync();
        }

    private:
        disk_view &disk_;
        uint64_t alloc_root_;
        std::mutex global_mutex_; // todo segmented mutex

        std::unordered_map<uint64_t, allocator_node> node_cache_;
        std::unordered_map<uint64_t, bool> node_dirty_;
        std::unordered_map<uint64_t, uint64_t> parent_map_;

        allocator_node &fetch_node(uint64_t addr) {
            auto it = node_cache_.find(addr);
            if (it != node_cache_.end()) {
                return it->second;
            } else {
                allocator_node node = disk_[addr];
                node_cache_[addr] = node;
                node_dirty_[addr] = false;
                return node_cache_[addr];
            }
        }

        void set_dirty(uint64_t addr) {
            node_dirty_[addr] = true;
        }

        void sync() {
            for (auto &it: node_cache_) {
                if (node_dirty_[it.first]) {
                    disk_[it.first] = it.second;
                }
            }
            node_cache_.clear();
            node_dirty_.clear();
        }

        uint64_t new_block_i() {

            uint64_t current_addr = alloc_root_;
            allocator_node *current_node = &fetch_node(current_addr);
            std::vector<uint64_t> path;

            while (current_node->header.tree_depth > 0) {
                path.push_back(current_addr);
                current_addr = current_node->extent_index(0).node_addr;
                current_node = &fetch_node(current_addr);
            }

            if (current_node->header.entries == 0)
                return 0; // todo throw

            alloc_extent_entry &entry = current_node->extent(0);
            uint64_t block_allocated = entry.disk_block_no;

            if (entry.len == 1) {
                node_remove_entry(current_addr, 0);
            } else {
                ++entry.disk_block_no;
                --entry.len;
            }
            set_dirty(current_addr);

            // todo maintain
            // if (current_node->header.entries == 0 && current_addr != alloc_root_) {
            //     delete_node(current_addr);
            // }

            return block_allocated;
        }

        void delete_block_internal(uint64_t addr) {
            // todo
        }

        void node_remove_entry(uint64_t node_addr, int index) {
            // allocator_node &node = fetch_node(node_addr);
            // // todo memcpy
            // if (node.header.tree_depth == 0) {
            //     for (int i = index; i < node.header.entries - 1; ++i) {
            //         node.extent(i) = node.extent(i + 1);
            //     }
            // } else {
            //     for (int i = index; i < node.header.entries - 1; ++i) {
            //         node.extent_index(i) = node.extent_index(i + 1);
            //     }
            // }
            // node.header.entries--;
        }

        void delete_node(uint64_t node_addr) {
            // auto parent_it = parent_map_.find(node_addr);
            // if (parent_it == parent_map_.end()) return;
            //
            // uint64_t parent_addr = parent_it->second;
            // allocator_node &parent = fetch_node(parent_addr);
            //
            // int index = -1;
            // for (int i = 0; i < parent.header.entries; ++i) {
            //     if (parent.extent_index(i).node_addr == node_addr) {
            //         index = i;
            //         break;
            //     }
            // }
            // if (index == -1) return;
            //
            // node_remove_entry(parent_addr, index);
            // delete_block_internal(node_addr);
            //
            // if (parent.header.entries == 0 && parent_addr != alloc_root_) {
            //     delete_node(parent_addr);
            // }
        }

        void bp_tree_split(uint64_t node_addr) {
            // allocator_node &node = fetch_node(node_addr);
            // uint64_t new_addr = new_block_i();
            // allocator_node &new_node = fetch_node(new_addr);
            // new_node.header = node.header;
            // int split_pos = node.header.entries / 2;
            //
            // if (node.header.tree_depth == 0) {
            //     for (int i = split_pos; i < node.header.entries; ++i) {
            //         new_node.extent(i - split_pos) = node.extent(i);
            //     }
            // } else {
            //     for (int i = split_pos; i < node.header.entries; ++i) {
            //         new_node.extent_index(i - split_pos) = node.extent_index(i);
            //         parent_map_[node.extent_index(i).node_addr] = new_addr;
            //     }
            // }
            // new_node.header.entries = node.header.entries - split_pos;
            // node.header.entries = split_pos;
            // set_dirty(node_addr);
            // set_dirty(new_addr);
            //
            // // update parent
            // if (node_addr == alloc_root_) {
            //     bp_tree_split_root(node_addr, new_addr);
            // } else {
            //     uint64_t parent_addr = parent_map_[node_addr];
            //     allocator_node &parent = fetch_node(parent_addr);
            //     alloc_extent_index_entry entry;
            //     entry.node_addr = new_addr;
            //     entry.disk_block_no = new_node.extent_index(new_node.header.entries - 1).disk_block_no;
            //     int pos = find_insert_pos(parent, entry.disk_block_no);
            //     node_insert_index_entry(parent_addr, pos, entry);
            //     parent_map_[new_addr] = parent_addr;
            // }
        }

        void bp_tree_split_root(uint64_t old_root, uint64_t new_node) {
            // uint64_t new_root = new_block_i();
            // allocator_node &root = fetch_node(new_root);
            // root.header.magic = 0x0909;
            // root.header.tree_depth = fetch_node(old_root).header.tree_depth + 1;
            // root.header.entries = 2;
            // root.header.entries_capacity = 14;
            //
            // root.extent_index(0).node_addr = old_root;
            // root.extent_index(0).disk_block_no = get_max_block(old_root);
            // root.extent_index(1).node_addr = new_node;
            // root.extent_index(1).disk_block_no = get_max_block(new_node);
            //
            // alloc_root_ = new_root;
            // set_dirty(new_root);
            // parent_map_[old_root] = new_root;
            // parent_map_[new_node] = new_root;
        }

        void bp_tree_merge(uint64_t node_addr) {
            // todo
        }
    };

}

#endif
