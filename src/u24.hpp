#pragma once
#include <cstdint>
#include <bit>

namespace impl {

    /**
        * A sane 24-bit integer type.
        */
    template<std::endian Endian = std::endian::little>
    struct U24Impl {
        constexpr U24Impl() = default;

        constexpr U24Impl(std::uint32_t value) {
            set(value);
        }

        constexpr operator std::uint32_t() const { // NOLINT (We want this to be implicitly convertiable in expressions)
            return get();
        }

        constexpr U24Impl& operator=(std::uint32_t val) {
            set(val);
            return *this;
        }

#define SIMPLE_BINOP(op) \
        constexpr U24Impl& operator op (const U24Impl& other) { \
            set(get() op other.get()); \
            return *this; \
        } \
        constexpr U24Impl& operator op (const std::uint32_t other) { \
            set(get() op other); \
            return *this; \
        }

        SIMPLE_BINOP(+)
        SIMPLE_BINOP(-)
        SIMPLE_BINOP(*)
        SIMPLE_BINOP(/)

        // Simple binary operations.
        SIMPLE_BINOP(^)
        SIMPLE_BINOP(&)
        SIMPLE_BINOP(|)

    private: // TODO: Public?
        constexpr void set(std::uint32_t val) {
            if constexpr(Endian == std::endian::big) {
                bytes_[0] = (val >> 16) & 0xFF;
                bytes_[1] = (val >> 8) & 0xFF;
                bytes_[2] = val & 0xFF;
            } else {
                bytes_[2] = (val >> 16) & 0xFF;
                bytes_[1] = (val >> 8) & 0xFF;
                bytes_[0] = val & 0xFF;
            }
        }

        constexpr std::uint32_t get() const {
            if constexpr(Endian == std::endian::big) {
                return bytes_[0] << 16 | bytes_[1] << 8 | bytes_[2];
            } else {
                return bytes_[2] << 16 | bytes_[1] << 8 | bytes_[0];
            }
        }
        // NO TOUCHY
        std::uint8_t bytes_[3];
    };

    // sanity checks
    static_assert(sizeof(U24Impl<>) == 3, "Your compiler sucks and U24Impl<> somehow isn't 3 bytes on it. Get a better one");
} // namespace detail

// uint24 types
template<std::endian Endian>
using u24Endian = impl::U24Impl<Endian>;

using u24 = impl::U24Impl<std::endian::native>;

