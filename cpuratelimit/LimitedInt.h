#pragma once
#include <limits>
#include <string>
#include <stdexcept>

template <class TElem>
constexpr inline std::basic_string<TElem> operator +(std::basic_string_view< TElem> left, std::basic_string< TElem>&& right)
{
    return right.insert(0, left);
}

template <class TElem>
constexpr inline std::basic_string<TElem> operator +(std::basic_string<TElem>&& left, std::basic_string_view< TElem> right)
{
    return left.append(right);
}

inline namespace {
    constexpr inline unsigned log10int(unsigned long long val)
    {
        if (val < 10)
            return 0;
        else
            return log10int(val / 10) + 1;
    }

    template <class TBase, TBase Tmax, TBase Tmin = std::numeric_limits<TBase>::min()>
    struct StaticLimitedInt
    {
        static_assert(std::is_arithmetic_v<TBase> || std::is_enum_v <TBase>);
    private:
        TBase value;
        constexpr static void verify(TBase value)
        {
            if (value < min_value()) throw std::underflow_error{ std::to_string(value) + " < " + std::to_string(min_value()) };
            if (value > max_value()) throw std::overflow_error{ std::to_string(value) + " > " + std::to_string(max_value()) };
        }
    public:
        using base_type = typename TBase;
        constexpr static TBase max_value() { return Tmax; }
        constexpr static TBase min_value() { return Tmin; }

        constexpr StaticLimitedInt(TBase v) : value(v)
        {
            //static_assert(std::is_integral_v<TBase>);
            static_assert(Tmin < Tmax);

            verify(v);
        }

        constexpr StaticLimitedInt& operator=(TBase v)
        {
            verify(v);

            value = v;
            return *this;
        }

        constexpr TBase get() const { return value; }
        constexpr operator TBase() const { return value; }
        constexpr const TBase* operator &() const { return &value; }
        constexpr explicit operator bool() const = delete;

        constexpr static bool canConvertFrom(TBase v) { return min_value() <= v && v <= max_value(); }
        constexpr static int log10OfMaxValue() { return log10int(max_value()); }
    };

    template <class TBase = int>
    struct LimitedInt
    {
        using base_type = typename TBase;
    private:
        TBase value;
        TBase max_value_;
        TBase min_value_;
    public:
        constexpr LimitedInt(TBase v = {}, TBase max = std::numeric_limits< TBase>::max(), TBase min = std::numeric_limits< TBase>::lowest())
            : value(v), max_value_(max), min_value_(min)
        {
            if (v < min_value_) throw std::underflow_error{ std::to_string(v) + " < " + std::to_string(min_value()) };
            if (v > max_value_) throw std::overflow_error{ std::to_string(v) + " > " + std::to_string(max_value()) };
        }

        template <class TValue, TValue Tmax, TValue Tmin>
        constexpr LimitedInt(StaticLimitedInt<TValue, Tmax, Tmin> other) : value(other), max_value_(Tmax), min_value_(Tmin)
        {
            static_assert(std::is_convertible_v<TValue, TBase>);
        }

        template <class TValue, TValue Tmax, TValue Tmin>
        constexpr LimitedInt& operator=(StaticLimitedInt<TValue, Tmax, Tmin> other)
        {
            value = other;
            max_value_ = other.max_value();
            min_value_ = other.min_value();
            return *this;
        }

        constexpr TBase max_value() const { return max_value_; }
        constexpr TBase min_value() const { return min_value_; }

        constexpr operator TBase() const { return value; }
        constexpr const TBase* operator &() const { return &value; }
        constexpr explicit operator bool() const = delete;

        constexpr bool canConvertFrom(TBase v) const { return min_value_ <= v && v <= max_value_; }
        constexpr int log10OfMaxValue() const { return log10int(max_value_); }
    };
}
