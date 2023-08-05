//
// Created by leon on 4/18/23.
//

#include <iostream>

#include "server/webserver.h"

int main(int argc, char *argv[]) {

    WebServer webserver;

    // 处理命令行参数
    if (argc > 1) {
        try {
            webserver.parse_args(argc, argv);
        } catch (const std::invalid_argument &e) {
            std::cerr << "unknown args" << std::endl;
            std::cerr << WebServer::help(argv[0]) << std::endl;
            return 1;
        }
    }

    webserver.run();

    return 0;
}