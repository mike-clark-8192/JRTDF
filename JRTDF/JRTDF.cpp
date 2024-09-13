#include <iostream>
#include <string>
#include <windows.h>

#define JRTDFSTRLEN 64

#ifdef _DEBUG
#define DEBUG_ 1
#else
#undef DEBUG_
#endif

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);

struct ButtonData {
    std::wstring targetButtonLabel = L"&Yes";
    HWND foundButtonHandle = NULL;
};

void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    UNREFERENCED_PARAMETER(dwmsEventTime);
    UNREFERENCED_PARAMETER(dwEventThread);
    UNREFERENCED_PARAMETER(idChild);
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(hook);

    if (idObject != OBJID_WINDOW)
        return;

    wchar_t windowClass[JRTDFSTRLEN];
    wchar_t windowTitle[JRTDFSTRLEN];
    if (GetClassNameW(hwnd, windowClass, JRTDFSTRLEN) && wcscmp(windowClass, L"#32770") == 0) {
#ifdef DEBUG_
        std::wcout << L"Found a dialog window." << std::endl;
#endif
        if (GetWindowTextW(hwnd, windowTitle, JRTDFSTRLEN)) {
#ifdef DEBUG_
            std::wcout << L"Window title: " << windowTitle << std::endl;
#endif
            if (wcscmp(windowTitle, L"Rename") == 0) {
                ButtonData data;
                EnumChildWindows(hwnd, EnumChildProc, (LPARAM)&data);

                if (data.foundButtonHandle) {
#ifdef DEBUG_
                    std::wcout << L"Found the '&Yes' button, sending click message." << std::endl;
#endif
                    SendMessage(data.foundButtonHandle, BM_CLICK, 0, 0);
                }
            }
        }
    }
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    ButtonData* pData = (ButtonData*)lParam;
    wchar_t className[JRTDFSTRLEN];
    wchar_t windowText[JRTDFSTRLEN];

    GetClassNameW(hwnd, className, JRTDFSTRLEN);
    GetWindowTextW(hwnd, windowText, JRTDFSTRLEN);

    if (wcscmp(className, L"Button") == 0 && wcscmp(windowText, pData->targetButtonLabel.c_str()) == 0) {
        pData->foundButtonHandle = hwnd;
        return FALSE;
    }

    return TRUE;
}

int main()
{
    DWORD explorerPID = 0;
    HWND explorerHWND = FindWindow(L"Progman", L"Program Manager");
    if (explorerHWND) {
        GetWindowThreadProcessId(explorerHWND, &explorerPID);
    }

    if (explorerPID == 0) {
        std::wcerr << L"explorer.exe not found." << std::endl;
        return 1;
    }

    HWINEVENTHOOK hook = SetWinEventHook(
        EVENT_SYSTEM_DIALOGSTART, EVENT_SYSTEM_DIALOGSTART,
        NULL,
        WinEventProc,
        explorerPID, 0,
        WINEVENT_OUTOFCONTEXT);

    if (!hook) {
        std::wcerr << L"Failed to set event hook!" << std::endl;
        return 1;
    }

    std::wcout << L"Hooked EVENT_SYSTEM_DIALOGSTART in explorer.exe..." << std::endl;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWinEvent(hook);
    return 0;
}