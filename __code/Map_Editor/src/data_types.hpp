#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <chrono>

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

template<typename VectorType>
using Vector = std::vector<VectorType>;

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
    Decimal(int i);
    Decimal(Float32 f);
    Decimal(Float64 d);
    explicit Decimal(SignedInteger64 raw, bool);

    operator double() const;
    operator float() const;
    operator int() const;

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

    String ToString(SignedInteger16 precision = 3) const;
    friend std::ostream& operator<<(std::ostream& os, const Decimal& d);
};

// ----Map Data Structures----
struct GraphicalCulture {
private:
    UnsignedInteger16 id;
    String name;
public:
    GraphicalCulture() : id(0), name("") {};
    GraphicalCulture(UnsignedInteger16 id, const String& name) : id(id), name(name) {};
};

struct Country {
private:
    UnsignedInteger16 id;
    Char tag[4];
    UnsignedInteger8 r, g, b;

public:
    Country() : id(0), tag{ 0, 0, 0, 0 }, r(0), g(0), b(0) {};
    //This will only get called after we call TagIsValid() on tagIn, no need to check
    Country(UnsignedInteger16 id, const String& tagIn) : id(id), r(0), g(0), b(0) { std::strncpy(tag, tagIn.c_str(), sizeof(tag)); };
};

struct Terrain {
private:
    UnsignedInteger16 id;
    UnsignedInteger8 r, g, b;
    Boolean navalTerrain, isWater;
    UnsignedInteger16 combatWidth, combatSupportWidth;
    UnsignedInteger16 matchValue;
    Decimal aiTerrainImportanceFactor;
    Decimal movementCost, navalMineHitChance;
    Decimal enemyArmyBonusAirSuperiorityFactor;
    Decimal sicknessChance;
    Decimal supplyFlowPenaltyFactor;
    Decimal truckAttritionFactor;
    String soundType;
    String name;
    HashMap<UnsignedInteger16, UnsignedInteger16> buildingsMaxLevel;
    //Ignoring units for now

public:

};

struct Building {
public:
    UnsignedInteger16 id;
    UnsignedInteger16 value;
    UnsignedInteger32 baseCost;
    Boolean infrastructure, airBase, supplyNode, isPort, antiAir, refinery, fuelSilo, radar, nuclearReactor;
    Boolean showModifier;
    Boolean alliedBuild;
    Boolean infrastructureConstructionEffect;
    //Level cap
    UnsignedInteger16 provinceMax, stateMax;
    String groupBy;
    //

private:

};


template<typename DataType>
struct VectorMap {
private:
    Vector<DataType> array;
    HashMap<String, UnsignedInteger32> indexMap;

public:

    void rebuildMap() {
        lookup_map.clear();
        for (auto& obj : objects) {
            lookup_map[obj.name] = std::make_shared<T>(obj);
        }
    }

    void push_back(const T& obj) { array.push_back(obj); }
    void emplace_back(const T& obj) { array.emplace_back(obj); }
    void reserve(const SiztT reserve) { array.reserve(reserve); }
    void shrink_to_fit() { array.shrink_to_fit(); }
};