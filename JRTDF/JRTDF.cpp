#include <iostream>
#include <string>
#include <windows.h>

#ifndef UNICODE
#error UNICODE is not defined
#endif

#ifdef _DEBUG
#define DEBUG_ 1
#else
#undef DEBUG_
#endif

#define MAXWINSTRLEN 100

void PrintWindowInfo(HWND hwnd)
{
    wchar_t windowTitle[MAXWINSTRLEN];
    wchar_t windowClass[MAXWINSTRLEN];
    if (GetWindowText(hwnd, windowTitle, MAXWINSTRLEN)) {
        std::wcout << L"Window title: " << windowTitle;
    }
    if (GetClassName(hwnd, windowClass, MAXWINSTRLEN)) {
        std::wcout << L" class: " << windowClass;
    }
    std::wcout << std::endl;
}

bool IsParentRenameDialogRecursivelyUpwards(HWND hwndYesButton)
{
    Sleep(10);
    HWND hwndParent = GetParent(hwndYesButton);
    if (!hwndParent) {
        return false;
    }
    wchar_t windowTitle[MAXWINSTRLEN];
    wchar_t windowClass[MAXWINSTRLEN];
    if (GetWindowText(hwndParent, windowTitle, MAXWINSTRLEN) && GetClassName(hwndParent, windowClass, MAXWINSTRLEN)) {
        if (wcscmp(windowTitle, L"Rename") == 0 && wcscmp(windowClass, L"#32770") == 0) {
#ifdef DEBUG_
            std::wcout << L"Found the 'Rename' dialog." << std::endl;
#endif
            return true;
        }
    }
    return IsParentRenameDialogRecursivelyUpwards(hwndParent);
}

bool IsYesButton(HWND hwnd)
{
    wchar_t windowTitle[MAXWINSTRLEN];
    wchar_t windowClass[MAXWINSTRLEN];
    if (GetWindowText(hwnd, windowTitle, MAXWINSTRLEN) && GetClassName(hwnd, windowClass, MAXWINSTRLEN)) {
        if (wcscmp(windowTitle, L"&Yes") == 0 && wcscmp(windowClass, L"Button") == 0) {
#ifdef DEBUG_
            std::wcout << L"Found the '&Yes' button." << std::endl;
#endif
            return true;
        }
    }
    return false;
}

void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) noexcept
{
    UNREFERENCED_PARAMETER(dwEventThread);
    UNREFERENCED_PARAMETER(dwmsEventTime);
    UNREFERENCED_PARAMETER(idChild);
    UNREFERENCED_PARAMETER(hook);
    UNREFERENCED_PARAMETER(event);
    try {

#ifdef DEBUG_
        std::wcout << L"hook: " << hook
                   << L", event: " << event
                   << L", hwnd: " << hwnd
                   << L", idObject: " << idObject
                   << L", idChild: " << idChild
                   << L", dwEventThread: " << dwEventThread
                   << L", dwmsEventTime: " << dwmsEventTime
                   << std::endl;
        PrintWindowInfo(hwnd);
#endif

        if (idObject != OBJID_WINDOW)
            return;
        Sleep(10);
        if (IsYesButton(hwnd)) {
            Sleep(10);
            if (IsParentRenameDialogRecursivelyUpwards(hwnd)) {
#ifdef DEBUG_
                std::wcout << L"Found the 'Yes' button in the 'Rename' dialog." << std::endl;
#endif
                SendMessage(hwnd, BM_CLICK, 0, 0);
            }
        }
    } catch (...) {
        std::wcerr << L"Exception caught in WinEventProc." << std::endl;
    }
}

int main()
{
    DWORD explorerPID = 0;
    HWND explorerHWND = FindWindow(L"Progman", L"Program Manager");
    if (explorerHWND) {
        GetWindowThreadProcessId(explorerHWND, &explorerPID); // Get the process ID
    } else {
        std::wcerr << L"Explorer.exe not found." << std::endl;
        return 1;
    }

    HWINEVENTHOOK hook = SetWinEventHook(
        EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE,
        //EVENT_SYSTEM_DIALOGSTART, EVENT_SYSTEM_DIALOGSTART,
        //EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW,
        NULL,
        WinEventProc,
        explorerPID, 0,
        WINEVENT_OUTOFCONTEXT);

    if (!hook) {
        std::wcerr << L"Failed to set event hook!" << std::endl;
        return 1;
    }

    std::wcout << L"Hook set. Monitoring window creation events in explorer.exe..." << std::endl;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWinEvent(hook);
    return 0;
}