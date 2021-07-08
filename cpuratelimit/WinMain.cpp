#include "framework.h"
#include "resource.h"
#include <clocale>

#define MAX_LOADSTRING 100

HINSTANCE g_hInst;
WCHAR g_szTitle[MAX_LOADSTRING];

#include "MainDialog.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' \
    name='Microsoft.Windows.Common-Controls' \
    version='6.0.0.0' \
    processorArchitecture='*' \
    publicKeyToken='6595b64144ccf1df' \
    language='*'\"")
#pragma comment(lib, "Pathcch.lib")

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    std::setlocale(LC_ALL, "");

#if !_NDEBUG
    wil::g_fBreakOnFailure = true;
#endif
    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);

    return (int)MainDialog::showModal(g_hInst, lpCmdLine);
}
