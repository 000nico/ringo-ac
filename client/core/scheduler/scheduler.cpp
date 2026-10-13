#include "scheduler.hpp"

#include "../communication_with_server/send_environment_report.hpp"
#include "../communication_with_server/send_module_report.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>

#define REPORT_BASE_INTERVAL_MS 120000 // 2 minutos
#define REPORT_JITTER_MS        20000  // +/- 20 segundos

static DWORD next_interval_ms() {
    const int jitter = (rand() % (2 * REPORT_JITTER_MS + 1)) - REPORT_JITTER_MS;
    return (DWORD)(REPORT_BASE_INTERVAL_MS + jitter);
}

void run_scheduler(HANDLE h_driver) {
    srand((unsigned)time(nullptr));

    for (;;) {
        ENVIRONMENT_DATA environment = {};
        if (collect_environment(h_driver, environment) && send_environment_report(environment))
            std::cout << "[+] environment report sent\n";
        else
            std::cout << "[-] environment report failed\n";

        MODULE_DATA modules = {};
        if (collect_module_data(h_driver, modules)) {
            const DETECTION_REPORTS& reports = modules.reports;
            const bool has_detections = reports.byovd.count > 0
                || reports.manual_mapped.count > 0
                || reports.flagged_dll.count > 0
                || reports.threads.count > 0
                || reports.hash_diff.count > 0;

            // solo mandamos el reporte si el driver encontro algo
            if (has_detections) {
                if (send_module_report(modules))
                    std::cout << "[+] module report sent\n";
                else
                    std::cout << "[-] module report failed\n";
            }
        }

        Sleep(next_interval_ms());
    }
}
