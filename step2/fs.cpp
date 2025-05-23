#include <iostream>

#include "../src/io_protocol.h"
#include "../src/fs.h"
#include "../src/fs_server.h"
#include "../src/mem_storage.h"
#include "../src/utils/misc.h"

int main(int argc, char *argv[]) {

    if (argc != 2 || !cs2313::is_uint(argv[1])) {
        std::cout << "Usage: fs port \n";
        return 1;
    }

    uint64_t fs_port = std::stoull(argv[2]);
    if (fs_port < 1000 || fs_port > 65535) {
        std::cout << "Invalid port: " << fs_port << " (expected 1000 - 65535)\n";
        return 1;
    }

    try {

        {
            cs2313::mem_storage mem(1, 4096, 256);
            cs2313::file_system fs(mem);
            cs2313::fs_server server(fs, fs_port);
            server.accept_connections();
        }

    } catch (cs2313::except &e) {
        std::cout << "[ERROR] " << e;
    }

    return 0;
}
