// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "thread"
#include "vector"

bool init = false;
bool hooked = false;
HMODULE protohModule;

HWND hwnd = nullptr;

struct HandleData
{
    unsigned long pid;
    HWND hwnd;
};
BOOL IsMainWindow(HWND handle)
{
    // Is top level & visible & not one of ours
    if (GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle))
    {
        return TRUE;
    }
    else return FALSE;
}
BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
    HandleData& data = *(HandleData*)lParam;

    DWORD pid = 0;
    GetWindowThreadProcessId(handle, &pid);

    if (data.pid != pid || !IsMainWindow(handle))
        return TRUE; // Keep searching

    data.hwnd = handle;

    return FALSE;
}

void UpdateMainHwnd(bool logOutput)
{
    // Go through all the top level windows, select the first that's visible & belongs to the process
    HandleData data{ GetCurrentProcessId(), nullptr };

    EnumWindows(EnumWindowsCallback, (LPARAM)&data);

    const auto hwndd = (intptr_t)data.hwnd;

    if (logOutput)
    {
        if (hwndd == 0)
            MessageBoxA(NULL, "UpdateMainHwnd did not find a valid hwnd!", "Error", MB_OK | MB_ICONERROR);
    }
    if (data.hwnd != nullptr)
        hwnd = data.hwnd;
}

bool AnyRawInputForHwnd(HWND hwnd)
{
    UINT num = 0;
    GetRegisteredRawInputDevices(nullptr, &num, sizeof(RAWINPUTDEVICE));

    if (num == 0)
        return false;

    std::vector<RAWINPUTDEVICE> devs(num);
    if (GetRegisteredRawInputDevices(devs.data(), &num, sizeof(RAWINPUTDEVICE)) == (UINT)-1)
        return false;

    for (auto& d : devs)
    {
        if (d.hwndTarget == hwnd)
            return true;
    }

    return false;
}

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

void Registergameinput(HWND hwnd)
{
    RAWINPUTDEVICE devs[2];

    // Mouse
    devs[0].usUsagePage = 0x01;   // HID_USAGE_PAGE_GENERIC
    devs[0].usUsage = 0x02;   // HID_USAGE_GENERIC_MOUSE
    devs[0].dwFlags = RIDEV_INPUTSINK;
    devs[0].hwndTarget = hwnd;

    // Keyboard
    devs[1].usUsagePage = 0x01;   // HID_USAGE_PAGE_GENERIC
    devs[1].usUsage = 0x06;   // HID_USAGE_GENERIC_KEYBOARD
    devs[1].dwFlags = RIDEV_INPUTSINK;
    devs[1].hwndTarget = hwnd;

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
#ifdef _WIN64
        HMODULE proto = GetModuleHandleA("ProtoInputHooks64.dll");
#else   
        HMODULE proto = GetModuleHandleA("ProtoInputHooks32.dll");
#endif
        if (proto)
        {
            hooked = true;
            tries = 0;
        }
    }
    tries = 0;
    if (!hooked) {
#ifdef _WIN64
        MessageBoxA(NULL, "Failed to find ProtoInputHooks64.dll in process after 30 tries!", "Error", MB_OK | MB_ICONERROR);
#else   MessageBoxA(NULL, "Failed to find ProtoInputHooks32.dll in process after 30 tries!", "Error", MB_OK | MB_ICONERROR);
#endif
        return;
	}

    Sleep(1000);
    UnregisterMouseAndKeyboard();
    Registergameinput(NULL);
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
