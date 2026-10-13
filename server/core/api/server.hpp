#pragma once
#include "third_party/httplib/httplib.h"
#include <string>

class RINGO_SERVER {
public:
    RINGO_SERVER(const std::string& host, int port);
    void start();

private:
    httplib::Server _svr;
    std::string     _host;
    int             _port;

    void register_routes();
};