// src/api/server.cpp
#include "server.hpp"
#include "routes/telemetry.hpp"
#include "routes/verdict.hpp"
#include <iostream>

RingoServer::RingoServer(const std::string& host, int port)
    : _host(host), _port(port) {}

void RingoServer::register_routes() {
    // telemetry sent by usermode
    _svr.Post("/telemetry/environment", handle_environment_report);
    
    // the game server consults this
    _svr.Get("/status/:hwid",           handle_get_status);

    // health check, heartbeat
    _svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // 404
    _svr.set_error_handler([](const httplib::Request&, httplib::Response& res) {
        res.status = 404;
        res.set_content("{\"error\":\"not found\"}", "application/json");
    });
}

void RingoServer::start() {
    register_routes();
    std::cout << "[ringo] listening on " << _host << ":" << _port << "\n";
    _svr.listen(_host.c_str(), _port);
}