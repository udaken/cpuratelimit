#pragma once
#include "framework.h"
#include "Config.h"
#include "JobObject.h"

#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>
#include <cassert>

#define TrackBaar_GetRangeMax(hwndCtrl) (int)SNDMSG((hwndCtrl), TBM_GETRANGEMAX, 0, 0)
#define TrackBaar_GetPos(hwndCtrl) (int)SNDMSG((hwndCtrl), TBM_GETPOS, 0, 0)
#define TrackBaar_SetPos(hwndCtrl, redraw, pos) (void)SNDMSG((hwndCtrl), TBM_SETPOS, (BOOL)(redraw), (pos))
#define TrackBaar_SetBuddy(hwndCtrl, leftTop, buddyCtrl) (void)SNDMSG((hwndCtrl), TBM_SETBUDDY, (leftTop), (LPARAM)(buddyCtrl))
#define TrackBaar_SetSelStart(hwndCtrl, redraw, end) (void)SNDMSG((hwndCtrl), TBM_SETSELSTART, (BOOL)(redraw), (end))
#define TrackBaar_SetSelEnd(hwndCtrl, redraw, end) (void)SNDMSG((hwndCtrl), TBM_SETSELEND, (BOOL)(redraw), (end))

#define CATCH_MSGBOX(hWnd) \
catch (const wil::ResultException& e) \
    { \
        wchar_t message[2048]; \
        wil::GetFailureLogString(message, ARRAYSIZE(message), e.GetFailureInfo()); \
        MessageBox(hWnd, message, g_szTitle, MB_ICONERROR); \
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
            _Ty(&& _Array)[_Size], index_sequence<_Idx...>)
        {
            return { {move(_Array[_Idx])...} };
        }
    }

    template<class TValue, size_t N>
    [[nodiscard]]
    constexpr array<remove_cv_t<TValue>, N> to_array(TValue(&a)[N]) { return to_array_lvalue_impl(a, make_index_sequence<N>{}); }
    template<class TValue, size_t N>
    [[nodiscard]]
    constexpr array<remove_cv_t<TValue>, N> to_array(TValue(&& a)[N]) { return to_array_rvalue_impl(move(a), make_index_sequence<N>{}); }
}
#endif

using std::begin, std::end;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

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

    iterator getChecked(HWND hDlg) const
    {
        return std::find_if(begin(group), end(group), [=](int ctrlId) { return IsDlgButtonChecked(hDlg, ctrlId) == BST_CHECKED; });
    }
    size_t getCheckedAsOffset(HWND hDlg) const
    {
        return offset(getChecked(hDlg));
    }

    size_t offset(iterator i) const
    {
        assert(i != end(group));
        return std::distance(first(), i);
    }

    auto indexOf(int ctrlId) const { return std::find(begin(group), end(group), ctrlId); }

    constexpr iterator first() const { return begin(group); }
    constexpr iterator last() const { return end(group) - 1; }

    operator std::array<int, size>() const { return group; }
};

template <size_t size>
constexpr RadioGroup<size> make_radioGroup(std::array<int, size>&& group) { return RadioGroup<size>{ group }; }

enum class SubMenuId {
    Config = 0,
    Bworse,
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

class MainDialog
{
    constexpr static inline auto const radioGroupCpuLimit = make_radioGroup(std::to_array(
        { IDC_RADIO_CPU_NOLIMIT, IDC_RADIO_CPU_WEIGHT_BASED, IDC_RADIO_CPU_MIN_MAX_RATE, IDC_RADIO_CPU_RATE }
    ));

    constexpr static auto const radioGroupBandwidth = make_radioGroup(std::to_array(
        { IDC_RADIO_BANDWIDTH, IDC_RADIO_BANDWIDTH_KB, IDC_RADIO_BANDWIDTH_MB, IDC_RADIO_BANDWIDTH_GB }
    ));

    MainDialog(HINSTANCE hInst, Config& config) : config_(config)
        , hMenu_(LoadMenu(hInst, MAKEINTRESOURCE(IDR_CPURATELIMIT)))
    {
        assert(hMenu_);
    }
    ~MainDialog() noexcept
    {
        DestroyMenu(hMenu_);
    }

    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        constexpr static auto const getUserData = [](HWND hDlg) {return reinterpret_cast<MainDialog*>(GetWindowLongPtr(hDlg, GWLP_USERDATA)); };

        switch (message)
        {
        case WM_INITDIALOG:
        {
            MainDialog* self{ reinterpret_cast<MainDialog*>(lParam) };
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
        auto [pos, size] = my::getWindowRect(check);
        THROW_IF_WIN32_BOOL_FALSE(ScreenToClient(hDlg_, &pos));

        DWORD_PTR systemAffinityMask = getSystemAffinityMask();

        for (DWORD i = 1; i < sizeof(DWORD_PTR) * 8; i++)
        {
            POINT pos1{ pos.x + ((size.cx + 3) * ((int)i % 16)), pos.y + (size.cy + 1) * ((int)i / 16) };
            affinityCheckBoxes_[i] = my::copyDlgItem(check, std::to_wstring(i).c_str(), pos1, size);
            Button_Enable(affinityCheckBoxes_[i], (systemAffinityMask & ((DWORD_PTR)1 << i)));
        }
        if (args_.size() > 0)
            put_path(args_[0]);

        fromConfig();
    }

    void fromConfig()
    {
        SetDlgItemText(hDlg_, IDC_EDIT_PATH, config_.commandPath.c_str());
        SetDlgItemText(hDlg_, IDC_EDIT_PARAM, config_.commandParametor.c_str());

        put_affinityMask(getSystemAffinityMask() & config_.affinityMask);

        radioGroupCpuLimit.check(hDlg_, radioGroupCpuLimit.first() + config_.cpuLimitType);

        Edit_LimitText(getDlgItem(IDC_EDIT_CPU_RATE_MIN), config_.cpuRateMin.log10OfMaxValue() + 1);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE_MIN, config_.cpuRateMin, FALSE);

        Edit_LimitText(getDlgItem(IDC_EDIT_CPU_RATE_MAX), config_.cpuRateMax.log10OfMaxValue() + 1);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE_MAX, config_.cpuRateMax, FALSE);

        setTrackbarFrom(IDC_SLIDER_CPU_RATE_WEIGHT, config_.cpuRateWeight);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE_WEIGHT, config_.cpuRateWeight, FALSE);
        TrackBaar_SetBuddy(getDlgItem(IDC_SLIDER_CPU_RATE_WEIGHT), FALSE, getDlgItem(IDC_EDIT_CPU_RATE_WEIGHT));

        setTrackbarFrom(IDC_SLIDER_CPU_RATE, config_.cpuRate);
        SetDlgItemInt(hDlg_, IDC_EDIT_CPU_RATE, config_.cpuRate, FALSE);
        TrackBaar_SetBuddy(getDlgItem(IDC_SLIDER_CPU_RATE), FALSE, getDlgItem(IDC_EDIT_CPU_RATE));

        Button_SetCheck(getDlgItem(IDC_CHECK_CPU_HARD_CAP), config_.cpuHardCap ? BST_CHECKED : BST_UNCHECKED);

        auto mem = std::min(config_.processMemory.get(), toMB(config_.memStatus.ullTotalPageFile));
        setTrackbarFrom(IDC_SLIDER_MEMORY_LIMIT, LimitedInt{ mem, toMB(config_.memStatus.ullTotalPageFile) }, 100);
        SetDlgItemInt(hDlg_, IDC_EDIT_MEMORY_LIMIT, mem, FALSE);
        TrackBaar_SetSelStart(getDlgItem(IDC_SLIDER_MEMORY_LIMIT), FALSE, 0);
        TrackBaar_SetSelEnd(getDlgItem(IDC_SLIDER_MEMORY_LIMIT), TRUE, toMB(config_.memStatus.ullTotalPhys));

        Button_SetCheck(getDlgItem(IDC_CHECK_MEMORY_LIMIT_JOB), config_.jobMemory ? BST_CHECKED : BST_UNCHECKED);

        radioGroupBandwidth.check(hDlg_, radioGroupBandwidth.first() + config_.bandWidthScale);

        setTrackbarFrom(IDC_SLIDER_BANDWIDTH_LIMIT, config_.bandWidth, 100);
        SetDlgItemInt(hDlg_, IDC_EDIT_BANDWIDTH_LIMIT, config_.bandWidth, FALSE);
    }

    Config toConfig() const
    {
        return Config
        {
            DESIGNED_INIT(.commandPath = ) my::getDlgItemText(hDlg_, IDC_EDIT_PATH),
            DESIGNED_INIT(.commandParametor = ) my::getDlgItemText(hDlg_, IDC_EDIT_PARAM),
            DESIGNED_INIT(.affinityMask = ) get_affinityMask(),
            DESIGNED_INIT(.cpuLimitType = ) static_cast<CpuLimitType>(radioGroupCpuLimit.getCheckedAsOffset(hDlg_)),
            DESIGNED_INIT(.cpuRateMin = ) checked_cast<WORD>(getDlgItemUInt(IDC_EDIT_CPU_RATE_MIN)),
            DESIGNED_INIT(.cpuRateMax = ) checked_cast<WORD>(getDlgItemUInt(IDC_EDIT_CPU_RATE_MAX)),
            DESIGNED_INIT(.cpuRateWeight = ) getTrackBarPos(IDC_SLIDER_CPU_RATE_WEIGHT),
            DESIGNED_INIT(.cpuRate = ) getTrackBarPos(IDC_SLIDER_CPU_RATE),
            DESIGNED_INIT(.cpuHardCap = ) Button_GetCheck(getDlgItem(IDC_CHECK_CPU_HARD_CAP)) == BST_CHECKED,
            DESIGNED_INIT(.processMemory = ) getTrackBarPos(IDC_SLIDER_MEMORY_LIMIT),
            DESIGNED_INIT(.jobMemory = ) Button_GetCheck(getDlgItem(IDC_CHECK_MEMORY_LIMIT_JOB)) == BST_CHECKED,
            DESIGNED_INIT(.bandWidthScale = ) static_cast<BandwidthScale>(radioGroupBandwidth.getCheckedAsOffset(hDlg_)),
            DESIGNED_INIT(.bandWidth = ) getTrackBarPos(IDC_SLIDER_BANDWIDTH_LIMIT),
        };
    }

    static std::wstring getConfigFileName()
    {
        return my::renameExtension(my::getModuleFileName(), L".ini"sv);
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
        case IDM_CONFIG_LOAD:
        {
            auto path = showOpenDialog(hDlg_, getConfigFileName(), L"*.ini");
            if (path)
            {
                config_.load(*path);
                fromConfig();
            }
            break;
        }
        case IDM_CONFIG_SAVE:
        {
            auto path = showSaveDialog(hDlg_, getConfigFileName(), L"*.ini");
            if (path)
            {
                config_ = toConfig();
                config_.save(*path);
            }
            break;
        }
        case IDC_BUTTON_RUN:
        {
            config_ = toConfig();

            if (config_.commandPath.empty())
            {
                return true;
            }
            auto job = JobObject::create();
            job.setCpuRateControlInfo(config_.get_cpuRateControlInfo());
            auto extendLimit = config_.get_extendedLimitInfo(TrackBaar_GetRangeMax(getDlgItem(IDC_SLIDER_MEMORY_LIMIT)));
            job.setExtendLimitInfo(extendLimit);
            job.setNetRateControlInfo(config_.get_netRateControlInfo());
            auto process = my::invokeProcess(config_.commandPath, config_.commandParametor);
            job.assignProcess(process.get());
            WaitForInputIdle(process.get(), 10000);
            auto pid = GetProcessId(process.get());
            MessageBox(hDlg_,
                (L"Process ran with PID: "sv + std::to_wstring(pid) + L"\n"
                    L"While this dialog is displayed, the process will be constrained."sv).c_str(),
                my::getWindowText(hDlg_).c_str(), MB_ICONINFORMATION);
            return true;
        }
        case IDC_SPLIT_LOAD_DEFAULT:
        {
            config_.restoreDefault();
            fromConfig();
            return true;
        }
        case IDC_SPLIT_FILE_SELECT:
        {
            auto path = showOpenDialog(hDlg_, my::getDlgItemText(hDlg_, IDC_EDIT_PATH), L"*.*");
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
    CATCH_MSGBOX(hDlg_)
    catch (const std::exception& e)
    {
        MessageBoxA(hDlg_, e.what(), "cpuratelimit", MB_ICONERROR);
        return false;
    }

    [[nodiscard]]
    std::optional<std::wstring> static showSaveDialog(HWND hDlg, std::wstring&& path, LPCWSTR filter)
    {
        path.resize(MAX_PATH);

        OPENFILENAME ofn
        {
            DESIGNED_INIT(.lStructSize = )sizeof(ofn),
            DESIGNED_INIT(.hwndOwner = ) hDlg,
            DESIGNED_INIT(.hInstance = )nullptr,
            DESIGNED_INIT(.lpstrFilter = )filter,
            DESIGNED_INIT(.lpstrCustomFilter = ) nullptr,
            DESIGNED_INIT(.nMaxCustFilter = ) 0,
            DESIGNED_INIT(.nFilterIndex = )0,
            DESIGNED_INIT(.lpstrFile = ) path.data(),
            DESIGNED_INIT(.nMaxFile = ) checked_cast<DWORD>(path.size()),
        };
        if (GetSaveFileName(&ofn))
        {
            return { path };
        }
        return {};
    }

    [[nodiscard]]
    std::optional<std::wstring> static showOpenDialog(HWND hDlg, std::wstring&& path, LPCWSTR filter)
    {
        path.resize(MAX_PATH);

        OPENFILENAME ofn
        {
            DESIGNED_INIT(.lStructSize = )sizeof(ofn),
            DESIGNED_INIT(.hwndOwner = ) hDlg,
            DESIGNED_INIT(.hInstance = )nullptr,
            DESIGNED_INIT(.lpstrFilter = )filter,
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
        const static std::unordered_map<int, SubMenuId> subMenuMap
        {
            { IDC_SPLIT_FILE_SELECT, SubMenuId::Bworse },
            { IDC_SPLIT_LOAD_DEFAULT, SubMenuId::Config },
        };

        auto i = subMenuMap.find(ctrlId);
        if (i != end(subMenuMap))
        {
            auto [p, size] = my::getWindowRect(getDlgItem(ctrlId));
            p.y += size.cy;

            TrackPopupMenu(GetSubMenu(hMenu_, static_cast<int>(i->second)), TPM_LEFTALIGN | TPM_TOPALIGN, p.x, p.y, 0, hDlg_, nullptr);
        }

    }
    void onNotifyTrackBarThumbPosChanging(WORD ctrlId, NMTRBTHUMBPOSCHANGING* pnmdropdown)
    {}

    void onNotifyLink(WORD ctrlId, PNMLINK pnmLink)
    {
        if (pnmLink->item.iLink == 0)
        {
            SHELLEXECUTEINFO sei{
                DESIGNED_INIT(.cbSize = ) sizeof(sei),
                DESIGNED_INIT(.fMask = ) 0,
                DESIGNED_INIT(.hwnd = ) nullptr,
                DESIGNED_INIT(.lpVerb = ) L"open",
                DESIGNED_INIT(.lpFile = ) pnmLink->item.szUrl,
                DESIGNED_INIT(.lpParameters = ) nullptr,
                DESIGNED_INIT(.lpDirectory = ) nullptr,
                DESIGNED_INIT(.nShow = ) SW_SHOWNORMAL,
            };
            THROW_IF_WIN32_BOOL_FALSE(ShellExecuteEx(&sei));
        }
    }

    void onWmNotify(LPNMHDR lpnhdr) try
    {
        switch (lpnhdr->code)
        {
        case BCN_DROPDOWN:
            onNotifyButtonDropdown((WORD)lpnhdr->idFrom, (LPNMBCDROPDOWN)lpnhdr);
            break;
        case TRBN_THUMBPOSCHANGING:
            onNotifyTrackBarThumbPosChanging((WORD)lpnhdr->idFrom, (NMTRBTHUMBPOSCHANGING*)lpnhdr);
            break;
        case NM_CLICK:
            onNotifyLink((WORD)lpnhdr->idFrom, (PNMLINK)lpnhdr);
            break;
        default:
            break;
        }
    }
    CATCH_MSGBOX(hDlg_)
    catch (const std::exception& e)
    {
        MessageBoxA(hDlg_, e.what(), "cpuratelimit", MB_ICONERROR);
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
        TrackBaar_SetPos(getDlgItem(id), TRUE, val);
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

    void put_path(std::wstring_view s)
    {
        check_nulterm(s);
        SetDlgItemText(hDlg_, IDC_EDIT_PATH, s.data());
    }

    [[nodiscard]]
    DWORD_PTR get_affinityMask() const
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
            }
        }
    }

    [[nodiscard]]
    unsigned getTrackBarPos(int ctrlId) const
    {
        return static_cast<unsigned>(TrackBaar_GetPos(getDlgItem(ctrlId)));
    }

    [[nodiscard]]
    int getDlgItemInt(int ctrlId) const
    {
        BOOL b;
        int val = GetDlgItemInt(hDlg_, ctrlId, &b, TRUE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    [[nodiscard]]
    unsigned  getDlgItemUInt(int ctrlId) const
    {
        BOOL b;
        unsigned  val = GetDlgItemInt(hDlg_, ctrlId, &b, FALSE);
        if (b == FALSE)
            throw std::invalid_argument("");
        return val;
    }

    [[nodiscard]]
    HWND getDlgItem(int ctrlId) const
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

        auto inipath = getConfigFileName();
        Config config{};
        config.load(inipath);
        MainDialog self{ hInst, config };
        self.args_ = { argv , argv + argc };

        return DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), nullptr, &DlgProc, reinterpret_cast<LPARAM>(&self));
    }

};

