#pragma once
#include "framework.h"
#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <optional>
#include <cmath>
#include <cassert>

#include "JobObject.h"

#include "LimitedInt.h"

#define TrackBaar_GetRangeMax(hwndCtrl) (int)SNDMSG((hwndCtrl), TBM_GETRANGEMAX, 0, 0)
#define TrackBaar_GetPos(hwndCtrl) (int)SNDMSG((hwndCtrl), TBM_GETPOS, 0, 0)
#define TrackBaar_SetPos(hwndCtrl, redraw, pos) (void)SNDMSG((hwndCtrl), TBM_SETPOS, (BOOL)(redraw), (pos))
#define TrackBaar_SetBuddy(hwndCtrl, leftTop, buddyCtrl) (void)SNDMSG((hwndCtrl), TBM_SETBUDDY, (leftTop), (LPARAM)(buddyCtrl))
#define TrackBaar_SetSelStart(hwndCtrl, redraw, end) (void)SNDMSG((hwndCtrl), TBM_SETSELSTART, (BOOL)(redraw), (end))
#define TrackBaar_SetSelEnd(hwndCtrl, redraw, end) (void)SNDMSG((hwndCtrl), TBM_SETSELEND, (BOOL)(redraw), (end))

#if 1
template <typename TValue, typename TFrom>
constexpr TValue checked_cast(TFrom from)
#else
template <typename T>
constexpr T checked_cast(auto from)
#endif
{
    static_assert(std::is_integral_v<decltype(from)>);
    static_assert(std::is_integral_v<TValue>);
    if (from > std::numeric_limits<TValue>::max())
        throw std::overflow_error(std::to_string(from));
    if (from < std::numeric_limits<TValue>::lowest())
        throw std::underflow_error(std::to_string(from));
    return static_cast<TValue>(from);
}

template <typename Elem, typename Traits >
void check_nulterm(std::basic_string_view<Elem, Traits> sv)
{
    constexpr static auto npos = std::basic_string_view<Elem, Traits>::npos;
    assert(sv.find_last_of(static_cast<Elem>(0)) != npos);
}

#if !__cpp_lib_to_array
namespace std {
    namespace {
        template <class _Ty, size_t _Size, size_t... _Idx>
        [[nodiscard]]
        constexpr array<remove_cv_t<_Ty>, _Size> to_array_lvalue_impl(
            _Ty(&_Array)[_Size], index_sequence<_Idx...>)
        {
            return { {_Array[_Idx]...} };
        }

        template <class _Ty, size_t _Size, size_t... _Idx>
        [[nodiscard]]
        constexpr array<remove_cv_t<_Ty>, _Size> to_array_rvalue_impl(
            _Ty(&&_Array)[_Size], index_sequence<_Idx...>)
        {
            return { {move(_Array[_Idx])...} };
        }
    }

    template<class TValue, size_t N>
    [[nodiscard]]
    constexpr array<remove_cv_t<TValue>, N> to_array(TValue(&a)[N]) { return to_array_lvalue_impl(a, make_index_sequence<N>{}); }
    template<class TValue, size_t N>
    [[nodiscard]]
    constexpr array<remove_cv_t<TValue>, N> to_array(TValue(&&a)[N]) { return to_array_rvalue_impl(move(a), make_index_sequence<N>{}); }
}
#endif

using std::begin, std::end;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

constexpr DWORD MB(ULONGLONG val) { return checked_cast<DWORD>(val / 1024 / 1024); }

template <class TElem>
constexpr std::basic_string<TElem> operator +(std::basic_string_view< TElem> left, std::basic_string< TElem>&& right)
{
    return right.insert(0, left);
}

template <class TElem>
constexpr std::basic_string<TElem> operator +(std::basic_string<TElem>&& left, std::basic_string_view< TElem> right)
{
    return left.append(right);
}

struct Config
{
    Config()
    {
        THROW_IF_WIN32_BOOL_FALSE(GlobalMemoryStatusEx(&memStatus));
        DWORD TotalPageFileInfMB = MB(memStatus.ullTotalPageFile);
        processMemory = { TotalPageFileInfMB,TotalPageFileInfMB, 1 };
    }

    std::wstring commandPath;
    std::wstring commandParametor;

    int cpuLimitType = 0;
    StaticLimitedInt<WORD, 100, 1> cpuRateMin = 1, cpuRateMax = 100;
    StaticLimitedInt<DWORD, 9, 1> cpuRateWeight = 5;
    StaticLimitedInt<DWORD, 10000, 1> cpuRate = 10000;
    bool cpuHardCap = true;
    constexpr static DWORD LimitPerProcess = (0x80000000000ULL / 1024 / 1024);
    LimitedInt<DWORD> processMemory;
    bool jobMemory = true;
    int bandWidthScale = 0;
    StaticLimitedInt<DWORD, 1023, 0> bandWidth = 1023;

    MEMORYSTATUSEX memStatus{ sizeof(memStatus) };

    void restoreDefault()
    {
        *this = Config();
    }

    constexpr static LPCWSTR appName = L"";

    void save(std::wstring_view path) const
    {
        check_nulterm(path);
        WritePrivateProfileString(appName, TEXT(nameof(cpuLimitType)), std::to_wstring(cpuLimitType).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuRateMax)), std::to_wstring(cpuRateMax).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuRateMin)), std::to_wstring(cpuRateMin).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuRateWeight)), std::to_wstring(cpuRateWeight).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuRate)), std::to_wstring(cpuRate).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuHardCap)), std::to_wstring(cpuHardCap).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(processMemory)), std::to_wstring(processMemory).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(jobMemory)), std::to_wstring(jobMemory).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(bandWidthScale)), std::to_wstring(bandWidthScale).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(bandWidth)), std::to_wstring(bandWidth).c_str(), path.data());
    }

    void load(std::wstring_view path)
    {
        check_nulterm(path);
        *this = Config();
        commandPath = GetPrivateProfileString(appName, TEXT(nameof(commandPath)), commandPath.c_str(), path.data());
        commandParametor = GetPrivateProfileString(appName, TEXT(nameof(commandParametor)), commandParametor.c_str(), path.data());
        cpuLimitType = GetPrivateProfileInt(appName, TEXT(nameof(cpuLimitType)), cpuLimitType, path.data());
        cpuRateMax = checked_cast<WORD>(GetPrivateProfileInt(appName, TEXT(nameof(cpuRateMax)), cpuRateMax, path.data()));
        cpuRateMin = checked_cast<WORD>(GetPrivateProfileInt(appName, TEXT(nameof(cpuRateMin)), cpuRateMin, path.data()));
        cpuRateWeight = GetPrivateProfileInt(appName, TEXT(nameof(cpuRateWeight)), cpuRateWeight, path.data());
        cpuRate = GetPrivateProfileInt(appName, TEXT(nameof(cpuRate)), cpuRate, path.data());
        cpuHardCap = GetPrivateProfileInt(appName, TEXT(nameof(cpuHardCap)), cpuHardCap, path.data());
        processMemory = GetPrivateProfileInt(appName, TEXT(nameof(processMemory)), processMemory, path.data());
        jobMemory = GetPrivateProfileInt(appName, TEXT(nameof(jobMemory)), jobMemory, path.data());
        bandWidthScale = GetPrivateProfileInt(appName, TEXT(nameof(bandWidthScale)), bandWidthScale, path.data());
        bandWidth = GetPrivateProfileInt(appName, TEXT(nameof(bandWidth)), bandWidth, path.data());
    }
private:
    static std::wstring GetPrivateProfileString(
        _In_opt_ LPCWSTR lpAppName,
        _In_opt_ LPCWSTR lpKeyName,
        _In_opt_ LPCWSTR lpDefault,
        _In_opt_ LPCWSTR lpFileName)
    {
        auto size = ::GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, nullptr, 0, lpFileName);
        std::wstring buf(size, L'\0');
        buf.resize(::GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, buf.data(), size, lpFileName));
        return buf;
    }
};

template <size_t size>
struct RadioGroup
{
    const std::array<int, size> group;
    using iterator = typename decltype(group)::const_iterator;

    void check(HWND hDlg, iterator checkId) const
    {
        assert(checkId != end(group));
        CheckRadioButton(hDlg, group[0], group[group.size() - 1], *checkId);
    }

    auto getChecked(HWND hDlg) const
    {
        return std::find_if(begin(group), end(group), [=](int ctrlId) { IsDlgButtonChecked(hDlg, ctrlId) == BST_CHECKED; });
    }

    auto indexOf(int ctrlId) const { return std::find(begin(group), end(group), ctrlId); }

    constexpr iterator first() const { return begin(group); }
    constexpr iterator last() const { return end(group) - 1; }

    operator std::array<int, size>() const { return group; }
};

template <size_t size>
constexpr RadioGroup<size> make_radioGroup(std::array<int, size>&& group) { return RadioGroup<size>{ group }; }

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
    constexpr static inline auto radioGroupCpuLimit = make_radioGroup(std::to_array(
        { IDC_RADIO_CPU_NOLIMIT, IDC_RADIO_CPU_WEIGHT_BASED, IDC_RADIO_CPU_MIN_MAX_RATE, IDC_RADIO_CPU_RATE }
    ));

    constexpr static auto radioGroupBandwidth = make_radioGroup(std::to_array(
        { IDC_RADIO_BANDWIDTH, IDC_RADIO_BANDWIDTH_KB, IDC_RADIO_BANDWIDTH_MB, IDC_RADIO_BANDWIDTH_GB }
    ));

    CMainDialog(HINSTANCE hInst, Config& config) : config_(config)
        , hMenu_(LoadMenu(hInst, MAKEINTRESOURCE(IDC_CPURATELIMIT)))
    {
        assert(hMenu_);
    }
    ~CMainDialog() noexcept
    {
        DestroyMenu(hMenu_);
    }

    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        constexpr static auto getUserData = [](HWND hDlg) {return reinterpret_cast<CMainDialog*>(GetWindowLongPtr(hDlg, GWLP_USERDATA)); };

        switch (message)
        {
        case WM_INITDIALOG:
        {
            CMainDialog* self{ reinterpret_cast<CMainDialog*>(lParam) };
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
            assert(self->hDlg_ == nullptr);
            self->hDlg_ = hDlg;

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
        affinityCheckBoxes_[0] = check;
        auto[pos, size] = my::getWindowRect(check);
        THROW_IF_WIN32_BOOL_FALSE(ScreenToClient(hDlg_, &pos));

        for (DWORD i = 1; i < sizeof(DWORD_PTR) * 8; i++)
        {
            POINT pos1{ pos.x + ((size.cx + 3) * ((int)i % 16)), pos.y + (size.cy + 1) * ((int)i / 16) };
            affinityCheckBoxes_[i] = my::copyDlgItem(check, std::to_wstring(i).c_str(), pos1, size);
        }
        if (args_.size() > 0)
            put_path(args_[0]);

        setValues();
    }

    void setValues()
    {
        radioGroupCpuLimit.check(hDlg_, radioGroupCpuLimit.first());

        SetDlgItemText(hDlg_, IDC_EDIT_PATH, config_.commandPath.c_str());
        SetDlgItemText(hDlg_, IDC_EDIT_PARAM, config_.commandParametor.c_str());

        radioGroupCpuLimit.check(hDlg_, radioGroupCpuLimit.first() + config_.cpuLimitType);

        setTrackbarFrom(IDC_SLIDER_CPU_RATE, config_.cpuRate);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE, config_.cpuRate, FALSE);
        TrackBaar_SetBuddy(getDlgItem(IDC_SLIDER_CPU_RATE), FALSE, getDlgItem(IDC_EDIT_CPU_RATE));

        Edit_LimitText(getDlgItem(IDC_EDIT_CPU_RATE_MIN), config_.cpuRateMin.log10OfMaxValue() + 1);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE_MIN, config_.cpuRateMin, FALSE);

        Edit_LimitText(getDlgItem(IDC_EDIT_CPU_RATE_MAX), config_.cpuRateMax.log10OfMaxValue() + 1);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE_MAX, config_.cpuRateMax, FALSE);

        setTrackbarFrom(IDC_SLIDER_CPU_RATE_WEIGHT, config_.cpuRateWeight);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE_WEIGHT, config_.cpuRateWeight, FALSE);
        TrackBaar_SetBuddy(getDlgItem(IDC_SLIDER_CPU_RATE_WEIGHT), FALSE, getDlgItem(IDC_EDIT_CPU_RATE_WEIGHT));

        setTrackbarFrom(IDC_SLIDER_MEMORY_LIMIT, config_.processMemory, 100);
        SetDlgItemInt(hDlg_, IDC_EDIT_MEMORY_LIMIT, MB(config_.memStatus.ullAvailPageFile), FALSE);
        TrackBaar_SetSelStart(getDlgItem(IDC_SLIDER_MEMORY_LIMIT), FALSE, 0);
        TrackBaar_SetSelEnd(getDlgItem(IDC_SLIDER_MEMORY_LIMIT), FALSE, MB(config_.memStatus.ullTotalPhys));

        setTrackbarFrom(IDC_SLIDER_BANDWIDTH_LIMIT, config_.bandWidth, 100);
        SetDlgItemInt(hDlg_, IDC_EDIT_BANDWIDTH_LIMIT, config_.bandWidth, FALSE);
        radioGroupBandwidth.check(hDlg_, radioGroupBandwidth.last());

        DWORD_PTR processAffinityMask, systemAffinityMask;
        GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
        put_affinityMask(systemAffinityMask);

    }

    [[nodiscard]]
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
            auto process = my::invokeProcess(path.c_str(), my::getDlgItemText(hDlg_, IDC_EDIT_PARAM).c_str());
            job.assignProcess(process.get());
            WaitForInputIdle(process.get(), 10000);
            auto pid = GetProcessId(process.get());
            MessageBox(hDlg_,
                (L"Process ran with PID: "sv + std::to_wstring(pid) + L"\n"
                    L"While this dialog is displayed, the process will be constrained."sv).c_str(),
                my::getWindowText(hDlg_).c_str(), MB_ICONINFORMATION);
            return true;
        }
        case IDC_LOAD_DEFAULT:
        {
            config_.restoreDefault();
            setValues();
            return true;
        }
        case IDC_SPLIT_FILE_SELECT:
        {
            auto path = openFileDialog(hDlg_, get_path());
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
                UINT val = GetDlgItemInt(hDlg_, ctrlId, &transelated, FALSE);

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

    [[nodiscard]]
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
            auto[p, size] = my::getWindowRect(getDlgItem(IDC_SPLIT_FILE_SELECT));
            p.y += size.cy;

            TrackPopupMenu(GetSubMenu(hMenu_, 0), TPM_LEFTALIGN | TPM_TOPALIGN, p.x, p.y, 0, hDlg_, nullptr);
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
            SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE_WEIGHT, value, FALSE);
        }
        else if (ctrlhWnd == getDlgItem(IDC_SLIDER_CPU_RATE))
        {
            auto value = getTrackBarPos(IDC_SLIDER_CPU_RATE);
            SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE, value, FALSE);
        }
        else if (ctrlhWnd == getDlgItem(IDC_SLIDER_BANDWIDTH_LIMIT))
        {
            auto value = getTrackBarPos(IDC_SLIDER_BANDWIDTH_LIMIT);
            SetDlgItemInt(hDlg_, IDC_EDIT_BANDWIDTH_LIMIT, value, FALSE);
        }
        else if (ctrlhWnd == getDlgItem(IDC_SLIDER_MEMORY_LIMIT))
        {
            auto value = getTrackBarPos(IDC_SLIDER_MEMORY_LIMIT);
            SetDlgItemInt(hDlg_, IDC_EDIT_MEMORY_LIMIT, value, FALSE);
        }
    }
    HWND hDlg_{};
    HMENU hMenu_{};
    std::vector<LPCWSTR> args_;
    std::array<HWND, sizeof(DWORD_PTR) * 8> affinityCheckBoxes_{};
    Config& config_;

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
            val = config_.cpuRateWeight;
            return true;
        case IDC_EDIT_CPU_RATE_MAX:
            val = config_.cpuRateMax;
            return true;
        case IDC_EDIT_CPU_RATE_MIN:
            val = config_.cpuRateMin;
            return true;
        default:
            return false;
        }
    }

    [[nodiscard]]
    std::wstring get_path()
    {
        return my::getDlgItemText(hDlg_, IDC_EDIT_PATH);
    }

    void put_path(std::wstring_view s)
    {
        check_nulterm(s);
        SetDlgItemText(hDlg_, IDC_EDIT_PATH, s.data());
    }

    [[nodiscard]]
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

    [[nodiscard]]
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

    [[nodiscard]]
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

    [[nodiscard]]
    DWORD_PTR get_affinityMask()
    {
        DWORD_PTR mask = 0;
        for (DWORD i = 0; i < affinityCheckBoxes_.size(); i++)
            if (auto hWnd = affinityCheckBoxes_[i])
                mask |= Button_GetCheck(hWnd) ? ((DWORD_PTR)1 << i) : 0;
        return mask;
    }
    void put_affinityMask(DWORD_PTR mask)
    {
        for (DWORD i = 0; i < affinityCheckBoxes_.size(); i++)
        {
            if (auto hWnd = affinityCheckBoxes_[i])
            {
                Button_SetCheck(hWnd, (mask & ((DWORD_PTR)1 << i) ? BST_CHECKED : BST_UNCHECKED));
                Button_Enable(hWnd, (mask & ((DWORD_PTR)1 << i)));
            }
        }
    }

    [[nodiscard]]
    int getTrackBarPos(int ctrlId)
    {
        return static_cast<int>(TrackBaar_GetPos(getDlgItem(ctrlId)));
    }

    [[nodiscard]]
    int getDlgItemInt(int ctrlId)
    {
        BOOL b;
        int val = GetDlgItemInt(hDlg_, ctrlId, &b, TRUE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    [[nodiscard]]
    unsigned  getDlgItemUInt(int ctrlId)
    {
        BOOL b;
        unsigned  val = GetDlgItemInt(hDlg_, ctrlId, &b, FALSE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    [[nodiscard]]
    HWND getDlgItem(int ctrlId)
    {
        HWND hWnd = GetDlgItem(hDlg_, ctrlId);
        THROW_IF_NULL_ALLOC(hWnd);
        return hWnd;
    }

public:

    static INT_PTR showModal(HINSTANCE hInst, LPCWSTR lpCmdLine)
    {
        int argc;
        auto argv = CommandLineToArgvW(lpCmdLine, &argc);
        argv++; argc--;

        auto inipath = my::changeExtention(my::getModuleFileName(), L".ini"sv);
        Config config{};
        config.load(inipath);
        CMainDialog self{ hInst, config };
        self.args_ = { argv , argv + argc };

        return DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), nullptr, &DlgProc, reinterpret_cast<LPARAM>(&self));
    }

};

