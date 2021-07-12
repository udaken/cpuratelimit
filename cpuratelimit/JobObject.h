#pragma once
#include "framework.h"
#include <stdexcept>

class JobObject final
{
    wil::unique_handle m_hJob;
    JobObject(wil::unique_handle&& hJob)
        : m_hJob{ std::move(hJob) }
    {
        if (m_hJob == nullptr)
        {
            throw std::invalid_argument(nameof(hJob));
        }
    }
public:
    [[nodiscard]]
    static JobObject createByName(LPCWSTR name)
    {
        wil::unique_handle hJob{ CreateJobObject(nullptr,name) };
        THROW_IF_NULL_ALLOC(hJob);
        return hJob;
    }

    [[nodiscard]]
    static JobObject create() { return createByName(nullptr); }

    void assignProcess(HANDLE hProcess)
    {
        THROW_IF_WIN32_BOOL_FALSE(AssignProcessToJobObject(m_hJob.get(), hProcess));
    }

    [[nodiscard]]
    HANDLE get() const noexcept { return m_hJob.get(); }

    void setCpuRateControlInfo(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info)
    {
        if (info.ControlFlags != 0)
            THROW_IF_WIN32_BOOL_FALSE(SetInformationJobObject(m_hJob.get(), JobObjectCpuRateControlInformation, &info, sizeof(info)));
    }

    void setExtendLimitInfo(JOBOBJECT_EXTENDED_LIMIT_INFORMATION& info)
    {
        if (info.BasicLimitInformation.LimitFlags != 0)
            THROW_IF_WIN32_BOOL_FALSE(SetInformationJobObject(m_hJob.get(), JobObjectExtendedLimitInformation, &info, sizeof(info)));
    }

    void setNetRateControlInfo(JOBOBJECT_NET_RATE_CONTROL_INFORMATION info)
    {
        if (info.ControlFlags != 0)
            THROW_IF_WIN32_BOOL_FALSE(SetInformationJobObject(m_hJob.get(), JobObjectNetRateControlInformation, &info, sizeof(info)));
    }
};

namespace my {

    [[nodiscard]]
    inline std::wstring getWindowText(HWND hWnd)
    {
        int len = GetWindowTextLength(hWnd);
        std::wstring buf(len, L'\0');
        GetWindowText(hWnd, buf.data(), (int)buf.size() + 1);
        return buf;
    }
    [[nodiscard]]
    inline wil::unique_handle invokeProcess(std::wstring_view path, std::wstring_view param = nullptr)
    {
        SHELLEXECUTEINFO sei = {
            DESIGNED_INIT(.cbSize = ) sizeof(sei),
            DESIGNED_INIT(.fMask = ) SEE_MASK_NOCLOSEPROCESS,
            DESIGNED_INIT(.hwnd = ) nullptr,
            DESIGNED_INIT(.lpVerb = ) nullptr,
            DESIGNED_INIT(.lpFile = ) path.data(),
            DESIGNED_INIT(.lpParameters = ) param.data(),
            DESIGNED_INIT(.lpDirectory = ) nullptr,
            DESIGNED_INIT(.nShow = ) SW_SHOW,
        };
        THROW_LAST_ERROR_IF(ShellExecuteEx(&sei) == FALSE && GetLastError() != ERROR_CANCELLED);
        return wil::unique_handle{ sei.hProcess };
    }

    [[nodiscard]]
    inline std::wstring getDlgItemText(HWND hDlg, INT id)
    {
        return getWindowText(GetDlgItem(hDlg, id));
    }

    constexpr inline DWORD WS_EX_MASK = (
        WS_EX_RIGHT |
        WS_EX_LEFT |
        WS_EX_RTLREADING |
        WS_EX_LTRREADING |
        WS_EX_LEFTSCROLLBAR |
        WS_EX_RIGHTSCROLLBAR |
        WS_EX_CONTROLPARENT |
        WS_EX_STATICEDGE |
        WS_EX_APPWINDOW |
        WS_EX_OVERLAPPEDWINDOW |
        WS_EX_PALETTEWINDOW |
        WS_EX_LAYERED |
        WS_EX_NOINHERITLAYOUT |
#if (WINVER >= 0x0602)
        WS_EX_NOREDIRECTIONBITMAP |
#endif
        WS_EX_LAYOUTRTL |
        WS_EX_COMPOSITED |
        WS_EX_NOACTIVATE);

    [[nodiscard]]
    inline HWND copyWindow(HWND hWndFrom, LPCWSTR name, POINT pos, SIZE size, HMENU hMenu = nullptr, LPVOID lParam = nullptr)
    {
        WINDOWINFO wi{ sizeof(wi) };
        THROW_IF_WIN32_BOOL_FALSE(GetWindowInfo(hWndFrom, &wi));

        auto hmodule{ (HMODULE)GetWindowLongPtr(hWndFrom, GWLP_HINSTANCE) };
        //auto hParent{ (HWND)GetWindowLong(hWndFrom, GWLP_HWNDPARENT) };
        auto hParent{ GetParent(hWndFrom) };
        auto hWnd{ CreateWindowEx((WS_EX_MASK & wi.dwExStyle),(LPCWSTR)wi.atomWindowType, name,
            wi.dwStyle, pos.x, pos.y, size.cx, size.cy, hParent, hMenu, hmodule, lParam) };
        THROW_IF_NULL_ALLOC(hWnd);
        return hWnd;
    }

    [[nodiscard]]
    inline HWND copyDlgItem(HWND hWndFrom, LPCWSTR name, POINT pos, SIZE size, int  ctrlId = 0, LPVOID lParam = nullptr)
    {
        auto hWnd = copyWindow(hWndFrom, name, pos, size, reinterpret_cast<HMENU>(static_cast<ULONG_PTR>(static_cast<DWORD>(ctrlId))), lParam);
        auto hFont = reinterpret_cast<HFONT>(SendMessage(hWndFrom, WM_GETFONT, 0, 0));
        if (hFont)
            SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
        return hWnd;
    }

    [[nodiscard]]
    inline std::pair<POINT, SIZE> rectToPointSize(const RECT& r)
    {
        return { POINT{ r.left, r.top,  }, SIZE{ r.right - r.left, r.bottom - r.top }, };
    }

    [[nodiscard]]
    inline std::pair<POINT, SIZE> getWindowRect(HWND hWnd)
    {
        RECT r;
        THROW_IF_WIN32_BOOL_FALSE(GetWindowRect(hWnd, &r));
        return rectToPointSize(r);
    }

    [[nodiscard]]
    inline std::pair<POINT, SIZE> getClientRect(HWND hWnd)
    {
        RECT r;
        THROW_IF_WIN32_BOOL_FALSE(GetClientRect(hWnd, &r));
        return rectToPointSize(r);
    }

    [[nodiscard]]
    inline std::wstring getModuleFileName()
    {
        std::wstring buf(MAX_PATH, L'\0');
        THROW_LAST_ERROR_IF(GetModuleFileName(nullptr, buf.data(), MAX_PATH) == 0);
        return buf;
    }

    [[nodiscard]]
    inline std::wstring renameExtension(std::wstring&& path, std::wstring_view ext)
    {
        path.reserve(path.capacity() + ext.length());
        THROW_IF_FAILED(PathCchRenameExtension(path.data(), path.capacity(), ext.data()));
        return path;
    }
}
