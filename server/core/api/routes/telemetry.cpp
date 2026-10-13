#include "telemetry.hpp"
#include <nlohmann/json.hpp>
#include "../../models/reports.hpp"
#include "../../db/reports.hpp" 
#include "core/db/db.hpp"
#include <iostream>

using json = nlohmann::json;

void handle_environment_report(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);
        ENVIRONMENT_REPORT report = j.get<ENVIRONMENT_REPORT>();

        db::save_environment_report(report); 
        // TODO: pass to engine

        res.status = 200;
        res.set_content("{\"status\":\"ok\"}", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid payload\"}", "application/json");
    }
}

void handle_module_report(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);
        MODULE_REPORT report = j.get<MODULE_REPORT>();

        db::save_module_report(report);
        // TODO: pass to the analysis engine

        res.status = 200;
        res.set_content("{\"status\":\"ok\"}", "application/json");
    }
    catch (const std::exception& e) {
        res.status = 400;
        res.set_content("{\"error\":\"invalid payload\"}", "application/json");
    }
}