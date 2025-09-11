// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize any global resources here if needed
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_THREAD_ATTACH:
        // Thread-specific initialization (disabled above)
        break;
    case DLL_THREAD_DETACH:
        // Thread-specific cleanup (disabled above)
        break;
    case DLL_PROCESS_DETACH:
        // Cleanup global resources
        // Note: The C# application should call ZD_Finalize() before unloading
        break;
    }
    return TRUE;
}