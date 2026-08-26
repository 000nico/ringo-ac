#include "driver.hpp"
#include "../../km/core/comunication/ioctl/codes.hpp"

BOOLEAN open_driver(HANDLE* pDriverHandle) {
    *pDriverHandle = CreateFile(
        DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,               
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (*pDriverHandle == INVALID_HANDLE_VALUE) return false;
    else return true;
}