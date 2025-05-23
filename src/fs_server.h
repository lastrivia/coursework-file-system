#ifndef FS_SERVER_H
#define FS_SERVER_H

#include <atomic>
#include <thread>

#include "fs.h"
#include "utils/socket.h"

namespace cs2313 {

    typedef uint8_t fs_instr;
    static constexpr fs_instr FS_INSTR_FORMAT = 15,
                              FS_INSTR_CD = 0,
                              FS_INSTR_LS = 1,
                              FS_INSTR_MK = 2,
                              FS_INSTR_RM = 3,
                              FS_INSTR_MKDIR = 4,
                              FS_INSTR_RMDIR = 5,
                              FS_INSTR_FILE_CAT = 8,
                              FS_INSTR_FILE_W = 9,
                              FS_INSTR_FILE_I = 10,
                              FS_INSTR_FILE_D = 11;

    typedef uint8_t fs_reply;
    static constexpr fs_reply FS_REPLY_INITIALIZED = 0x10,
                              FS_REPLY_INITIALIZED_NO_FORMAT = 0x11,
                              FS_REPLY_OK = 0x20,
                              FS_REPLY_REJECT = 0x30; // todo detailed

    class fs_server {
    public:
        fs_server(file_system &fs, uint16_t port):
            fs_(fs),
            server_socket_(port),
            threads_sig_term_(false) {}

        ~fs_server() {
            threads_sig_term_.store(true);
            for (auto &t: threads_)
                if (t.joinable())
                    t.join();
        }

        void serve_client(server_connection_socket_handle connection_socket) {
            fs_folder_handle current_folder = fs_.root_folder();
            std::string str_buf, data_buf;
            std::string path = "/";

            try {
                if (fs_.formatted())
                    connection_socket.send(FS_REPLY_INITIALIZED);
                else
                    connection_socket.send(FS_REPLY_INITIALIZED_NO_FORMAT);
            } catch (except &e) {
                throw; // todo
            }

            while (threads_sig_term_.load(std::memory_order_acquire) == false) {
                try {
                    fs_instr instr;
                    connection_socket.recv(instr);
                    switch (instr) {

                        case FS_INSTR_CD: {
                            connection_socket.recv_str(str_buf);
                            try {
                                fs_folder_handle folder = current_folder.open_folder(str_buf.c_str());
                                current_folder = folder;
                                if (str_buf != ".") {
                                    if (str_buf == "..") {
                                        if (path != "/") {
                                            path.pop_back();
                                            size_t pos = path.find_last_of('/');
                                            path.erase(pos + 1);
                                        }
                                    } else {
                                        path += str_buf;
                                        path += '/';
                                    }
                                }
                                connection_socket.send(FS_REPLY_OK);
                                connection_socket.send_str(path);
                            } catch (except &e) {
                                switch (e.error_code()) {
                                    case ERROR_FS_NAME_NOT_EXIST:
                                        connection_socket.send(FS_REPLY_REJECT);
                                        break;
                                    default:
                                        throw;
                                }
                            }
                        }
                        break;

                        case FS_INSTR_LS: {
                            std::vector<std::string> ls = current_folder.list();
                            uint32_t size = ls.size();
                            connection_socket.send(FS_REPLY_OK);
                            connection_socket.send(size);
                            for (auto &s: ls)
                                connection_socket.send(s);
                        }
                        break;

                        case FS_INSTR_MK:
                        case FS_INSTR_MKDIR: {
                            connection_socket.recv_str(str_buf);
                            try {
                                current_folder.create(str_buf.c_str(), instr == FS_INSTR_MKDIR);
                                connection_socket.send(FS_REPLY_OK);
                            } catch (except &e) {
                                switch (e.error_code()) {
                                    case ERROR_FS_FULL_NODE:
                                    case ERROR_FS_NAME_ALREADY_EXIST:
                                    case ERROR_FS_NAME_TOO_LONG:
                                        connection_socket.send(FS_REPLY_REJECT);
                                        break;
                                    default:
                                        throw;
                                }
                            }
                        }
                        break;

                        case FS_INSTR_RM:
                        case FS_INSTR_RMDIR: {
                            connection_socket.recv_str(str_buf);
                            try {
                                current_folder.remove(str_buf.c_str(), instr == FS_INSTR_RMDIR);
                                connection_socket.send(FS_REPLY_OK);
                            } catch (except &e) {
                                switch (e.error_code()) {
                                    case ERROR_FS_HANDLE_BUSY:
                                    case ERROR_FS_NAME_NOT_EXIST:
                                        connection_socket.send(FS_REPLY_REJECT);
                                        break;
                                    default:
                                        throw;
                                }
                            }
                        }
                        break;

                        case FS_INSTR_FILE_CAT: {
                            connection_socket.recv_str(str_buf);
                            try {
                                fs_file_handle file = current_folder.open(str_buf.c_str());
                                data_buf = file.read_all();
                                connection_socket.send(FS_REPLY_OK);
                                connection_socket.send_str(data_buf);
                            } catch (except &e) {
                                switch (e.error_code()) {
                                    case ERROR_FS_NAME_NOT_EXIST:
                                        connection_socket.send(FS_REPLY_REJECT);
                                        break;
                                    default:
                                        throw;
                                }
                            }
                        }
                        break;

                        case FS_INSTR_FILE_W: {
                            connection_socket.recv_str(str_buf);
                            connection_socket.recv_str(data_buf);
                            try {
                                fs_file_handle file = current_folder.open(str_buf.c_str());
                                file.write_all(data_buf.c_str());
                                connection_socket.send(FS_REPLY_OK);
                            } catch (except &e) {
                                switch (e.error_code()) {
                                    case ERROR_FS_NAME_NOT_EXIST:
                                    case ERROR_FS_FULL_NODE:
                                        connection_socket.send(FS_REPLY_REJECT);
                                        break;
                                    default:
                                        throw;
                                }
                            }
                        }
                        break;

                        case FS_INSTR_FILE_I:
                            // todo later
                            break;

                        case FS_INSTR_FILE_D:
                            // todo later
                            break;

                        case FS_INSTR_FORMAT: {
                            fs_.format();
                            connection_socket.send(FS_REPLY_OK);
                        }
                        break;

                        default:
                            connection_socket.send(FS_REPLY_REJECT);
                            break;
                    }
                } catch (except &e) {
                    if (e.error_code() == ERROR_SOCKET_CLOSED_BY_REMOTE)
                        return;
                    throw;
                }
            }
        }

        void accept_connections() {
            while (threads_sig_term_.load(std::memory_order_acquire) == false) {
                server_connection_socket_handle connection_socket = server_socket_.accept();
                threads_.emplace_back(&fs_server::serve_client, this, std::move(connection_socket));
            }
        }

    private:
        file_system &fs_;
        server_socket_handle server_socket_;
        std::atomic<bool> threads_sig_term_;
        std::vector<std::thread> threads_;
    };
}

#endif
