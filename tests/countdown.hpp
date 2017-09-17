// countdown.hpp
//
#pragma once
namespace tools {

struct Countdown {
public:
    explicit constexpr Countdown(const std::size_t From) noexcept
        : Value(From) {}

    constexpr operator std::size_t() const noexcept { return Value; }

    std::size_t operator--() noexcept {
        if (Value) Value--;
        return Value;
    }

    std::size_t operator--(int) noexcept {
        if (Value) return --Value;
        return Value;
    }

private:
    std::size_t Value;
};

} // tools
