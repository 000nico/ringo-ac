#include "driver.hpp"
#include "../../../driver/core/communication/codes.hpp"
#include <iostream>
#include <string>

namespace {

    constexpr wchar_t K_SERVICE_NAME[] = L"RingoDriver";
    constexpr wchar_t K_DRIVER_FILE_NAME[] = L"rngdrv.sys";
    
    std::wstring get_driver_path() {
        wchar_t module_path[MAX_PATH] = {};
        DWORD length = GetModuleFileNameW(nullptr, module_path, ARRAYSIZE(module_path));
        if (length == 0 || length == ARRAYSIZE(module_path))
            return {};
    
        std::wstring path(module_path, length);
        const std::wstring::size_type separator = path.find_last_of(L"\\/");
        if (separator == std::wstring::npos)
            return {};
    
        return path.substr(0, separator + 1) + K_DRIVER_FILE_NAME;
    }
    
    bool remove_existing_service(SC_HANDLE manager) {
        SC_HANDLE service = OpenServiceW(manager, K_SERVICE_NAME,SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS);
        
        if (service == nullptr)
            return GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST;
    
        SERVICE_STATUS_PROCESS status = {};
        DWORD bytes_needed = 0;
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status),sizeof(status), &bytes_needed)) {
            CloseServiceHandle(service);
            return false;
        }
    
        if (status.dwCurrentState != SERVICE_STOPPED) {
            SERVICE_STATUS stop_status = {};
            if (!ControlService(service, SERVICE_CONTROL_STOP, &stop_status) &&
                GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
                CloseServiceHandle(service);
                return false;
            }
    
            for (int attempt = 0; attempt < 50; ++attempt) {
                Sleep(100);
                if (!QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytes_needed )) {
                    CloseServiceHandle(service);
                    return false;
                }
                
                if (status.dwCurrentState == SERVICE_STOPPED)
                    break;
            }
    
            if (status.dwCurrentState != SERVICE_STOPPED) {
                CloseServiceHandle(service);
                return false;
            }
        }
    
        const bool removed = DeleteService(service) != FALSE;
        CloseServiceHandle(service);
        return removed;
    }
    
    bool start_driver() {
        const std::wstring driver_path = get_driver_path();
        if (driver_path.empty()) {
            std::cerr << "Could not resolve the driver path\n";
            return false;
        }
    
        SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (manager == nullptr) {
            std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
            return false;
        }
    
        if (!remove_existing_service(manager)) {
            std::cerr << "Could not remove the existing driver service\n";
            CloseServiceHandle(manager);
            return false;
        }
    
        SC_HANDLE service = CreateServiceW(
            manager,
            K_SERVICE_NAME,
            K_SERVICE_NAME,
            SERVICE_START | SERVICE_QUERY_STATUS,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            driver_path.c_str(),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        );
        
        if (service == nullptr) {
            std::cerr << "CreateService failed, error: " << GetLastError() << "\n";
            CloseServiceHandle(manager);
            return false;
        }
    
        const BOOL start_result = StartServiceW(service, 0, nullptr);
        const DWORD start_error = start_result ? ERROR_SUCCESS : GetLastError();
        const bool started = start_result || start_error == ERROR_SERVICE_ALREADY_RUNNING;
        if (!started)
            std::cerr << "StartService failed, error: " << start_error << "\n";
    
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return started;
    }

} // namespace

BOOLEAN open_driver(HANDLE* p_driver_handle) {
    if (p_driver_handle == nullptr || !start_driver())
        return false;

    *p_driver_handle = CreateFile(
        DEVICE_NAME_UM,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (*p_driver_handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        printf("CreateFile failed, error: %lu\n", err);
        return false;
    }
    
    return true;
}