#include <iostream>

#include "../src/disk_client.h"
#include "../src/fs.h"
#include "../src/fs_server.h"
#include "../src/utils/misc.h"

int main(int argc, char *argv[]) {

    if (argc != 3 || !cs2313::is_uint(argv[1]) || !cs2313::is_uint(argv[2])) {
        std::cout << "Usage: fs disk_port port \n";
        return 1;
    }

    uint64_t disk_port = std::stoull(argv[1]),
             fs_port = std::stoull(argv[2]);
    if (fs_port < 1000 || fs_port > 65535) {
        std::cout << "Invalid port: " << fs_port << " (expected 1000 - 65535)\n";
        return 1;
    }
    if (disk_port < 1000 || disk_port > 65535) {
        std::cout << "Invalid port: " << disk_port << " (expected 1000 - 65535)\n";
        return 1;
    }

    try {

        {
            cs2313::disk_client client("127.0.0.1", static_cast<uint16_t>(disk_port));
            client.start_handler();
            cs2313::file_system fs(client);
            cs2313::fs_server server(fs, fs_port);
            server.accept_connections();
        }

    } catch (cs2313::except &e) {
        std::cout << "[ERROR] " << e;
    }

    return 0;
}
