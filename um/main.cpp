#include <iostream>
#include "core/driver.hpp"

int main()
{
    HANDLE hDriver = NULL;
    if (open_driver(&hDriver))
        std::cout << "[+] Driver opened" << std::endl;
    else
        std::cout << "[-] Error opening driver" << std::endl;
}