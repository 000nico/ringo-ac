#include "driver.hpp"
#include "../../km/core/communication/codes.hpp"
#include <iostream>
#include <string>

namespace {

    constexpr wchar_t kServiceName[] = L"RingoDriver";
    constexpr wchar_t kDriverFileName[] = L"rngdrv.sys";
    
    std::wstring get_driver_path() {
        wchar_t modulePath[MAX_PATH] = {};
        DWORD length = GetModuleFileNameW(nullptr, modulePath, ARRAYSIZE(modulePath));
        if (length == 0 || length == ARRAYSIZE(modulePath))
            return {};
    
        std::wstring path(modulePath, length);
        const std::wstring::size_type separator = path.find_last_of(L"\\/");
        if (separator == std::wstring::npos)
            return {};
    
        return path.substr(0, separator + 1) + kDriverFileName;
    }
    
    bool remove_existing_service(SC_HANDLE manager) {
        SC_HANDLE service = OpenServiceW(manager, kServiceName,SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS);
        
        if (service == nullptr)
            return GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST;
    
        SERVICE_STATUS_PROCESS status = {};
        DWORD bytesNeeded = 0;
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status),sizeof(status), &bytesNeeded)) {
            CloseServiceHandle(service);
            return false;
        }
    
        if (status.dwCurrentState != SERVICE_STOPPED) {
            SERVICE_STATUS stopStatus = {};
            if (!ControlService(service, SERVICE_CONTROL_STOP, &stopStatus) &&
                GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
                CloseServiceHandle(service);
                return false;
            }
    
            for (int attempt = 0; attempt < 50; ++attempt) {
                Sleep(100);
                if (!QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytesNeeded )) {
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
        const std::wstring driverPath = get_driver_path();
        if (driverPath.empty()) {
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
            kServiceName,
            kServiceName,
            SERVICE_START | SERVICE_QUERY_STATUS,
            SERVICE_KERNEL_DRIVER,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            driverPath.c_str(),
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
    
        const BOOL startResult = StartServiceW(service, 0, nullptr);
        const DWORD startError = startResult ? ERROR_SUCCESS : GetLastError();
        const bool started = startResult || startError == ERROR_SERVICE_ALREADY_RUNNING;
        if (!started)
            std::cerr << "StartService failed, error: " << startError << "\n";
    
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return started;
    }

} // namespace

BOOLEAN open_driver(HANDLE* pDriverHandle) {
    if (pDriverHandle == nullptr || !start_driver())
        return false;

    *pDriverHandle = CreateFile(
        DEVICE_NAME_UM,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (*pDriverHandle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        printf("CreateFile failed, error: %lu\n", err);
        return false;
    }
    
    return true;
}