#pragma once
#include "framework.h"
#include "LimitedInt.h"

#include <string>
#include <string_view>
#include <cassert>

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
    constexpr static const auto npos = std::basic_string_view<Elem, Traits>::npos;
    assert(sv.find_last_of(static_cast<Elem>(0)) != npos);
}

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

constexpr inline DWORD toMB(ULONGLONG val) { return checked_cast<DWORD>(val / 1024 / 1024); }

inline DWORD_PTR getSystemAffinityMask()
{
    DWORD_PTR processAffinityMask, systemAffinityMask;
    THROW_IF_WIN32_BOOL_FALSE(GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask));
    return systemAffinityMask;
}

inline std::wstring to_wstring_hex(unsigned long long value)
{
    WCHAR buf[sizeof(value) * 2 + 1];
    size_t n = swprintf(buf, ARRAYSIZE(buf), L"%llx", value);
    return { buf, n };
}

enum /*class*/ BandwidthScale : unsigned
{
    BandwidthScaleb = 0,
    BandwidthScaleKb = 1,
    BandwidthScaleMb = 2,
    BandwidthScaleGb = 3,
    BandwidthScaleMax = BandwidthScaleGb,
};

enum /*class*/ CpuLimitType : unsigned
{
    CpuLimitTypeNone = 0,
    CpuLimitTypeWaightBase,
    CpuLimitTypeMinMAxRate,
    CpuLimitTypeRate,
    CpuLimitTypeMax = CpuLimitTypeRate,
};

struct Config
{
    constexpr static const LPCWSTR appName = L"cpuratelimit";
    constexpr static DWORD LimitPerProcess = (0x80000000000ULL / 1024 / 1024);

    std::wstring commandPath;
    std::wstring commandParametor;

    ULONG_PTR affinityMask = MAXULONG_PTR;
    StaticLimitedInt<CpuLimitType, CpuLimitTypeMax, CpuLimitTypeNone> cpuLimitType = CpuLimitTypeNone;
    StaticLimitedInt<WORD, 100, 1> cpuRateMin = 1, cpuRateMax = 100;
    StaticLimitedInt<DWORD, 9, 1> cpuRateWeight = 5;
    StaticLimitedInt<DWORD, 10000, 1> cpuRate = 10000;
    bool cpuHardCap = true;
    StaticLimitedInt<DWORD, LimitPerProcess, 1> processMemory = LimitPerProcess;
    bool jobMemory = true;
    StaticLimitedInt< BandwidthScale, BandwidthScaleMax, BandwidthScaleb > bandWidthScale = BandwidthScaleGb;
    StaticLimitedInt<DWORD, 1023, 0> bandWidth = 1023;

    MEMORYSTATUSEX memStatus = getMmStatus();

    void restoreDefault()
    {
        *this = Config();
    }

    void save(std::wstring_view path) const
    {
        check_nulterm(path);

        WritePrivateProfileString(appName, TEXT(nameof(commandPath)), commandPath.c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(commandParametor)), commandParametor.c_str(), path.data());
        WritePrivateProfileLongLong(appName, TEXT(nameof(affinityMask)), affinityMask, path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuLimitType)), std::to_wstring(cpuLimitType).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuRateMin)), std::to_wstring(cpuRateMin).c_str(), path.data());
        WritePrivateProfileString(appName, TEXT(nameof(cpuRateMax)), std::to_wstring(cpuRateMax).c_str(), path.data());
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
        *this = Config
        {
            DESIGNED_INIT(.commandPath = ) GetPrivateProfileString(appName, TEXT(nameof(commandPath)), commandPath.c_str(), path.data()),
            DESIGNED_INIT(.commandParametor = ) GetPrivateProfileString(appName, TEXT(nameof(commandParametor)), commandParametor.c_str(), path.data()),
            DESIGNED_INIT(.affinityMask = ) static_cast<ULONG_PTR>(GetPrivateProfileLongLong(appName, TEXT(nameof(affinityMask)), affinityMask, path.data())),
            DESIGNED_INIT(.cpuLimitType = ) static_cast<CpuLimitType>(GetPrivateProfileInt(appName, TEXT(nameof(cpuLimitType)), cpuLimitType, path.data())),
            DESIGNED_INIT(.cpuRateMin = ) checked_cast<WORD>(GetPrivateProfileInt(appName, TEXT(nameof(cpuRateMin)), cpuRateMin, path.data())),
            DESIGNED_INIT(.cpuRateMax = ) checked_cast<WORD>(GetPrivateProfileInt(appName, TEXT(nameof(cpuRateMax)), cpuRateMax, path.data())),
            DESIGNED_INIT(.cpuRateWeight = ) GetPrivateProfileInt(appName, TEXT(nameof(cpuRateWeight)), cpuRateWeight, path.data()),
            DESIGNED_INIT(.cpuRate = ) GetPrivateProfileInt(appName, TEXT(nameof(cpuRate)), cpuRate, path.data()),
            DESIGNED_INIT(.cpuHardCap = ) GetPrivateProfileInt(appName, TEXT(nameof(cpuHardCap)), cpuHardCap, path.data()) != FALSE,
            DESIGNED_INIT(.processMemory = ) GetPrivateProfileInt(appName, TEXT(nameof(processMemory)), processMemory, path.data()),
            DESIGNED_INIT(.jobMemory = ) GetPrivateProfileInt(appName, TEXT(nameof(jobMemory)), jobMemory, path.data()) != FALSE,
            DESIGNED_INIT(.bandWidthScale = ) static_cast<BandwidthScale>(GetPrivateProfileInt(appName, TEXT(nameof(bandWidthScale)), bandWidthScale, path.data())),
            DESIGNED_INIT(.bandWidth = ) GetPrivateProfileInt(appName, TEXT(nameof(bandWidth)), bandWidth, path.data()),
        };
    }

    [[nodiscard]]
    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION get_cpuRateControlInfo() const
    {
        JOBOBJECT_CPU_RATE_CONTROL_INFORMATION info{};

        switch (cpuLimitType)
        {
        case CpuLimitTypeRate:
            info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
            info.CpuRate = cpuRate;
            break;
        case CpuLimitTypeWaightBase:
            info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_WEIGHT_BASED;
            info.ControlFlags |= cpuHardCap ? JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP : 0;
            info.Weight = cpuRateWeight;
            break;
        case CpuLimitTypeMinMAxRate:
            info.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE;
            info.MaxRate = cpuRateMax;
            info.MinRate = cpuRateMin;
            break;
        case CpuLimitTypeNone:
        default:
            break;
        }
        return info;
    }

    [[nodiscard]]
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION get_extendedLimitInfo(unsigned max) const
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
        auto pos = processMemory;
        if (pos < max)
        {
            if (jobMemory)
            {
                info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
                info.JobMemoryLimit = processMemory * 1024ULL * 1024U;
            }
            else
            {
                info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
                info.ProcessMemoryLimit = processMemory * 1024ULL * 1024U;
            }
        }

        if (getSystemAffinityMask() != affinityMask)
        {
            info.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
            info.BasicLimitInformation.Affinity = affinityMask;
        }

        return info;
    }

    [[nodiscard]]
    JOBOBJECT_NET_RATE_CONTROL_INFORMATION get_netRateControlInfo() const
    {
        if (bandWidthScale == BandwidthScaleGb && bandWidth == bandWidth.max_value())
        {
            return {};
        }
        JOBOBJECT_NET_RATE_CONTROL_INFORMATION info{};
        info.ControlFlags = JOB_OBJECT_NET_RATE_CONTROL_ENABLE | JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH;
        info.MaxBandwidth = bandWidth *
            bandWidthScale == BandwidthScaleGb ? 1024ULL * 1024 * 1024 :
            bandWidthScale == BandwidthScaleMb ? 1024ULL * 1024 :
            bandWidthScale == BandwidthScaleKb ? 1024ULL :
            1;
        return info;
    }

private:
    static MEMORYSTATUSEX getMmStatus()
    {
        MEMORYSTATUSEX memStatus{ sizeof(memStatus) };
        THROW_IF_WIN32_BOOL_FALSE(GlobalMemoryStatusEx(&memStatus));
        return memStatus;
    }

    static std::wstring GetPrivateProfileString(
        _In_opt_ LPCWSTR lpAppName,
        _In_opt_ LPCWSTR lpKeyName,
        _In_opt_ LPCWSTR lpDefault,
        _In_opt_ LPCWSTR lpFileName)
    {
        std::wstring buf(512, L'\0');
        auto size = ::GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, buf.data(), buf.size(), lpFileName);
        if (size == 0)
            return lpDefault;
        if (size == buf.size() - 1)
            buf.resize(size + 256);
        buf.resize(::GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, buf.data(), size + 1, lpFileName));
        return buf;
    }

    static void WritePrivateProfileLongLong(
        _In_opt_ LPCWSTR lpAppName,
        _In_opt_ LPCWSTR lpKeyName,
        ULONGLONG value,
        _In_opt_ LPCWSTR lpFileName)
    {
        auto hex = (L"0x"sv + to_wstring_hex(value));
        WritePrivateProfileString(lpAppName, lpKeyName, hex.c_str(), lpFileName);
    }
    static ULONGLONG GetPrivateProfileLongLong(
        _In_opt_ LPCWSTR lpAppName,
        _In_opt_ LPCWSTR lpKeyName,
        ULONGLONG ullDefault,
        _In_opt_ LPCWSTR lpFileName)
    {
        std::wstring buf(512, L'\0');
        auto size = ::GetPrivateProfileString(lpAppName, lpKeyName, nullptr, buf.data(), buf.size(), lpFileName);
        if (size == 0)
            return ullDefault;
        return std::stoull(buf, nullptr, 0);
    }
};
