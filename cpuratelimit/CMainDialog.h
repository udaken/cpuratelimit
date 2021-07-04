#pragma once
#include "framework.h"
#include <vector>
#include <array>
#include <string>
#include <cmath>

#include "JobObject.h"

#include "LimitedInt.h"

#define TrackBaar_SetBuddy(hwndCtrl, leftTop, buddyCtrl) SNDMSG((hwndCtrl), TBM_SETBUDDY, (leftTop), (LPARAM)(buddyCtrl))
#define TrackBaar_SetSelStart(hwndCtrl, redraw, end) SNDMSG((hwndCtrl), TBM_SETSELSTART, (redraw), (end))
#define TrackBaar_SetSelEnd(hwndCtrl, redraw, end) SNDMSG((hwndCtrl), TBM_SETSELEND, (redraw), (end))

#if 1
template <typename T, typename TFrom>
constexpr T checked_cast(TFrom from)
#else
template <typename T>
constexpr T checked_cast(auto from)
#endif
{
    static_assert(std::is_integral_v<decltype(from)>);
    static_assert(std::is_integral_v<T>);
    if (from > std::numeric_limits<T>::max())
        throw std::overflow_error(std::to_string(from));
    if (from < std::numeric_limits<T>::lowest())
        throw std::underflow_error(std::to_string(from));
    return static_cast<T>(from);
}

constexpr DWORD MB(ULONGLONG val) { return checked_cast<DWORD>(val / 1024 / 1024); }

struct Config
{
    Config()
    {
        THROW_IF_WIN32_BOOL_FALSE(GlobalMemoryStatusEx(&memStatus));
        DWORD TotalPageFileInfMB = MB(memStatus.ullTotalPageFile);
        processMemory = { TotalPageFileInfMB,TotalPageFileInfMB, 1 };

    }
    StaticLimitedInt<WORD, 100, 1> cpuRateMin = 1, cpuRateMax = 100;
    StaticLimitedInt<DWORD, 9, 1> cpuRateWeight = 5;
    StaticLimitedInt<DWORD, 10000, 1> cpuRate = 10000;
    constexpr static DWORD LimitPerProcess = (0x80000000000ULL / 1024 / 1024);
    LimitedInt<DWORD> processMemory;
    StaticLimitedInt<DWORD, 1024, 0> bandWidth = 1024;

    MEMORYSTATUSEX memStatus{ sizeof(memStatus) };
};

class CMainDialog
{
    CMainDialog(Config& config) : config(config)
    {

    }
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        constexpr static auto getUserData = [](HWND hDlg) {return reinterpret_cast<CMainDialog*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));        };

        switch (message)
        {
        case WM_INITDIALOG:
        {
            CMainDialog* self{ reinterpret_cast<CMainDialog*>(lParam) };
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
            self->hDlg = hDlg;

            self->onWmInitDialog();

            return (INT_PTR)TRUE;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            else if (getUserData(hDlg)->onWmCommand(LOWORD(wParam), HIWORD(wParam)))
            {
                return (INT_PTR)TRUE;
            }
            break;
        }
        case WM_NOTIFY:
        {
            getUserData(hDlg)->onWmNotify((LPNMHDR)lParam);
            break;
        }
        default:
            break;
        }
        return (INT_PTR)FALSE;
    }

    void onWmInitDialog()
    {
        Button_SetCheck(getDlgItem(IDC_RADIO_CPU_NOLIMIT), BST_CHECKED);

        setTrackbarFrom(IDC_SLIDER_CPU_RATE, config.cpuRate);
        SetDlgItemInt(hDlg, IDC_EDIT_CPU_RATE, config.cpuRate, FALSE);
        TrackBaar_SetBuddy(getDlgItem(IDC_SLIDER_CPU_RATE), FALSE, getDlgItem(IDC_EDIT_CPU_RATE));

        Edit_LimitText(getDlgItem(IDC_EDIT_CPU_RATE_MIN), config.cpuRateMin.log10OfMaxValue() + 1);
        SetDlgItemInt(hDlg, IDC_EDIT_CPU_RATE_MIN, config.cpuRateMin, FALSE);

        Edit_LimitText(getDlgItem(IDC_EDIT_CPU_RATE_MAX), config.cpuRateMax.log10OfMaxValue() + 1);
        SetDlgItemInt(hDlg, IDC_EDIT_CPU_RATE_MAX, config.cpuRateMax, FALSE);

        setTrackbarFrom(IDC_SLIDER_CPU_RATE_WEIGHT, config.cpuRateWeight);
        SetDlgItemInt(hDlg, IDC_EDIT_CPU_RATE_WEIGHT, config.cpuRateWeight, FALSE);
        TrackBaar_SetBuddy(getDlgItem(IDC_SLIDER_CPU_RATE_WEIGHT), FALSE, getDlgItem(IDC_EDIT_CPU_RATE_WEIGHT));

        setTrackbarFrom(IDC_SLIDER_MEMORY_LIMIT, config.processMemory, 100);
        SetDlgItemInt(hDlg, IDC_EDIT_MEMORY_LIMIT, MB(config.memStatus.ullAvailPageFile), FALSE);

        TrackBaar_SetSelStart(getDlgItem(IDC_SLIDER_MEMORY_LIMIT), FALSE, 0);
        TrackBaar_SetSelEnd(getDlgItem(IDC_SLIDER_MEMORY_LIMIT), FALSE, MB(config.memStatus.ullTotalPhys));

        setTrackbarFrom(IDC_SLIDER_BANDWIDTH_LIMIT, config.bandWidth, 100);
        Button_SetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_GB), BST_CHECKED);

        auto check{ getDlgItem(IDC_CHECK_AFFINITY0) };
        affinityCheckBox[0] = check;
        auto [pos, size] = my::getWindowRect(check);
        ScreenToClient(hDlg, &pos);

        SYSTEM_INFO si{};
        GetSystemInfo(&si);
#if _DEBUG
        si.dwNumberOfProcessors = sizeof(DWORD_PTR) * 8;
#endif
        for (DWORD i = 1; i < si.dwNumberOfProcessors; i++)
        {
            POINT pos1{ pos.x + ((size.cx + 3) * ((int)i % 16)), pos.y + (size.cy + 1) * ((int)i / 16) };
            affinityCheckBox[i] = my::copyDlgItem(check, std::to_wstring(i).c_str(), pos1, size);
        }
        //put_affinityMask(si.dwActiveProcessorMask);
        DWORD_PTR processAffinityMask, systemAffinityMask;
        GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
        put_affinityMask(systemAffinityMask);

        if (args.size() > 0)
            put_path(args[0]);
    }

    bool onWmCommand(WORD ctrlId, WORD codeNotify) try
    {
        switch (ctrlId)
        {
        case IDC_BUTTON_RUN:
        {
            auto path = get_path();
            if (path.empty())
            {
                return true;
            }
            auto job = JobObject::create();
            job.setCpuRateControlInfo(get_cpuRateControlInfo());
            auto extendLimit = get_extendedLimitInfo();
            job.setExtendLimitInfo(extendLimit);
            job.setNetRateControlInfo(get_netRateControlInfo());
            auto process = my::invokeProcess(path.c_str(), my::getDlgItemText(hDlg, IDC_EDIT_PARAM).c_str());
            job.assignProcess(process.get());
            WaitForInputIdle(process.get(), 10000);
            return true;
        }
        case IDC_SPLIT_FILE_SELECT:
        {
            WCHAR path[MAX_PATH]{};
            OPENFILENAME ofn
            {
                DESIGNED_INIT(.lStructSize = )sizeof(ofn),
                DESIGNED_INIT(.hwndOwner = ) hDlg,
                DESIGNED_INIT(.hInstance = )nullptr,
                DESIGNED_INIT(.lpstrFilter = )L"*.*",
                DESIGNED_INIT(.lpstrCustomFilter = ) nullptr,
                DESIGNED_INIT(.nMaxCustFilter = ) 0,
                DESIGNED_INIT(.nFilterIndex = )0,
                DESIGNED_INIT(.lpstrFile = ) path,
                DESIGNED_INIT(.nMaxFile = )ARRAYSIZE(path),
            };
            if (GetOpenFileName(&ofn))
            {
                put_path(path);
            }
            return true;
        }
        default:
        {
            LimitedInt<DWORD> item;
            if (codeNotify == EN_UPDATE && tryGetLimitedInt(ctrlId, item))
            {
                BOOL transelated;
                UINT val = GetDlgItemInt(hDlg, ctrlId, &transelated, FALSE);

                if (!(transelated && item.canConvertFrom(val)))
                {
                    Edit_Undo(getDlgItem(ctrlId));
                    Edit_EmptyUndoBuffer(getDlgItem(ctrlId));
                }
            }
        }
        break;
        }
        return false;
    }
    catch (...)
    {
        DebugBreak();
        return false;
    }

    void onNotifyButtonDropdown(WORD ctrlId, LPNMBCDROPDOWN pnmdropdown)
    {

    }
    void onNotifyTrackBarThumbPosChanging(WORD ctrlId, NMTRBTHUMBPOSCHANGING* pnmdropdown)
    {

    }

    void onWmNotify(LPNMHDR lpnhdr)
    {
        switch (lpnhdr->code)
        {
        case BCN_DROPDOWN:
            onNotifyButtonDropdown((WORD)lpnhdr->idFrom, (LPNMBCDROPDOWN)lpnhdr);
            break;
        case TRBN_THUMBPOSCHANGING:
            onNotifyTrackBarThumbPosChanging((WORD)lpnhdr->idFrom, (NMTRBTHUMBPOSCHANGING*)lpnhdr);
            break;
        default:
            break;
        }
    }

    HWND hDlg{};
    std::vector<LPCWSTR> args;
    std::array<HWND, sizeof(DWORD_PTR) * 8> affinityCheckBox{};
    Config& config;

    void setTrackbarFrom(WORD id, LimitedInt<DWORD> val, DWORD tic = 10)
    {
        SendMessage(getDlgItem(id), TBM_SETRANGEMIN, FALSE, val.min_value());
        SendMessage(getDlgItem(id), TBM_SETRANGEMAX, FALSE, val.max_value());
        DWORD tickfreq = (val.max_value() - val.min_value() + 1) / tic;
        SendMessage(getDlgItem(id), TBM_SETTICFREQ, tickfreq ? tickfreq : 1, 0);
        SendMessage(getDlgItem(id), TBM_SETPOS, TRUE, val);
    }

    bool tryGetLimitedInt(WORD id, LimitedInt<DWORD>& val) const
    {
        switch (id)
        {
        case IDC_EDIT_CPU_RATE_WEIGHT:
            val = config.cpuRateWeight;
            return true;
        case IDC_EDIT_CPU_RATE_MAX:
            val = config.cpuRateMax;
            return true;
        case IDC_EDIT_CPU_RATE_MIN:
            val = config.cpuRateMin;
            return true;
        default:
            return false;
        }
    }

    std::wstring get_path()
    {
        return my::getDlgItemText(hDlg, IDC_EDIT_PATH);
    }

    void put_path(std::wstring_view s)
    {
        SetDlgItemText(hDlg, IDC_EDIT_PATH, s.data());
    }

    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION get_cpuRateControlInfo()
    {
        JOBOBJECT_CPU_RATE_CONTROL_INFORMATION i{};
        i.ControlFlags = Button_GetCheck(getDlgItem(IDC_RADIO_CPU_NOLIMIT)) ? 0 : JOB_OBJECT_CPU_RATE_CONTROL_ENABLE;
        if (Button_GetCheck(getDlgItem(IDC_RADIO_CPU_RATE)))
        {
            i.ControlFlags |=
                JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
            i.CpuRate = getDlgItemUInt(IDC_EDIT_CPU_RATE);
        }
        else if (Button_GetCheck(getDlgItem(IDC_RADIO_CPU_WEIGHT_BASED)))
        {
            i.ControlFlags |=
                JOB_OBJECT_CPU_RATE_CONTROL_WEIGHT_BASED;
            i.ControlFlags |=
                Button_GetCheck(getDlgItem(IDC_CHECK_CPU_HARD_CAP)) ? JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP : 0;
            i.Weight = getDlgItemUInt(IDC_EDIT_CPU_RATE_WEIGHT);
        }
        else if (Button_GetCheck(getDlgItem(IDC_RADIO_CPU_MIN_MAX_RATE)))
        {
            i.ControlFlags |= JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE;
            i.MaxRate = checked_cast<WORD>(getDlgItemUInt(IDC_EDIT_CPU_RATE_MAX));
            i.MinRate = checked_cast<WORD>(getDlgItemUInt(IDC_EDIT_CPU_RATE_MIN));
        }
        return i;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION get_extendedLimitInfo()
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION i{};
        i.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
        i.ProcessMemoryLimit = getDlgItemUInt(IDC_SLIDER_MEMORY_LIMIT) * 1024ULL * 1024U;

        i.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
        i.BasicLimitInformation.Affinity = get_affinityMask();

        return i;
    }

    JOBOBJECT_NET_RATE_CONTROL_INFORMATION get_netRateControlInfo()
    {
        JOBOBJECT_NET_RATE_CONTROL_INFORMATION i{};
        i.ControlFlags = JOB_OBJECT_NET_RATE_CONTROL_ENABLE | JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH;
        i.MaxBandwidth = getDlgItemUInt(IDC_SLIDER_BANDWIDTH_LIMIT) *
            Button_GetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_GB)) ? 1024ULL * 1024 * 1024 :
            Button_GetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_MB)) ? 1024ULL * 1024 :
            Button_GetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_KB)) ? 1024ULL :
            1;
        return i;

    }

    DWORD_PTR get_affinityMask()
    {
        DWORD_PTR mask = 0;
        for (DWORD i = 0; i < affinityCheckBox.size(); i++)
            if (auto hWnd = affinityCheckBox[i])
                mask |= Button_GetCheck(hWnd) ? ((DWORD_PTR)1 << i) : 0;
        return mask;
    }
    void put_affinityMask(DWORD_PTR mask)
    {
        for (DWORD i = 0; i < affinityCheckBox.size(); i++)
        {
            if (auto hWnd = affinityCheckBox[i])
                Button_SetCheck(hWnd, (mask & ((DWORD_PTR)1 << i) ? BST_CHECKED : BST_UNCHECKED));
        }
    }

    int getDlgItemInt(WORD ctrlId)
    {
        BOOL b;
        int val = GetDlgItemInt(hDlg, ctrlId, &b, TRUE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    unsigned  getDlgItemUInt(WORD ctrlId)
    {
        BOOL b;
        unsigned  val = GetDlgItemInt(hDlg, ctrlId, &b, FALSE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    HWND getDlgItem(WORD ctrlId)
    {
        HWND hWnd = GetDlgItem(hDlg, ctrlId);
        THROW_IF_NULL_ALLOC(hWnd);
        return hWnd;
    }

public:

    static INT_PTR showModal(HINSTANCE hInst, LPCWSTR lpCmdLine)
    {
        int argc;
        auto argv = CommandLineToArgvW(lpCmdLine, &argc);
        argv++; argc--;

        Config config{};
        CMainDialog self{ config };
        self.args = { argv , argv + argc };

        return DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), nullptr, &DlgProc, reinterpret_cast<LPARAM>(&self));
    }

};

