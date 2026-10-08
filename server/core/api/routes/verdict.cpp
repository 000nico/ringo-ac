#include "verdict.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void handle_get_status(const httplib::Request& req, httplib::Response& res) {
    std::string hwid = req.path_params.at("hwid");

    if (hwid.empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"missing hwid\"}", "application/json");
        return;
    }

    json response = {
        { "hwid",   hwid },
        { "status", "clean" },
        { "reason", "" }
    };

    res.status = 200;
    res.set_content(response.dump(), "application/json");
}