#ifndef FS_SHELL_H
#define FS_SHELL_H

#include <iostream>
#include <sstream>
#include <string>

#include "fs_server.h"
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
        if (reply == FS_REPLY_INITIALIZED_NO_FORMAT) {
            client_socket_.send(FS_INSTR_FORMAT);
            client_socket_.recv(reply);
            std::cout << "Disk formatted on the first run." << std::endl;
        }

        bool exit_flag = false;
        while (!exit_flag) {
            std::cout << styled("file-system", STYLE_GREEN, STYLE_BOLD) << ":" << styled(path_prompt_, STYLE_BLUE, STYLE_BOLD) << "$ ";
            std::string input_line;
            std::getline(std::cin, input_line);
            if (input_line.empty()) continue;

            std::istringstream iss(input_line);
            std::string cmd;
            iss >> cmd;

            if (cmd == "e") {
                exit_flag = true;
            }
            else if (cmd == "f") {
                client_socket_.send(FS_INSTR_FORMAT);
                client_socket_.recv(reply);
                if (reply != FS_REPLY_OK) {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "mk") {
                std::string filename;
                if (!(iss >> filename)) {
                    std::cout << "Missing file name." << std::endl;
                    continue;
                }
                client_socket_.send(FS_INSTR_MK);
                client_socket_.send_str(filename);
                client_socket_.recv(reply);
                if (reply != FS_REPLY_OK) {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "mkdir") {
                std::string dirname;
                if (!(iss >> dirname)) {
                    std::cout << "Missing directory name." << std::endl;
                    continue;
                }
                client_socket_.send(FS_INSTR_MKDIR);
                client_socket_.send_str(dirname);
                client_socket_.recv(reply);
                if (reply != FS_REPLY_OK) {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "rm") {
                std::string filename;
                if (!(iss >> filename)) {
                    std::cout << "Missing file name." << std::endl;
                    continue;
                }
                client_socket_.send(FS_INSTR_RM);
                client_socket_.send_str(filename);
                client_socket_.recv(reply);
                if (reply != FS_REPLY_OK) {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "cd") {
                std::string path;
                if (!(iss >> path)) {
                    std::cout << "Missing path." << std::endl;
                    continue;
                }
                client_socket_.send(FS_INSTR_CD);
                client_socket_.send_str(path);
                client_socket_.recv(reply);
                if (reply == FS_REPLY_OK) {
                    client_socket_.recv_str(path_prompt_);
                } else {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "rmdir") {
                std::string dirname;
                if (!(iss >> dirname)) {
                    std::cout << "Missing directory name." << std::endl;
                    continue;
                }
                client_socket_.send(FS_INSTR_RMDIR);
                client_socket_.send_str(dirname);
                client_socket_.recv(reply);
                if (reply != FS_REPLY_OK) {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "ls") {
                client_socket_.send(FS_INSTR_LS);
                client_socket_.recv(reply);
                if (reply == FS_REPLY_OK) {
                    uint32_t count;
                    client_socket_.recv(count);
                    for (uint32_t i = 0; i < count; ++i) {
                        std::string entry;
                        client_socket_.recv_str(entry);
                        std::cout << entry << std::endl;
                    }
                } else {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "cat") {
                std::string filename;
                if (!(iss >> filename)) {
                    std::cout << "Missing file name." << std::endl;
                    continue;
                }
                client_socket_.send(FS_INSTR_FILE_CAT);
                client_socket_.send_str(filename);
                client_socket_.recv(reply);
                if (reply == FS_REPLY_OK) {
                    std::string data;
                    client_socket_.recv_str(data);
                    std::cout << data << std::endl;
                } else {
                    std::cout << "failed" << std::endl;
                }
            }
            else if (cmd == "w") {
                std::string filename, data;
                if (!(iss >> filename) || !std::getline(iss >> std::ws, data)) {
                    std::cout << "Invalid command format" << std::endl;
                    continue;
                }
                client_socket_.send(FS_INSTR_FILE_W);
                client_socket_.send_str(filename);
                client_socket_.send_str(data);
                client_socket_.recv(reply);
                if (reply != FS_REPLY_OK) {
                    std::cout << "failed" << std::endl;
                }
            }
            else {
                std::cout << "Unknown command" << std::endl;
            }
        }
    }

    private:
        client_socket_handle client_socket_;
        std::string path_prompt_;
    };
}

#endif
