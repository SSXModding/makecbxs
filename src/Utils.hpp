#pragma once
#include <iostream>
#include <bit>
#include <cstdint>
#include <cassert>

// TODO: mcolib2
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;
using usize = std::size_t;


template <typename T>
inline T ReadUserType(std::istream& stream) {
    T temp {};
    if(stream) {
        stream.read(reinterpret_cast<char*>(&temp), sizeof(T));
    }

    if(!stream) {
        throw std::runtime_error("Attemped to read invalid stream");
    }

    return temp;
}

// TODO: We have u24 now, we don't really need these

template<std::endian Endian = std::endian::little>
std::uint32_t ReadU24(std::istream& stream) {
    std::uint8_t buf[3]{};
    for(int i = 0; i < 3; ++i)
        buf[i] = stream.get();

    switch(Endian) {
        case std::endian::little:
            return ((buf[2] << 16) | (buf[1] << 8) | buf[0]);
        case std::endian::big:
            return ((buf[0] << 16) | (buf[1] << 8) | buf[2]);
        default:
            assert(false);
            break;
    }
}

template<std::endian Endian = std::endian::little>
std::uint32_t ReadU24FromPtr(const std::uint8_t* buf) {
    switch(Endian) {
        case std::endian::little:
            return ((buf[2] << 16) | (buf[1] << 8) | buf[0]);
        case std::endian::big:
            return ((buf[0] << 16) | (buf[1] << 8) | buf[2]);
        default:
            assert(false);
            break;
    }
}
