#pragma once
#include "framework.h"
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <cassert>

#include "JobObject.h"

#include "LimitedInt.h"
#include <optional>

#define TrackBaar_GetRangeMax(hwndCtrl) (int)SNDMSG((hwndCtrl), TBM_GETRANGEMAX, 0, 0)
#define TrackBaar_GetPos(hwndCtrl) (int)SNDMSG((hwndCtrl), TBM_GETPOS, 0, 0)
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

using std::begin, std::end;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

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
    StaticLimitedInt<DWORD, 1023, 0> bandWidth = 1023;

    MEMORYSTATUSEX memStatus{ sizeof(memStatus) };

    void restoreDefault()
    {
        *this = Config();
    }
};

template <size_t size>
struct RadioGroup
{
    const std::array<int, size> group;

    void check(HWND hDlg, int checkId) const
    {
        assert(std::find(begin(group), end(group), checkId) != end(group));
        CheckRadioButton(hDlg, group[0], group[group.size() - 1], checkId);
    }

    constexpr int last() const { return group[group.size() - 1]; }

    operator std::array<int, size>() const { return group; }
};

static inline struct {
    const int ctrlId;
    const std::vector<int> relatedControls;
} const radioButtonRelatedControl[] =
{
    {IDC_RADIO_CPU_WEIGHT_BASED, {IDC_SLIDER_CPU_RATE_WEIGHT}},
    {IDC_RADIO_CPU_MIN_MAX_RATE, {IDC_EDIT_CPU_RATE_WEIGHT, IDC_CHECK_CPU_HARD_CAP}},
    {IDC_RADIO_CPU_RATE, {}},
};

class CMainDialog
{
    constexpr static inline auto radioGroupCpuLimit = RadioGroup{ std::to_array(
        { IDC_RADIO_CPU_NOLIMIT, IDC_RADIO_CPU_WEIGHT_BASED, IDC_RADIO_CPU_MIN_MAX_RATE, IDC_RADIO_CPU_RATE }
    ) };

    constexpr static auto radioGroupBandwidth = RadioGroup{ std::to_array(
        { IDC_RADIO_BANDWIDTH, IDC_RADIO_BANDWIDTH_KB, IDC_RADIO_BANDWIDTH_MB, IDC_RADIO_BANDWIDTH_GB }
    ) };

    CMainDialog(HINSTANCE hInst, Config& config) : config(config)
        , hMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDC_CPURATELIMIT)))
    {
        assert(hMenu);
    }
    ~CMainDialog() noexcept
    {
        DestroyMenu(hMenu);
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
            assert(self->hDlg == nullptr);
            self->hDlg = hDlg;

            self->onWmInitDialog();

            return (INT_PTR)TRUE;
        }
        case WM_COMMAND:
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
        case WM_NOTIFY:
            getUserData(hDlg)->onWmNotify((LPNMHDR)lParam);
            break;
        case WM_VSCROLL:
        case WM_HSCROLL:
            getUserData(hDlg)->onWmScroll(message == WM_VSCROLL, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
            break;
        default:
            break;
        }
        return (INT_PTR)FALSE;
    }

    void onWmInitDialog()
    {
        auto check{ getDlgItem(IDC_CHECK_AFFINITY0) };
        affinityCheckBox[0] = check;
        auto [pos, size] = my::getWindowRect(check);
        THROW_IF_WIN32_BOOL_FALSE(ScreenToClient(hDlg, &pos));

        for (DWORD i = 1; i < sizeof(DWORD_PTR) * 8; i++)
        {
            POINT pos1{ pos.x + ((size.cx + 3) * ((int)i % 16)), pos.y + (size.cy + 1) * ((int)i / 16) };
            affinityCheckBox[i] = my::copyDlgItem(check, std::to_wstring(i).c_str(), pos1, size);
        }
        if (args.size() > 0)
            put_path(args[0]);

        setValues();
    }

    void setValues()
    {
        radioGroupCpuLimit.check(hDlg, radioGroupCpuLimit.group[0]);

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
        SetDlgItemInt(hDlg, IDC_EDIT_BANDWIDTH_LIMIT, config.bandWidth, FALSE);
        radioGroupBandwidth.check(hDlg, radioGroupBandwidth.last());

        //put_affinityMask(si.dwActiveProcessorMask);
        DWORD_PTR processAffinityMask, systemAffinityMask;
        GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
        put_affinityMask(systemAffinityMask);

    }

    HWND createToolTip(int toolID, HWND hDlg, PTSTR pszText)
    {
        if (!toolID || !hDlg || !pszText)
        {
            return FALSE;
        }
        HWND hwndTool = GetDlgItem(hDlg, toolID);

        // Create the tooltip. g_hInst is the global instance handle.
        HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hDlg, NULL,
            nullptr, NULL);

        if (!hwndTool || !hwndTip)
        {
            return (HWND)NULL;
        }

        // Associate the tooltip with the tool.
        TOOLINFO toolInfo = { 0 };
        toolInfo.cbSize = sizeof(toolInfo);
        toolInfo.hwnd = hDlg;
        toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        toolInfo.uId = (UINT_PTR)hwndTool;
        toolInfo.lpszText = pszText;
        SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

        return hwndTip;
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
            auto pid = GetProcessId(process.get());
            MessageBox(hDlg, (L"Process ran with PID: "s + std::to_wstring(pid).append(L"\nWhile this dialog is displayed, the process will be constrained."sv)).c_str(), 
                my::getWindowText(hDlg).c_str(), MB_ICONINFORMATION);
            return true;
        }
        case IDC_LOAD_DEFAULT:
        {
            config.restoreDefault();
            setValues();
            return true;
        }
        case IDC_SPLIT_FILE_SELECT:
        {
            auto path = openFileDialog(hDlg, get_path());
            if (path)
            {
                put_path(*path);
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

    std::optional<std::wstring> static openFileDialog(HWND hDlg, std::wstring&& path)
    {
        path.resize(MAX_PATH);

        OPENFILENAME ofn
        {
            DESIGNED_INIT(.lStructSize = )sizeof(ofn),
            DESIGNED_INIT(.hwndOwner = ) hDlg,
            DESIGNED_INIT(.hInstance = )nullptr,
            DESIGNED_INIT(.lpstrFilter = )L"*.*",
            DESIGNED_INIT(.lpstrCustomFilter = ) nullptr,
            DESIGNED_INIT(.nMaxCustFilter = ) 0,
            DESIGNED_INIT(.nFilterIndex = )0,
            DESIGNED_INIT(.lpstrFile = ) path.data(),
            DESIGNED_INIT(.nMaxFile = ) checked_cast<DWORD>(path.size()),
        };
        if (GetOpenFileName(&ofn))
        {
            return { path };
        }
        return {};
    }

    void onNotifyButtonDropdown(WORD ctrlId, LPNMBCDROPDOWN pnmdropdown)
    {
        if (ctrlId == IDC_SPLIT_FILE_SELECT)
        {
            auto [p, size] = my::getWindowRect(getDlgItem(IDC_SPLIT_FILE_SELECT));
            p.y += size.cy;

            TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_LEFTALIGN | TPM_TOPALIGN, p.x, p.y, 0, hDlg, nullptr);
        }
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

    void onWmScroll(bool vertical, WORD pos, WORD cause, HWND ctrlhWnd)
    {
        if (ctrlhWnd == getDlgItem(IDC_SLIDER_CPU_RATE_WEIGHT))
        {
            auto value = getTrackBarPos(IDC_SLIDER_CPU_RATE_WEIGHT);
            SetDlgItemInt(hDlg, IDC_EDIT_CPU_RATE_WEIGHT, value, FALSE);
        }
        else if (ctrlhWnd == getDlgItem(IDC_SLIDER_BANDWIDTH_LIMIT))
        {
            auto value = getTrackBarPos(IDC_SLIDER_BANDWIDTH_LIMIT);
            SetDlgItemInt(hDlg, IDC_EDIT_BANDWIDTH_LIMIT, value, FALSE);
        }
        else if (ctrlhWnd == getDlgItem(IDC_SLIDER_MEMORY_LIMIT))
        {

        }
    }
    HWND hDlg{};
    HMENU hMenu{};

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
            i.CpuRate = getTrackBarPos(IDC_SLIDER_CPU_RATE);
        }
        else if (Button_GetCheck(getDlgItem(IDC_RADIO_CPU_WEIGHT_BASED)))
        {
            i.ControlFlags |=
                JOB_OBJECT_CPU_RATE_CONTROL_WEIGHT_BASED;
            i.ControlFlags |=
                Button_GetCheck(getDlgItem(IDC_CHECK_CPU_HARD_CAP)) ? JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP : 0;
            i.Weight = getTrackBarPos(IDC_SLIDER_CPU_RATE_WEIGHT);
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
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        auto max = TrackBaar_GetRangeMax(getDlgItem(IDC_SLIDER_MEMORY_LIMIT));
        auto pos = getTrackBarPos(IDC_SLIDER_MEMORY_LIMIT);
        if (pos < max)
        {
            if (Button_GetCheck(getDlgItem(IDC_CHECK_MEMORY_LIMIT_JOB)))
            {
                info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
                info.JobMemoryLimit = getTrackBarPos(IDC_SLIDER_MEMORY_LIMIT) * 1024ULL * 1024U;
            }
            else
            {
                info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
                info.ProcessMemoryLimit = getTrackBarPos(IDC_SLIDER_MEMORY_LIMIT) * 1024ULL * 1024U;
            }
        }

        DWORD_PTR processAffinityMask, systemAffinityMask;
        GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
        if (systemAffinityMask != get_affinityMask())
        {
            info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
            info.BasicLimitInformation.Affinity = get_affinityMask();
        }

        return info;
    }

    JOBOBJECT_NET_RATE_CONTROL_INFORMATION get_netRateControlInfo()
    {
        if (Button_GetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_GB)) && 
            getTrackBarPos(IDC_SLIDER_BANDWIDTH_LIMIT) == TrackBaar_GetRangeMax(getDlgItem(IDC_SLIDER_BANDWIDTH_LIMIT)))
        {
            return {};
        }
        JOBOBJECT_NET_RATE_CONTROL_INFORMATION info{};
        info.ControlFlags = JOB_OBJECT_NET_RATE_CONTROL_ENABLE | JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH;
        info.MaxBandwidth = getTrackBarPos(IDC_SLIDER_BANDWIDTH_LIMIT) *
            Button_GetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_GB)) ? 1024ULL * 1024 * 1024 :
            Button_GetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_MB)) ? 1024ULL * 1024 :
            Button_GetCheck(getDlgItem(IDC_RADIO_BANDWIDTH_KB)) ? 1024ULL :
            1;
        return info;
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
            {
                Button_SetCheck(hWnd, (mask & ((DWORD_PTR)1 << i) ? BST_CHECKED : BST_UNCHECKED));
                Button_Enable(hWnd, (mask & ((DWORD_PTR)1 << i)));
            }
        }
    }

    int getTrackBarPos(int ctrlId)
    {
        return static_cast<int>(TrackBaar_GetPos(getDlgItem(ctrlId)));
    }

    int getDlgItemInt(int ctrlId)
    {
        BOOL b;
        int val = GetDlgItemInt(hDlg, ctrlId, &b, TRUE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    unsigned  getDlgItemUInt(int ctrlId)
    {
        BOOL b;
        unsigned  val = GetDlgItemInt(hDlg, ctrlId, &b, FALSE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    HWND getDlgItem(int ctrlId)
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
        CMainDialog self{ hInst, config };
        self.args = { argv , argv + argc };

        return DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), nullptr, &DlgProc, reinterpret_cast<LPARAM>(&self));
    }

};

