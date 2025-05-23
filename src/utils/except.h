#ifndef EXCEPT_H
#define EXCEPT_H

#include <cstring>

namespace cs2313 {

    enum ERROR_CODE {
        ERROR_SOCKET_CREATE_FAIL = 0x0101,
        ERROR_SOCKET_CONNECT_FAIL = 0x0102,
        ERROR_SOCKET_SEND_FAIL = 0x0103,
        ERROR_SOCKET_RECV_FAIL = 0x0104,
        ERROR_SOCKET_CLOSED_BY_REMOTE = 0x0105,
        ERROR_SOCKET_TERMINATED = 0x0106,
        ERROR_VIRTUAL_DRIVE_INVALID_ARGS = 0x0201,
        ERROR_VIRTUAL_DRIVE_FILE_CREATE = 0x0202,
        ERROR_VIRTUAL_DRIVE_MMAP = 0x0203,
        ERROR_DISK_ADDR_INVALID = 0x0241,
        ERROR_FS_HANDLE_BUSY = 0x0421,
        ERROR_FS_HANDLE_INVALID = 0x0422,
        ERROR_FS_FULL_NODE = 0x0431,
        ERROR_FS_NAME_NOT_EXIST = 0x0441,
        ERROR_FS_NAME_ALREADY_EXIST = 0x0442,
        ERROR_FS_NAME_TOO_LONG = 0x0443,
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

        int error_code() const noexcept { return error_code_; }

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
