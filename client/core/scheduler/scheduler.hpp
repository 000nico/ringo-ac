#pragma once
#include <windows.h>

// Corre para siempre: cada ~2 minutos (con jitter) colecta el environment del
// driver y lo manda al server. Pensado para llamarse al final del main del cliente.
void run_scheduler(HANDLE h_driver);
