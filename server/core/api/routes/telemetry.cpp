#include "telemetry.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include "../../models/reports.hpp"
#include "../../db/reports.hpp" 
#include "core/db/db.hpp"
#include "core/analysis/engine.hpp"
#include <iostream>

using json = nlohmann::json;

void handle_environment_report(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);
        ENVIRONMENT_REPORT report = j.get<ENVIRONMENT_REPORT>();

        db::save_environment_report(report); 
        engine::analyze_environment(report);

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
        auto report = std::make_unique<MODULE_REPORT>();
        j.get_to(*report);

        db::save_module_report(report->hwid, report->timestamp, req.body);
        engine::analyze_modules(*report);

        res.status = 200;
        res.set_content("{\"status\":\"ok\"}", "application/json");
    }
    catch (const std::exception& e) {
        std::cerr << "[telemetry/module] " << e.what() << "\n";
        res.status = 400;
        res.set_content("{\"error\":\"invalid payload\"}", "application/json");
    }
}