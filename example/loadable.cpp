#include <Windows.h>

DWORD WINAPI hello()
{
    MessageBoxW(nullptr, L"hellooooo :3", L"hiiiiiii <3", 0);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        CreateThread(NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(hello), NULL, NULL, NULL);
        break;
    case DLL_PROCESS_DETACH:
        MessageBoxW(nullptr, L">__<", L"k, bye then", 0);
        break;
    }

    return true;
}