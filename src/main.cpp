//
// Created by leon on 4/18/23.
//

#include "server/webserver.h"

int main(int argc, char *argv[]) {

    WebServer webserver(argc, argv);

    webserver.run();

    return 0;
}