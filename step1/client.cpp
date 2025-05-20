#include <iostream>

#include "../src/io_protocol.h"
#include "../src/raw_shell.h"
#include "../src/utils/misc.h"

int main(int argc, char *argv[]) {

    if (argc != 2 || !cs2313::is_uint(argv[1])) {
        std::cout << "Usage: client port \n";
        return 1;
    }

    uint64_t arg_port = std::stoull(argv[1]);
    if (arg_port < 1000 || arg_port > 65535) {
        std::cout << "Invalid port: " << arg_port << " (expected 1000 - 65535)\n";
        return 1;
    }

    try {

        {
            cs2313::disk_storage_client client("127.0.0.1", static_cast<uint16_t>(arg_port));
            client.start_handler();
            cs2313::raw_shell shell(client);
            shell.run();
        }

    } catch (cs2313::except &e) {
        std::cout << "[ERROR] " << e;
    }

    return 0;
}
