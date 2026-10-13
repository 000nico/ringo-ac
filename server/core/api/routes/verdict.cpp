#include "verdict.hpp"
#include <nlohmann/json.hpp>
#include "core/db/verdict.hpp"

using json = nlohmann::json;

void handle_get_status(const httplib::Request& req, httplib::Response& res) {
    std::string hwid = req.path_params.at("hwid");

    if (hwid.empty()) {
        res.status = 400;
        res.set_content("{\"error\":\"missing hwid\"}", "application/json");
        return;
    }

    PLAYER_RISK p = db::get_or_create(hwid);

    const char* status_str = "clean";
    if (p.status == VERDICT_STATUS::BANNED)       status_str = "banned";
    else if (p.status == VERDICT_STATUS::FLAGGED) status_str = "flagged";

    json response = {
        { "hwid",       p.hwid },
        { "status",     status_str },
        { "reason",     p.ban_reason },
        { "risk_score", p.risk_score }
    };

    res.status = 200;
    res.set_content(response.dump(), "application/json");
}
