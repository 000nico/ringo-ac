#pragma once
#include "third_party/httplib/httplib.h"
#include <string>

class RingoServer {
public:
    RingoServer(const std::string& host, int port);
    void start();

private:
    httplib::Server _svr;
    std::string     _host;
    int             _port;

    void register_routes();
};