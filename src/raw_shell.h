#ifndef RAW_SHELL_H
#define RAW_SHELL_H

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include "disk_interface.h"

namespace cs2313 {

    class raw_shell {
    public:
        explicit raw_shell(disk_interface &disk) :
            disk_(disk) {

            disk_description description = disk_.get_description();
            cylinders_ = description.cylinders;
            sectors_per_cylinder_ = description.sectors_per_cylinder;
            bytes_per_sector_ = description.bytes_per_sector;

            buf_ = new char[bytes_per_sector_ + 1];
            memset(buf_, 0, bytes_per_sector_ + 1);
        }

        ~raw_shell() {
            delete[] buf_;
        }

        void run() {

            std::string line;
            while (std::getline(std::cin, line)) {
                try {
                    std::istringstream iss(line);
                    std::string cmd;
                    iss >> cmd;

                    if (cmd == "I") {
                        std::cout << cylinders_ << " " << sectors_per_cylinder_ << std::endl;

                    } else if (cmd == "R") {
                        uint64_t c, s;
                        if (iss >> c >> s) {
                            if (c < cylinders_ && s < sectors_per_cylinder_) {
                                uint64_t addr = c * sectors_per_cylinder_ + s;
                                disk_.read(addr, buf_);

                                std::cout << "Yes ";
                                std::cout.write(buf_, bytes_per_sector_);
                                std::cout << std::endl;
                            } else {
                                std::cout << "No" << std::endl;
                            }
                        } else {
                            std::cout << "No" << std::endl;
                        }

                    } else if (cmd == "W") {
                        uint64_t c, s;
                        if (iss >> c >> s) {
                            std::string data;
                            std::getline(iss, data);
                            data.erase(0, data.find_first_not_of(" \t\r\n"));

                            if (c < cylinders_ && s < sectors_per_cylinder_) {
                                uint64_t addr = c * sectors_per_cylinder_ + s;
                                memset(buf_, 0, bytes_per_sector_ + 1);
                                memcpy(buf_, data.data(), std::min(bytes_per_sector_, data.size()));
                                disk_.write(addr, buf_);
                                std::cout << "Yes" << std::endl;
                            } else {
                                std::cout << "No" << std::endl;
                            }
                        } else {
                            std::cout << "No" << std::endl;
                        }

                    } else if (cmd == "E") {
                        disk_.shutdown();
                        std::cout << "Goodbye!" << std::endl;
                        break;

                    } else {
                        std::cout << "Invalid command" << std::endl;
                    }

                } catch (except &e) {
                    std::cout << "[ERROR] " << e;
                }
            }
        }

    private:
        disk_interface &disk_;
        uint64_t cylinders_, sectors_per_cylinder_, bytes_per_sector_;
        char *buf_;
    };
}

#endif
