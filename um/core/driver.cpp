#include "driver.hpp"
#include "../../km/core/comunication/ioctl/codes.hpp"
#include<iostream>

BOOLEAN open_driver(HANDLE* pDriverHandle) {
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