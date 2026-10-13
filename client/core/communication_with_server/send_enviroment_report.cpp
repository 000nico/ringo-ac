#include <httplib.h>
#include <nlohmann/json.hpp>
#include "server_info.hpp"
#include "send_environment_report.hpp"
#include "serialize.hpp"
#include "core/communication_with_driver/communication_with_driver.hpp"

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

using json = nlohmann::json;

template <typename T>
static bool query(HANDLE drv, ENVIRONMENT_INFO_CLASS cls, T& out) {
    DWORD returned = 0;
    return NT_SUCCESS(get_environment_information(drv, cls, &out, sizeof(out), &returned));
}

bool collect_environment(HANDLE drv, ENVIRONMENT_DATA& out) {
    out = {};

    return query(drv, INFO_SECURE_BOOT,           out.secure_boot)
        && query(drv, INFO_HVCI,                 out.hvci)
        && query(drv, INFO_VBS,                  out.vbs)
        && query(drv, INFO_HYPERVISOR_STATUS,     out.hypervisor)
        && query(drv, INFO_KERNEL_DEBUGGING_STATUS, out.kernel_debugging)
        && query(drv, INFO_TEST_SIGNING_MODE,      out.test_signing)
        && query(drv, INFO_TPM,                  out.tpm)
        && query(drv, INFO_HWID,                 out.hwid);
}

bool send_environment_report(const ENVIRONMENT_DATA& env) {
    httplib::Client cli(SERVER_HOST, SERVER_PORT);
    cli.set_connection_timeout(3);
    cli.set_read_timeout(5);

    json j = env;
    auto res = cli.Post("/telemetry/environment", j.dump(), "application/json");

    return res && res->status == 200;
}