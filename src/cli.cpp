#include "core/core.hpp"
#include <iostream>
int main(int argc, char **argv) {
    try {
        if (argc >= 3 && std::string(argv[1]) == "--evidence") {
            int count = argc > 3 ? std::stoi(argv[3]) : 1000;
            int mesh = argc > 4 ? std::stoi(argv[4]) : 4000;
            tb::export_evidence(argv[2], count, mesh);
            std::cout << "Evidence exported.\n";
        } else {
            std::cout << tb::system_info()
                      << "Usage: workbench_cli --evidence NEW_FOLDER [cases] [elements]\n";
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
