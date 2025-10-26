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
    GraphicalCulture(const String& name) : id(0), name(name) {};
    GraphicalCulture(UnsignedInteger16 id, const String& name) : id(id), name(name) {};

    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct Country {
private:
    Char tag[4];
    UnsignedInteger8 r, g, b;
    UnsignedInteger16 id;
    UnsignedInteger16 graphicalCulture, graphicalCulture2D;

public:
    Country() : id(0), tag{ 0, 0, 0, 0 }, r(0), g(0), b(0), graphicalCulture(0), graphicalCulture2D(0) {};
    //These will only get called after we call TagIsValid() on tagIn, no need to check
    Country(const String& tagIn, const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b, const UnsignedInteger16 graphicalCulture, const UnsignedInteger16 graphicalCulture2D) :
        id(0), r(r), g(g), b(b), graphicalCulture(graphicalCulture), graphicalCulture2D(graphicalCulture2D) { std::strncpy(tag, tagIn.c_str(), sizeof(tag)); };

    String GetTag();
    const String GetTag() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
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
    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct Building {
public:
    UnsignedInteger16 id;
    UnsignedInteger16 value;
    UnsignedInteger32 baseCost, per_level_extra_cost;
    UnsignedInteger16 iconFrame;
    Boolean infrastructure, airBase, supplyNode, isPort, antiAir, refinery, fuelSilo, radar, nuclearReactor;
    Boolean showModifier;
    Boolean alliedBuild;
    Boolean infrastructureConstructionEffect;
    Boolean onlyCoastal;    //Base game has a spelling error (only_costal)
    Boolean disabledInDmz;
    Boolean levelCapSharesSlots;
    UnsignedInteger16 levelCapProvinceMax, levelCapStateMax;
    String levelCapGroupBy;

    String name;
    Decimal damageFactor;

private:
    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

//Custom data type that allows indexing by index or name/tag
//e.g
//std::cout << countriesArray[8].GetTag() << ", " << countriesArray["SCT"].GetTag();

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
            obj.updateId(i);
            indexMap[obj.name] = i++;
        }
    }

    void PushBack(const DataType& obj) {
        array.push_back(obj);
        indexMap[obj.name] = array.size() - 1;
        array.back().UpdateId(array.size() - 1);
    }
    void PushBack(DataType&& obj) {
        array.push_back(std::move(obj));
        indexMap[array.back().GetName()] = array.size() - 1;
        array.back().UpdateId(array.size() - 1);
    }
    
    template<typename... Args>
    void EmplaceBack(Args&&... args) {
        array.emplace_back(std::forward<Args>(args)...);
        indexMap[array.back().GetName()] = array.size() - 1;
        array.back().UpdateId(array.size() - 1);
    }

    void Reserve(const SizeT reserve) { array.reserve(reserve); }
    SizeT Capacity() { return array.capacity(); }
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

template<>
struct VectorMap<Country> {
private:
    Vector<Country> array;
    HashMap<String, UnsignedInteger64> indexMap;

public:
    void RebuildMap() {
        indexMap.clear();
        UnsignedInteger64 i = 0;
        for (auto& obj : array) {
            obj.UpdateId(i);
            indexMap[obj.GetTag()] = i++;
        }
    }

    void PushBack(const Country& obj) {
        array.push_back(obj);
        indexMap[obj.GetTag()] = array.size() - 1;
        array.back().UpdateId(array.size() - 1);
    }

    void PushBack(Country&& obj) {
        array.push_back(std::move(obj));
        indexMap[array.back().GetTag()] = array.size() - 1;
        array.back().UpdateId(array.size() - 1);
    }

    template<typename... Args>
    void EmplaceBack(Args&&... args) {
        array.emplace_back(std::forward<Args>(args)...);
        indexMap[array.back().GetTag()] = array.size() - 1;
        array.back().UpdateId(array.size() - 1);
    }

    void Reserve(const SizeT reserve) { array.reserve(reserve); }
    SizeT Capacity() const { return array.capacity(); }
    void ShrinkToFit() { array.shrink_to_fit(); }

    Boolean NameInArray(const String& findString) {
        if (indexMap.find(findString) == indexMap.end()) return false;
        return true;
    }

    const Boolean NameInArray(const String& findString) const {
        if (indexMap.find(findString) == indexMap.end()) return false;
        return true;
    }

    Country& operator[](SizeT index) { return array[index]; }
    const Country& operator[](SizeT index) const { return array[index]; }

    Country& operator[](const String& key) {
        auto it = indexMap.find(key);
        if (it != indexMap.end()) {
            return array[it->second];
        }
        else {
            throw std::out_of_range("Key" + key + "not found in VectorMap");
        }
    }

    const Country& operator[](const String& key) const {
        auto it = indexMap.find(key);
        if (it != indexMap.end()) {
            return array[it->second];
        }
        else {
            throw std::out_of_range("Key" + key + "not found in VectorMap");
        }
    }
};