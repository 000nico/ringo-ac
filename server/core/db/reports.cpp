#include "reports.hpp"
#include "db.hpp"
#include <ctime>

namespace db {

void save_environment_report(const ENVIRONMENT_REPORT& r) {
    SQLite::Statement stmt(*g_db, R"(
        INSERT INTO environment_reports
            (hwid, timestamp, debugger_present, hypervisor_detected,
             secure_boot_enabled, hvci_enabled, test_signing_enabled, vbs_enabled)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    stmt.bind(1, r.hwid);
    stmt.bind(2, (int64_t)time(nullptr));
    stmt.bind(3, (int)r.kernel_debugging.nt_query_enabled.value);
    stmt.bind(4, (int)r.hypervisor.nt_query.value);
    stmt.bind(5, (int)r.secure_boot.nt_query_enabled.value);
    stmt.bind(6, (int)r.hvci.code_integrity.value);
    stmt.bind(7, (int)r.test_signing.code_integrity.value);
    stmt.bind(8, (int)r.vbs.nt_query_secure_kernel.value);
    stmt.exec();
}

void save_module_report(const std::string& hwid, uint64_t timestamp, const std::string& payload) {
    SQLite::Statement stmt(*g_db, R"(
        INSERT INTO module_reports (hwid, timestamp, payload)
        VALUES (?, ?, ?)
    )");
    stmt.bind(1, hwid);
    stmt.bind(2, (int64_t)timestamp);
    stmt.bind(3, payload);
    stmt.exec();
}

} // namespace db
