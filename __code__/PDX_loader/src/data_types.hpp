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

// ----Custom Data Structures----
struct DoubleString {
public :
    String a, b;

    DoubleString() : a(""), b("") {};
    DoubleString(const String& x, const String& y) : a(x), b(y) {};
};

// ----Decimal----
//HoI4 uses a 32-bit number to 3 decimal points but Vicky 3 uses a 64-bit to 5, so let's use that instead because why not

struct Decimal {
private:
    SignedInteger64 value;

public:
    Decimal();
    Decimal(SignedInteger32 i);
    Decimal(UnsignedInteger32 i);
    Decimal(SignedInteger64 i);
    Decimal(UnsignedInteger64 i);
    Decimal(Float32 f);
    Decimal(Float64 d);
    Decimal(String str);
    explicit Decimal(SignedInteger64 raw, bool);

    Decimal(const char* str);

    operator SignedInteger32() const;
    operator UnsignedInteger32() const;
    operator SignedInteger64() const;
    operator UnsignedInteger64() const;
    operator Float64() const;
    operator Float32() const;

    Decimal operator+(const Decimal& other) const;
    Decimal operator-(const Decimal& other) const;
    Decimal operator*(const Decimal& other) const;
    Decimal operator/(const Decimal& other) const;

    Decimal& operator+=(const Decimal& other);
    Decimal& operator-=(const Decimal& other);
    Decimal& operator*=(const Decimal& other);
    Decimal& operator/=(const Decimal& other);

    bool operator==(const Decimal& o) const;
    bool operator!=(const Decimal& o) const;
    bool operator<(const Decimal& o) const;
    bool operator<=(const Decimal& o) const;
    bool operator>(const Decimal& o) const;
    bool operator>=(const Decimal& o) const;

    SignedInteger64 GetRawValue();
    const SignedInteger64 GetRawValue() const;

    String ToString(SignedInteger16 precision = 5) const;
    friend std::ostream& operator<<(std::ostream& os, const Decimal& d);
};

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


struct Date {
private:
    SignedInteger64 hoursSinceStart;
    SignedInteger32 year;
    UnsignedInteger8 month, date, hour;

public:
    Date() : hoursSinceStart(0), year(-5000), month(1), date(1), hour(1) {};
    Date(const UnsignedInteger32 date);
    Date(const String& str);

    SignedInteger64 GetHoursSinceStart();
    const SignedInteger64 GetHoursSinceStart() const;
};

/*
Custom data type that allows indexing by index or name/tag
Requires a .GetName() function and a .SetId() function

e.g std::cout << countriesArray[8].GetTag() << ", " << countriesArray["SCT"].GetTag();
*/

template<typename DataType>
struct VectorMap {
private:
    Vector<DataType> array;
    HashMap<String, UnsignedInteger64> indexMap;

public:
    void RebuildMap() {
        indexMap.clear();
        UnsignedInteger64 i = 0;
        for (auto& obj : array) {
            obj.SetId(i);
            indexMap[obj.GetName()] = i++;
        }
    }

    void PushBack(const DataType& obj) {
        array.push_back(obj);
        indexMap[obj.GetName()] = array.size() - 1;
        array.back().SetId(array.size() - 1);
    }
    void PushBack(DataType&& obj) {
        array.push_back(std::move(obj));
        indexMap[array.back().GetName()] = array.size() - 1;
        array.back().SetId(array.size() - 1);
    }
    
    template<typename... Args>
    void EmplaceBack(Args&&... args) {
        array.emplace_back(std::forward<Args>(args)...);
        indexMap[array.back().GetName()] = array.size() - 1;
        array.back().SetId(array.size() - 1);
    }

    void Reserve(const SizeT reserve) { array.reserve(reserve); }
    SizeT Size() { return array.size(); }
    const SizeT Size() const { return array.size(); }
    SizeT Capacity() { return array.capacity(); }
    const SizeT Capacity() const { return array.capacity(); }
    void ShrinkToFit() { array.shrink_to_fit(); }

    Boolean NameInArray(const String& findString) {
        if (indexMap.find(findString) == indexMap.end()) return false;
        return true;
    }

    const Boolean NameInArray(const String& findString) const {
        if (indexMap.find(findString) == indexMap.end()) return false;
        return true;
    }

    DataType& operator[](SizeT index) { return array[index]; }
    const DataType& operator[](SizeT index) const { return array[index]; }

    using iterator = typename Vector<DataType>::iterator;
    using const_iterator = typename Vector<DataType>::const_iterator;

    iterator begin() { return array.begin(); }
    iterator end() { return array.end(); }

    const_iterator begin() const { return array.begin(); }
    const_iterator end() const { return array.end(); }

    const_iterator cbegin() const { return array.cbegin(); }
    const_iterator cend() const { return array.cend(); }

    DataType& operator[](const String& key) {
        auto it = indexMap.find(key);
        if (it != indexMap.end()) {
            return array[it->second];
        }
        else {
            throw std::out_of_range("Key" + key + "not found in VectorMap");
        }
    }

    const DataType& operator[](const String& key) const {
        auto it = indexMap.find(key);
        if (it != indexMap.end()) {
            return array[it->second];
        }
        else {
            throw std::out_of_range("Key" + key + "not found in VectorMap");
        }
    }
};

/*
template struct VectorMap<GraphicalCulture>;
//template struct VectorMap<Country>;
template struct VectorMap<Building>;
template struct VectorMap<BuildingSpawnPoint>;
template struct VectorMap<Terrain>;
template struct VectorMap<GraphicalTerrain>;
template struct VectorMap<Resource>;
template struct VectorMap<StateCategory>;
template struct VectorMap<Continent>;
*/