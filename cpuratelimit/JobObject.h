#pragma once
#include <stdexcept>

#if __cpp_designated_initializers
#define DESIGNED_INIT(member) member
#else
#define DESIGNED_INIT(member)
#endif
#define nameof(_a) # _a

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
        THROW_IF_WIN32_BOOL_FALSE(SetInformationJobObject(m_hJob.get(), JobObjectNetRateControlInformation, &info, sizeof(info)));
    }
    void setIoRateControlInfo(JOBOBJECT_IO_RATE_CONTROL_INFORMATION  info)
    {
        THROW_IF_WIN32_BOOL_FALSE((BOOL)SetIoRateControlInformationJobObject(m_hJob.get(), &info));
    }
};

namespace my {

    [[nodiscard]]
    inline wil::unique_handle invokeProcess(LPCWSTR path, LPCWSTR param = nullptr)
    {
        SHELLEXECUTEINFO sei = {
            DESIGNED_INIT(.cbSize = ) sizeof(sei),
            DESIGNED_INIT(.fMask = ) SEE_MASK_NOCLOSEPROCESS,
            DESIGNED_INIT(.hwnd = ) nullptr,
            DESIGNED_INIT(.lpVerb = ) nullptr,
            DESIGNED_INIT(.lpFile = ) path,
            DESIGNED_INIT(.lpParameters = ) param,
            DESIGNED_INIT(.lpDirectory = ) nullptr,
            DESIGNED_INIT(.nShow = ) SW_SHOW,
        };
        THROW_IF_WIN32_BOOL_FALSE(ShellExecuteEx(&sei));
        return wil::unique_handle{ sei.hProcess };
    }
    [[nodiscard]]
    inline std::wstring getDlgItemText(HWND hDlg, INT id)
    {
        int len = GetWindowTextLength(GetDlgItem(hDlg, id));
        std::wstring buf(len, L'\0');
        GetDlgItemText(hDlg, id, buf.data(), (int)buf.capacity());
        return buf;
    }

    constexpr DWORD WS_EX_MASK = (
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
        auto hWnd = copyWindow(hWndFrom, name, pos, size, reinterpret_cast<HMENU>(static_cast<DWORD>(ctrlId)), lParam);
        auto hFont = reinterpret_cast<HFONT>(SendMessage(hWndFrom, WM_GETFONT, 0, 0));
        if (hFont)
            SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
        return hWnd;
    }

    inline std::pair<POINT, SIZE> getWindowRect(HWND hWnd)
    {
        RECT r;
        THROW_IF_WIN32_BOOL_FALSE(GetWindowRect(hWnd, &r));
        return { POINT{ r.left, r.top,  }, SIZE{ r.right - r.left, r.bottom - r.top }, };
    }
}
