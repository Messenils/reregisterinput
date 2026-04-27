// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "thread"
#include "vector"

bool init = false;
bool hooked = false;
HMODULE protohModule;

#ifdef _WIN64
#define PROTOINPUTHOOKS_DLL_NAME "ProtoInputHooks64.dll"
#else   
#define PROTOINPUTHOOKS_DLL_NAME "ProtoInputHooks32.dll"
#endif

void UnregisterMouseAndKeyboard()
{
    RAWINPUTDEVICE devs[2];

    // Mouse
    devs[0].usUsagePage = 0x01;   // HID_USAGE_PAGE_GENERIC
    devs[0].usUsage = 0x02;   // HID_USAGE_GENERIC_MOUSE
    devs[0].dwFlags = RIDEV_REMOVE;
    devs[0].hwndTarget = nullptr;   // Must be NULL when removing

    // Keyboard
    devs[1].usUsagePage = 0x01;   // HID_USAGE_PAGE_GENERIC
    devs[1].usUsage = 0x06;   // HID_USAGE_GENERIC_KEYBOARD
    devs[1].dwFlags = RIDEV_REMOVE;
    devs[1].hwndTarget = nullptr;   // Must be NULL when removing

    if (!RegisterRawInputDevices(devs, 2, sizeof(RAWINPUTDEVICE)))
    {
        MessageBoxA(nullptr, "Failed to unregister raw input", "Error", MB_OK | MB_ICONERROR);
    }
}

void Registergameinput()
{
    RAWINPUTDEVICE devs[2];

    // Mouse
    devs[0].usUsagePage = 0x01;   // HID_USAGE_PAGE_GENERIC
    devs[0].usUsage = 0x02;   // HID_USAGE_GENERIC_MOUSE
    devs[0].dwFlags = RIDEV_INPUTSINK;
    devs[0].hwndTarget = NULL;

    // Keyboard
    devs[1].usUsagePage = 0x01;   // HID_USAGE_PAGE_GENERIC
    devs[1].usUsage = 0x06;   // HID_USAGE_GENERIC_KEYBOARD
    devs[1].dwFlags = RIDEV_INPUTSINK;
    devs[1].hwndTarget = NULL;

    if (!RegisterRawInputDevices(devs, 2, sizeof(RAWINPUTDEVICE)))
    {
        MessageBoxA(NULL, "Failed to register raw input", "Error", MB_OK | MB_ICONERROR);
    }
}

int tries = 0;
void ThreadFunction(HMODULE hModule)
{
    while (!hooked && tries <= 30)
    { 
		tries++;
        Sleep(1000);

        HMODULE proto = GetModuleHandleA(PROTOINPUTHOOKS_DLL_NAME);
        if (proto)
        {
            hooked = true;
        }
    }
    if (!hooked) {
        MessageBoxA(NULL, "Failed to find " PROTOINPUTHOOKS_DLL_NAME  " in process after 30 tries!", "Error", MB_OK | MB_ICONERROR);
        return;
	}

    Sleep(1000);
    UnregisterMouseAndKeyboard();
    Registergameinput();
    return;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        if (!init)
        {
            std::thread one(ThreadFunction, hModule);
            one.detach();
            init = true;
        }
        //CloseHandle(one);
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        exit(0);
        break;
    }
    }
    return true;
}
