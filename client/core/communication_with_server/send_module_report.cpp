#include <httplib.h>
#include <nlohmann/json.hpp>
#include <ctime>

#include "server_info.hpp"
#include "send_module_report.hpp"
#include "serialize.hpp"

#include "core/communication_with_driver/communication_with_driver.hpp"

using json = nlohmann::json;

bool collect_module_data(HANDLE drv, MODULE_DATA& out) {
    out = {};

    // se escribe directo en MODULE_DATA: no hace falta un buffer extra de ~350KB
    if (!get_reports(drv, &out.reports))
        return false;

    out.timestamp = (uint64_t)time(nullptr);

    HWID hwid = {};
    DWORD bytes_returned = 0;
    if (get_environment_information(drv, INFO_HWID, &hwid, sizeof(hwid), &bytes_returned))
        out.hwid = hwid_to_string(hwid);

    return true;
}

bool send_module_report(const MODULE_DATA& report) {
    httplib::Client cli(SERVER_HOST, SERVER_PORT);
    cli.set_connection_timeout(3);
    cli.set_read_timeout(5);

    json j = report;
    auto res = cli.Post("/telemetry/module", j.dump(), "application/json");

    return res && res->status == 200;
}
