#pragma once
#include<ntifs.h>

#ifndef _LOADED_MODULE_DEFINED
#define _LOADED_MODULE_DEFINED
typedef struct _LOADED_MODULE {
    WCHAR NameBuffer[256];
    UNICODE_STRING FullImageName;
    BOOLEAN isSigned;
    struct {
        struct {
            PVOID ImageBase;
            SIZE_T ImageSize;
        } Image;
    } Mem;
} LOADED_MODULE;
#endif

VOID SetLoadImageCallback();

// TODO
/* usar algo asi para buscar si la ruta es sospechosa
BOOLEAN EsRutaSospechosa(_In_ PUNICODE_STRING FullImageName) {
	// rutas típicas de cheats / inyectores
	UNICODE_STRING temp  = RTL_CONSTANT_STRING(L"\\Temp\\");
	UNICODE_STRING appdt = RTL_CONSTANT_STRING(L"\\AppData\\Local\\Temp\\");
	UNICODE_STRING down  = RTL_CONSTANT_STRING(L"\\Downloads\\");

	if (RtlFindUnicodeSubstring(FullImageName, &temp,  TRUE)) return TRUE;
	if (RtlFindUnicodeSubstring(FullImageName, &appdt, TRUE)) return TRUE;
	if (RtlFindUnicodeSubstring(FullImageName, &down,  TRUE)) return TRUE;

	return FALSE;
}
*/