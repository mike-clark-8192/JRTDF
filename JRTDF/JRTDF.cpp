#include <iostream>
#include <string>
#include <windows.h>
#include <UIAutomation.h>
#pragma comment(lib, "UIAutomationCore.lib")

#define JRTDFSTRLEN 64

#define DIALOG_CONTENT L"If you change a file name extension, the file might become unusable.\n\nAre you sure you want to change it?"
#define DIALOG_TITLE L"Rename"
#define YESBUTTONTEXT L"&Yes"

#ifdef _DEBUG
#define DEBUG_PRINT(x) std::wcout << x << std::endl
#else
#define DEBUG_PRINT(x)
#endif

struct ButtonData {
    HWND foundButtonHandle = NULL;
};

IUIAutomation* pAutomation = NULL;

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);
bool FindFirstTextElement(IUIAutomationElement* pElement, IUIAutomationElement** ppFoundElement);
std::wstring GetTextFromControl(HWND hwnd);
void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
BOOL InitializeUIAutomation();

int JRTDF() {
    if (!InitializeUIAutomation()) {
        return 1;
    }

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
    if (GetClassNameW(hwnd, windowClass, JRTDFSTRLEN) && wcscmp(windowClass, L"#32770") == 0)
    {
        wchar_t windowTitle[JRTDFSTRLEN];
        DEBUG_PRINT(L"Found a dialog window.");
        if (GetWindowTextW(hwnd, windowTitle, JRTDFSTRLEN)) {
            DEBUG_PRINT(L"Window title: " << windowTitle);
            if (wcscmp(windowTitle, DIALOG_TITLE) == 0) {
                ButtonData data;
                EnumChildWindows(hwnd, EnumChildProc, reinterpret_cast<LPARAM>(&data));

                if (data.foundButtonHandle) {
                    DEBUG_PRINT(L"Found the '&Yes' button");
                    std::wstring dialogContent = GetTextFromControl(hwnd);
                    DEBUG_PRINT(L"Dialog content: " << dialogContent);
                    SendMessage(data.foundButtonHandle, BM_CLICK, 0, 0);
                }
            }
        }
    }
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    ButtonData* pData = reinterpret_cast<ButtonData*>(lParam);
    wchar_t className[JRTDFSTRLEN];

    if (GetClassNameW(hwnd, className, JRTDFSTRLEN) && wcscmp(className, L"Button") == 0)
    {
        wchar_t windowText[JRTDFSTRLEN];
        if (GetWindowTextW(hwnd, windowText, JRTDFSTRLEN) && wcscmp(windowText, YESBUTTONTEXT) == 0) {
            pData->foundButtonHandle = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}


BOOL InitializeUIAutomation()
{
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM." << std::endl;
        return FALSE;
    }

    hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, IID_IUIAutomation, reinterpret_cast<void**>(&pAutomation));
    if (FAILED(hr)) {
        std::wcerr << L"Failed to create UI Automation instance." << std::endl;
        CoUninitialize();
        return FALSE;
    }

    return TRUE;
}


std::wstring GetTextFromControl(HWND hwnd)
{
    IUIAutomationElement* pRootElement = NULL;
    IUIAutomationElement* pFoundElement = NULL;
    VARIANT varProp;
    // BSTR bstrName;

    // Initialize the return value as an empty wstring
    std::wstring text;

    // Get the root element from the specified window handle
    pAutomation->ElementFromHandle(hwnd, &pRootElement);
    if (pRootElement) {
        if (FindFirstTextElement(pRootElement, &pFoundElement) && pFoundElement) {
            // If a text element is found, get its Name property which contains the text
            pFoundElement->GetCurrentPropertyValue(UIA_NamePropertyId, &varProp);
            if (varProp.vt == VT_BSTR && varProp.bstrVal) {
                text = varProp.bstrVal; // Convert BSTR to std::wstring
            }
            pFoundElement->Release();
        }
        pRootElement->Release();
    }

    return text; // Return the found text or empty if not found
}

bool FindFirstTextElement(IUIAutomationElement* pElement, IUIAutomationElement** ppFoundElement)
{
    IUIAutomationTreeWalker* pControlWalker = NULL;
    IUIAutomationElement* pChild = NULL;
    VARIANT varProp;

    // Create a TreeWalker for UI Automation
    pAutomation->get_ControlViewWalker(&pControlWalker);
    if (pControlWalker == NULL) {
        return false;
    }

    // Check if the current element is of the type Text
    pElement->GetCurrentPropertyValue(UIA_ControlTypePropertyId, &varProp);
    if (varProp.vt == VT_I4 && varProp.lVal == UIA_TextControlTypeId) {
        *ppFoundElement = pElement;
        (*ppFoundElement)->AddRef(); // Add reference as we are returning this element
        pControlWalker->Release();
        return true;
    }

    // Recursively traverse the child elements
    pControlWalker->GetFirstChildElement(pElement, &pChild);
    while (pChild != NULL) {
        if (FindFirstTextElement(pChild, ppFoundElement)) {
            pChild->Release();
            pControlWalker->Release();
            return true;
        }
        IUIAutomationElement* pNext = NULL;
        pControlWalker->GetNextSiblingElement(pChild, &pNext);
        pChild->Release();
        pChild = pNext;
    }

    pControlWalker->Release();
    return false;
}

#ifdef _CONSOLE
int main(int /*argc*/, char** /*argv*/)
{
    return JRTDF();
}
#else
int WINAPI WinMain(_In_ HINSTANCE /*hInstance*/, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
    return JRTDF();
}
#endif
