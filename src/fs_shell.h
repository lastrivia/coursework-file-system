#ifndef FS_SHELL_H
#define FS_SHELL_H

#include <iostream>
#include <sstream>
#include <string>

#include "fs_protocol.h"
#include "utils/shell_style.h"
#include "utils/socket.h"

namespace cs2313 {

    class fs_shell {
    public:
        fs_shell(uint16_t server_port):
            client_socket_("127.0.0.1", server_port),
            path_prompt_("/") {}

        void run() {
            fs_reply reply;
            client_socket_.recv(reply);
            if (reply == FS_CONNECTED_REPLY_NO_FORMAT) {
                client_socket_.send(FS_INSTR_FORMAT);
                client_socket_.recv(reply);
                std::cout << "Disk formatted on the first run" << std::endl;
            }

            bool exit_flag = false;
            while (!exit_flag) {
                std::cout << styled("file-system", STYLE_GREEN, STYLE_BOLD) << ":" << styled(path_prompt_, STYLE_BLUE, STYLE_BOLD) << "$ ";
                std::string input_line;
                std::getline(std::cin, input_line);
                if (input_line.empty()) continue;

                std::istringstream iss(input_line);
                std::string cmd;
                std::string name;
                iss >> cmd;

                if (cmd == "e") {
                    exit_flag = true;
                } else if (cmd == "f") {
                    client_socket_.send(FS_INSTR_FORMAT);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, false) << std::endl;
                    }
                } else if (cmd == "mk") {
                    if (!(iss >> name)) {
                        std::cout << "Missing file name" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_MK);
                    client_socket_.send_str(name);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, false, name) << std::endl;
                    }
                } else if (cmd == "mkdir") {
                    if (!(iss >> name)) {
                        std::cout << "Missing directory name" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_MKDIR);
                    client_socket_.send_str(name);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, true, name) << std::endl;
                    }
                } else if (cmd == "rm") {
                    if (!(iss >> name)) {
                        std::cout << "Missing file name" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_RM);
                    client_socket_.send_str(name);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, false, name) << std::endl;
                    }
                } else if (cmd == "cd") {
                    if (!(iss >> name)) {
                        std::cout << "Missing path" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_CD);
                    client_socket_.send_str(name);
                    client_socket_.recv(reply);
                    if (reply == FS_REPLY_OK) {
                        client_socket_.recv_str(path_prompt_);
                    } else {
                        std::cout << fail_prompt(reply, true, name) << std::endl;
                    }
                } else if (cmd == "rmdir") {
                    if (!(iss >> name)) {
                        std::cout << "Missing directory name" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_RMDIR);
                    client_socket_.send_str(name);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, true, name) << std::endl;
                    }
                } else if (cmd == "ls") {
                    client_socket_.send(FS_INSTR_LS);
                    client_socket_.recv(reply);
                    if (reply == FS_REPLY_OK) {
                        uint32_t count;
                        client_socket_.recv(count);
                        for (uint32_t i = 0; i < count; ++i) {
                            std::string entry;
                            client_socket_.recv_str(entry);
                            if (entry.back() == '/') {
                                entry.pop_back();
                                std::cout << styled(entry, STYLE_BLUE, STYLE_BOLD);
                            } else
                                std::cout << entry;

                            if (i % 4 == 3 || i == count - 1)
                                std::cout << std::endl;
                            else
                                std::cout << '\t';
                        }
                    } else {
                        std::cout << fail_prompt(reply, true) << std::endl;
                    }
                } else if (cmd == "cat") {
                    if (!(iss >> name)) {
                        std::cout << "Missing file name" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_FILE_CAT);
                    client_socket_.send_str(name);
                    client_socket_.recv(reply);
                    if (reply == FS_REPLY_OK) {
                        std::string data;
                        client_socket_.recv_str(data);
                        std::cout << data << std::endl;
                    } else {
                        std::cout << fail_prompt(reply, false, name) << std::endl;
                    }
                } else if (cmd == "w") {
                    std::string data;
                    if (!(iss >> name) || !std::getline(iss >> std::ws, data)) {
                        std::cout << "Invalid command format" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_FILE_W);
                    client_socket_.send_str(name);
                    client_socket_.send_str(data);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, false, name) << std::endl;
                    }
                }  else if (cmd == "i") {
                    std::string data;
                    uint64_t pos;
                    if (!(iss >> name >> pos) || !std::getline(iss >> std::ws, data)) {
                        std::cout << "Invalid command format" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_FILE_I);
                    client_socket_.send_str(name);
                    client_socket_.send(pos);
                    client_socket_.send_str(data);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, false, name) << std::endl;
                    }
                }  else if (cmd == "d") {
                    uint64_t pos, len;
                    if (!(iss >> name >> pos >> len)) {
                        std::cout << "Invalid command format" << std::endl;
                        continue;
                    }
                    client_socket_.send(FS_INSTR_FILE_D);
                    client_socket_.send_str(name);
                    client_socket_.send(pos);
                    client_socket_.send(len);
                    client_socket_.recv(reply);
                    if (reply != FS_REPLY_OK) {
                        std::cout << fail_prompt(reply, false, name) << std::endl;
                    }
                } else {
                    std::cout << "Unknown command: " << cmd << std::endl;
                }
            }
        }

    private:
        client_socket_handle client_socket_;
        std::string path_prompt_;

        static std::string fail_prompt(fs_reply reply, bool is_folder_instr, const std::string &name = "") {
            std::string str = styled("Failed", STYLE_RED, STYLE_BOLD) + ": ";
            switch (reply) {
                case FS_REPLY_NOT_EXIST:
                    str += is_folder_instr ? "directory " : "file ";
                    str += name;
                    str += " not found";
                    break;
                case FS_REPLY_ALREADY_EXIST:
                    str += is_folder_instr ? "directory " : "file ";
                    str += name;
                    str += " already exists";
                    break;
                case FS_REPLY_NAME_TOO_LONG:
                    str += "the name is too long";
                    break;
                case FS_REPLY_NAME_INVALID:
                    str += "invalid name";
                    break;
                case FS_REPLY_BUSY_HANDLE:
                    str += "the ";
                    str += is_folder_instr ? "directory " : "file ";
                    str += "is in use";
                    break;
                case FS_REPLY_CAPACITY_EXCEEDED:
                    str += "capacity exceeded";
                    break;
                case FS_REPLY_UNKNOWN_ERROR:
                default:
                    str += "unknown error";
                    break;
            }
            return str;
        }
    };
}

#endif
