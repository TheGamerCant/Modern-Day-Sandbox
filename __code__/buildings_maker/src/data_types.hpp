#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <chrono>
#include <array>

//  ----Rename Types----

using Char = char;
using UnsignedChar = unsigned char;

using UnsignedInteger8 = uint8_t;
using UnsignedInteger16 = uint16_t;
using UnsignedInteger32 = uint32_t;
using UnsignedInteger64 = uint64_t;

using SignedInteger8 = int8_t;
using SignedInteger16 = int16_t;
using SignedInteger32 = int32_t;
using SignedInteger64 = int64_t;

using SizeT = size_t;

using Float32 = float;
using Float64 = double;

using Boolean = bool;

using String = std::string;

using Path = std::filesystem::path;

using Timestamp = std::chrono::steady_clock::time_point;

template<typename HashKey, typename HashValue>
using HashMap = std::unordered_map<HashKey, HashValue>;

template<typename SetType>
using Set = std::unordered_set<SetType>;

template<typename VectorType>
using Vector = std::vector<VectorType>;

template<typename ArrayType, SizeT amount>
using Array = std::array<ArrayType, amount>;

//Colour Structs
struct ColourRGB;
struct ColourRGBA;

struct ColourRGB {
public:
    UnsignedInteger8 r, g, b;

    ColourRGB();
    ColourRGB(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b);
    ColourRGB(const String& str);
    ColourRGB(const ColourRGBA rgba);

    bool operator==(const ColourRGB& other) const noexcept {
        return r == other.r && g == other.g && b == other.b;
    }

    UnsignedInteger32 ToInteger();
    const UnsignedInteger32 ToInteger() const;
    String ToHex();
    const String ToHex() const;
};

struct ColourRGBA {
public:
    UnsignedInteger8 r, g, b, a;

    ColourRGBA();
    ColourRGBA(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b);
    ColourRGBA(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b, const UnsignedInteger8 a);
    ColourRGBA(const String& str);
    ColourRGBA(const ColourRGB rgb);

    bool operator==(const ColourRGBA& other) const noexcept {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    UnsignedInteger32 ToInteger();
    const UnsignedInteger32 ToInteger() const;
    String ToHex();
    const String ToHex() const;
};
