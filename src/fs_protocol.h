#ifndef FS_PROTOCOL_H
#define FS_PROTOCOL_H

#include <cstdint>

#include "utils/except.h"

namespace cs2313 {

    typedef uint8_t fs_instr;
    static constexpr fs_instr FS_INSTR_CD = 0,
                              FS_INSTR_LS = 1,
                              FS_INSTR_MK = 2,
                              FS_INSTR_RM = 3,
                              FS_INSTR_MKDIR = 4,
                              FS_INSTR_RMDIR = 5,
                              FS_INSTR_CHMOD = 6, // todo
                              FS_INSTR_FILE_CAT = 8,
                              FS_INSTR_FILE_W = 9,
                              FS_INSTR_FILE_I = 10,
                              FS_INSTR_FILE_D = 11,
                              FS_INSTR_FORMAT = 15;

    typedef uint8_t fs_reply;
    static constexpr fs_reply FS_REPLY_LOGIN_AUTH = 0x10,
                              FS_REPLY_REGISTER_AUTH = 0x11,
                              FS_REPLY_LOGIN_SUCCESS = 0x12,
                              FS_REPLY_LOGIN_REJECT = 0x13,
                              FS_REPLY_REGISTER_SUCCESS = 0x14,
                              FS_REPLY_REGISTER_REJECT = 0x15, // todo
                              FS_REPLY_OK = 0x20,
                              FS_REPLY_NOT_EXIST = 0x30,
                              FS_REPLY_ALREADY_EXIST = 0x31,
                              FS_REPLY_NAME_TOO_LONG = 0x32,
                              FS_REPLY_NAME_INVALID = 0x33,
                              FS_REPLY_BUSY_HANDLE = 0x34,
                              FS_REPLY_CAPACITY_EXCEEDED = 0x35,
                              FS_REPLY_ACCESS_DENIED = 0x36,
                              FS_REPLY_UNKNOWN_ERROR = 0x3F;

    inline fs_reply error_reply(ERROR_CODE error_code) {
        switch (error_code) {
            case ERROR_FS_BUSY_HANDLE:
                return FS_REPLY_BUSY_HANDLE;
            case ERROR_FS_CAPACITY_EXCEEDED:
                return FS_REPLY_CAPACITY_EXCEEDED;
            case ERROR_FS_ACCESS_DENIED:
                return FS_REPLY_ACCESS_DENIED;
            case ERROR_FS_NAME_NOT_EXIST:
                return FS_REPLY_NOT_EXIST;
            case ERROR_FS_NAME_ALREADY_EXIST:
                return FS_REPLY_ALREADY_EXIST;
            case ERROR_FS_NAME_TOO_LONG:
                return FS_REPLY_NAME_TOO_LONG;
            case ERROR_FS_NAME_INVALID:
                return FS_REPLY_NAME_INVALID;
            default:
                return FS_REPLY_UNKNOWN_ERROR;
        }
    }
}

#endif
