#include "reports.hpp"
#include "db.hpp"
#include <nlohmann/json.hpp>
#include <ctime>

using json = nlohmann::json;

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

void save_module_report(const MODULE_REPORT& r) {
    // los vectores los serializamos como JSON string
    json unsigned_json = r.unsigned_modules;
    json suspicious_json = r.suspicious_module_names;

    SQLite::Statement stmt(*g_db, R"(
        INSERT INTO module_reports
            (hwid, timestamp, process_hash_mismatch, unsigned_modules, suspicious_modules)
        VALUES (?, ?, ?, ?, ?)
    )");
    stmt.bind(1, r.hwid);
    stmt.bind(2, (int64_t)r.timestamp);
    stmt.bind(3, (int)r.process_hash_mismatch);
    stmt.bind(4, unsigned_json.dump());
    stmt.bind(5, suspicious_json.dump());
    stmt.exec();
}

} // namespace db