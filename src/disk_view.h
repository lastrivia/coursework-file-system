#ifndef DISK_VIEW_H
#define DISK_VIEW_H

#include "disk_interface.h"

namespace cs2313 {

    class disk_view {
    public:
        disk_view(disk_interface &disk): disk_(disk) {}

        class storage_proxy {
        public:
            storage_proxy() = delete;

            template<typename T>
            operator T() const {
                T temp;
                disk_.read(addr_, reinterpret_cast<char *>(&temp));
                return temp;
            }

            template<typename T>
            storage_proxy &operator=(const T &val) {
                disk_.write(addr_, reinterpret_cast<const char *>(&val));
                return *this;
            }

            void read_raw(char *s) {
                disk_.read(addr_, s);
            }

            void write_raw(const char *s) {
                disk_.write(addr_, s);
            }

        private:
            friend class disk_view;
            disk_interface &disk_;
            uint64_t addr_;

            storage_proxy(disk_interface &disk, const uint64_t addr):
                disk_(disk), addr_(addr) {}
        };

        storage_proxy operator[](uint64_t addr) const {
            return {disk_, addr};
        }

    private:
        disk_interface &disk_;
    };

}

#endif
