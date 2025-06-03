#ifndef EXCEPT_H
#define EXCEPT_H

#include <cstring>

namespace cs2313 {

    enum ERROR_CODE {
        ERROR_SOCKET_CREATE_FAIL = 0x11,
        ERROR_SOCKET_CONNECT_FAIL = 0x12,
        ERROR_SOCKET_SEND_FAIL = 0x13,
        ERROR_SOCKET_RECV_FAIL = 0x14,
        ERROR_SOCKET_CLOSED_BY_REMOTE = 0x15,
        ERROR_SOCKET_TERMINATED = 0x16,
        ERROR_PWD_INIT_FAIL = 0x21,
        ERROR_PWD_COMPUTE_FAIL = 0x22,
        ERROR_DISK_ADDR_INVALID = 0x81,
        ERROR_VIRTUAL_DRIVE_INVALID_ARGS = 0x101,
        ERROR_VIRTUAL_DRIVE_FILE_CREATE = 0x102,
        ERROR_VIRTUAL_DRIVE_MMAP = 0x103,
        ERROR_FS_BUSY_HANDLE = 0x141,
        ERROR_FS_CAPACITY_EXCEEDED = 0x142,
        ERROR_FS_ACCESS_DENIED = 0x143,
        ERROR_FS_NAME_NOT_EXIST = 0x151,
        ERROR_FS_NAME_ALREADY_EXIST = 0x152,
        ERROR_FS_NAME_TOO_LONG = 0x153,
        ERROR_FS_NAME_INVALID = 0x154,
    };

    class except : public std::exception {
    public:
        except(const ERROR_CODE error_code):
            system_errno_(0),
            error_code_(error_code),
            message_("") {}

        except(const ERROR_CODE error_code, const std::string_view message):
            system_errno_(0),
            error_code_(error_code),
            message_(message) {}

        except(const int system_errno, const ERROR_CODE error_code, const std::string_view message):
            system_errno_(system_errno),
            error_code_(error_code),
            message_(message) {}

        int system_errno() const noexcept { return system_errno_; }

        ERROR_CODE error_code() const noexcept { return error_code_; }

        const std::string &message() const noexcept { return message_; }

        friend std::ostream &operator<<(std::ostream &out, except &e) {
            out << e.error_code_ << ": " << e.message_;
            if (e.system_errno_)
                out << " (" << strerror(e.system_errno_) << ")";
            out << std::endl;
            return out;
        }

    private:
        int system_errno_;
        ERROR_CODE error_code_;
        std::string message_;
    };

}
#endif
