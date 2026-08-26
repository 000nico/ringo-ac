#include "driver.hpp"

BOOLEAN open_driver(HANDLE* pDriverHandle) {
    *pDriverHandle = CreateFile(
        L"\\\\.\\ringo",
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