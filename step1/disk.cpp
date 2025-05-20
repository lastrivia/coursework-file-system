#include <iostream>

#include "../src/virtual_drive.h"
#include "../src/utils/misc.h"

int main(int argc, char *argv[]) {

    uint64_t cylinders = 0;
    uint64_t sectors_per_cylinder = 0;
    uint64_t bytes_per_sector = 256;
    uint64_t delay_us = 0;
    uint16_t port = 0;
    const char *filename = nullptr;

    using cs2313::is_uint;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" && i + 1 < argc && is_uint(argv[i + 1])) {
            ++i;
            cylinders = std::stoull(argv[i]);
        } else if (arg == "-s" && i + 1 < argc && is_uint(argv[i + 1])) {
            ++i;
            sectors_per_cylinder = std::stoull(argv[i]);
        } else if (arg == "-b" && i + 1 < argc && is_uint(argv[i + 1])) {
            ++i;
            bytes_per_sector = std::stoull(argv[i]);
        } else if (arg == "-d" && i + 1 < argc && is_uint(argv[i + 1])) {
            ++i;
            delay_us = std::stoull(argv[i]);
        } else if (arg == "-p" && i + 1 < argc && is_uint(argv[i + 1])) {
            ++i;
            uint64_t arg_port = std::stoull(argv[i]);
            if(arg_port < 1000 || arg_port > 65535) {
                std::cout << "Invalid port: " << arg_port << " (expected 1000 - 65535)\n";
                return 1;
            }
            port = static_cast<uint16_t>(arg_port);
        } else if (arg[0] != '-') {
            filename = argv[i];
        } else {
            std::cout << "Unknown or malformed argument: " << arg << "\n";
            return 1;
        }
    }

    if (cylinders == 0 || sectors_per_cylinder == 0 || port == 0 || !filename) {
        std::cout << "Usage: disk filename -c cylinders -s sectors_per_cylinder [-b bytes_per_sector=256] [-d delay_us=0] -p port \n";
        return 1;
    }

    try {
        cs2313::virtual_drive drive(cylinders, sectors_per_cylinder, bytes_per_sector, delay_us, filename, port);
        drive.start();
        drive.wait();

    } catch (cs2313::except &e) {
        std::cout << "[ERROR] " << e;
    }

    return 0;
}
