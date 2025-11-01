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

//Colour Structs
struct ColourRGB {
public:
    UnsignedInteger8 r, g, b;

    ColourRGB();
    ColourRGB(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b);
    ColourRGB(const String& str);
};

// ----Map Data Structures----
enum ProvinceType : UnsignedInteger8 {
    Land,
    Sea,
    Lake
};


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

namespace BuildingMetadata {
    enum Enum : UnsignedInteger8 {
        None,
        Infrastructure,
        AirBase,
        SupplyNode,
        IsPort,
        AntiAir,
        Refinery,
        FuelSilo,
        Radar,
        NuclearReactor,
        GunEmplacement
    };
}

namespace IntelligenceType {
    enum Enum : UnsignedInteger8 {
        None,
        Civilian,
        Army,
        Airforce,
        Navy
    };
}

struct Building {
private:
    UnsignedInteger16 id;
    UnsignedInteger16 value;
    UnsignedInteger32 baseCost, baseCostConversion, perLevelExtraCost, perControlledBuildingExtraCost;
    SignedInteger16 iconFrame;
    UnsignedInteger8 landFort, navalFort;
    UnsignedInteger8 rocketProduction, rocketLaunchCapacity;
    BuildingMetadata::Enum buildingMetadata;
    Boolean showModifier;
    Boolean alliedBuild;
    Boolean infrastructureConstructionEffect;
    Boolean onlyCoastal;    //Base game has a spelling error (only_costal)
    Boolean disabledInDmz;
    Boolean needSupply;
    Boolean needDetection;
    Boolean hideIfMissingTech;
    Boolean onlyDisplayIfExists;
    Boolean isBuildable;
    UnsignedInteger8 showOnMap, showOnMapMeshes;
    Boolean alwaysShown, hasDestroyedMesh, centered;
    IntelligenceType::Enum detectingIntelType;
    Boolean levelCapSharesSlots;
    UnsignedInteger16 levelCapProvinceMax, levelCapStateMax;        //levelCapProvinceMax defines a building as provincial
    SignedInteger32 levelCapExclusiveWith;
    String levelCapGroupBy;
    String name;
    String specialIcon;
    Decimal damageFactor;
    Decimal militaryProduction, generalProduction, navalProduction;
    Vector<String> tags;
    Vector<String> specialization, dlcAllowed;  //Should usually only be one but can be more I believe
    Vector<String> provinceDamageModifiers, stateDamageModifier;
    HashMap<String, Decimal> countryModifiers, stateModifiers;
    Vector<UnsignedInteger16> countryModifiersCountries;
    HashMap<String, String> missingTechLoc;
    //Nuclear_facility also has a 'construction_speed_factor' modifier that is commented out and doesn't appear anywhere else or on the wiki


public:
    Building() : 
        
        id(0), value(0), baseCost(0), baseCostConversion(0), perLevelExtraCost(0), perControlledBuildingExtraCost(0), iconFrame(-1), landFort(0), navalFort(0), rocketProduction(0), 
        rocketLaunchCapacity(0), buildingMetadata(BuildingMetadata::None), showModifier(false), alliedBuild(false), infrastructureConstructionEffect(false), onlyCoastal(false), disabledInDmz(false),
        needSupply(false), needDetection(false), hideIfMissingTech(false), onlyDisplayIfExists(false), isBuildable(true), showOnMap(0), showOnMapMeshes(1), alwaysShown(false), 
        hasDestroyedMesh(false), centered(false), detectingIntelType(IntelligenceType::None), levelCapSharesSlots(false), levelCapProvinceMax(0), levelCapStateMax(15), levelCapExclusiveWith(-1), 
        levelCapGroupBy(""), name(""), specialIcon(""), damageFactor(1), militaryProduction(0), generalProduction(0), navalProduction(0), tags(), specialization(), dlcAllowed(),
        provinceDamageModifiers(), stateDamageModifier(), countryModifiers(), stateModifiers(), countryModifiersCountries(), missingTechLoc() {}

    Building(const UnsignedInteger16 value, const UnsignedInteger32 baseCost, const UnsignedInteger32 baseCostConversion, const UnsignedInteger32 perLevelExtraCost,
        const UnsignedInteger32 perControlledBuildingExtraCost, const SignedInteger16 iconFrame, const UnsignedInteger8 landFort, const UnsignedInteger8 navalFort,
        const UnsignedInteger8 rocketProduction, const UnsignedInteger8 rocketLaunchCapacity, const BuildingMetadata::Enum buildingMetadata, const Boolean showModifier, const Boolean alliedBuild,
        const Boolean infrastructureConstructionEffect, const Boolean onlyCoastal, const Boolean disabledInDmz, const Boolean needSupply, const Boolean needDetection,
        const Boolean hideIfMissingTech, const Boolean onlyDisplayIfExists, const Boolean isBuildable, const UnsignedInteger8 showOnMap, const UnsignedInteger8 showOnMapMeshes, 
        const Boolean alwaysShown, const Boolean hasDestroyedMesh, const Boolean centered, const IntelligenceType::Enum detectingIntelType, const Boolean levelCapSharesSlots, const UnsignedInteger16 levelCapProvinceMax,
        const UnsignedInteger16 levelCapStateMax, const SignedInteger32 levelCapExclusiveWith, const String& levelCapGroupBy, const String& name, const String& specialIcon, 
        const Decimal damageFactor, const Decimal militaryProduction, const Decimal generalProduction, const Decimal navalProduction,
        const Vector<String>& tags, const Vector<String>& specialization, const Vector<String>& dlcAllowed, const Vector<String>& provinceDamageModifiers, const Vector<String>& stateDamageModifier,
        const HashMap<String, Decimal>& countryModifiers, const HashMap<String, Decimal>& stateModifiers, const Vector<UnsignedInteger16>& countryModifiersCountries,
        const HashMap<String, String>& missingTechLoc) :

        id(0), value(value), baseCost(baseCost), baseCostConversion(baseCostConversion), perLevelExtraCost(perLevelExtraCost), perControlledBuildingExtraCost(perControlledBuildingExtraCost), 
        iconFrame(iconFrame), landFort(landFort), navalFort(navalFort), rocketProduction(rocketProduction), rocketLaunchCapacity(rocketLaunchCapacity), buildingMetadata(buildingMetadata),
        showModifier(showModifier), alliedBuild(alliedBuild), infrastructureConstructionEffect(infrastructureConstructionEffect), onlyCoastal(onlyCoastal), disabledInDmz(disabledInDmz),
        needSupply(needSupply), needDetection(needDetection), hideIfMissingTech(hideIfMissingTech), onlyDisplayIfExists(onlyDisplayIfExists), isBuildable(isBuildable), showOnMap(showOnMap),
        showOnMapMeshes(showOnMapMeshes), alwaysShown(alwaysShown), hasDestroyedMesh(hasDestroyedMesh), centered(centered), detectingIntelType(detectingIntelType), levelCapSharesSlots(levelCapSharesSlots),
        levelCapProvinceMax(levelCapProvinceMax), levelCapStateMax(levelCapStateMax), levelCapExclusiveWith(levelCapExclusiveWith), levelCapGroupBy(levelCapGroupBy), name(name), specialIcon(specialIcon), 
        damageFactor(damageFactor), militaryProduction(militaryProduction), generalProduction(generalProduction), navalProduction(navalProduction), tags(tags), specialization(specialization),
        dlcAllowed(dlcAllowed), provinceDamageModifiers(provinceDamageModifiers), stateDamageModifier(stateDamageModifier), countryModifiers(countryModifiers), stateModifiers(stateModifiers),
        countryModifiersCountries(countryModifiersCountries), missingTechLoc(missingTechLoc) {}

    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;

    void setExclusive(const SignedInteger32 exclusive);
};

struct BuildingSpawnPoint {
private:
    UnsignedInteger16 id;
    UnsignedInteger16 max;
    Boolean typeState, typeProvince;
    Boolean onlyCoastal;        //Once again, spelt "only_costal" in the base game files
    Boolean disableAutoNudging;
    String name;

public:
    BuildingSpawnPoint() : id(0), max(0), typeState(false), typeProvince(false), onlyCoastal(false), disableAutoNudging(false), name("") {};
    BuildingSpawnPoint(const UnsignedInteger16 max, const Boolean typeState, const Boolean typeProvince, const Boolean onlyCoastal, const Boolean disableAutoNudging, const String& name) :
        id(0), max(0), typeState(typeState), typeProvince(typeProvince), onlyCoastal(onlyCoastal), disableAutoNudging(disableAutoNudging), name(name) {};

    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct Terrain {
private:
    UnsignedInteger16 id;
    UnsignedInteger8 r, g, b;
    Boolean navalTerrain, isWater;
    ProvinceType provinceType;
    UnsignedInteger16 combatWidth, combatSupportWidth;
    UnsignedInteger16 matchValue;
    Decimal aiTerrainImportanceFactor;
    Decimal supplyFlowPenaltyFactor;
    String soundType;
    String name;
    HashMap<UnsignedInteger16, UnsignedInteger16> buildingsMaxLevel;        //Will always be province building
    HashMap<String, Decimal> modifiers, unitModifiers;      //attack, movement and defence are stored as regular modifiers for unitModifiers and subUnitModifiers
    HashMap<String, HashMap<String, Decimal>> subUnitModifiers;

    //Note to self - do StringCanBecomeFloat() on terrain keys to find out if modifier or unit/subunit modifier

public:
    Terrain() : id(0), r(0), g(0), b(0), navalTerrain(false), isWater(false), provinceType(Land), combatWidth(0), combatSupportWidth(0), matchValue(0), aiTerrainImportanceFactor(1), supplyFlowPenaltyFactor(0),
        soundType(""), name(""), buildingsMaxLevel(), modifiers(), unitModifiers(), subUnitModifiers() {}
    Terrain(const UnsignedInteger8 r,const UnsignedInteger8 g,const UnsignedInteger8 b, const Boolean navalTerrain, const Boolean isWater, const ProvinceType provinceType, 
        const UnsignedInteger16 combatWidth, const UnsignedInteger16 combatSupportWidth, const UnsignedInteger16 matchValue, const Decimal aiTerrainImportanceFactor,
        const Decimal supplyFlowPenaltyFactor, const String& soundType, const String& name, const HashMap<UnsignedInteger16, UnsignedInteger16>& buildingsMaxLevel,
        const HashMap<String, Decimal>& modifiers, const HashMap<String, Decimal>& unitModifiers, const HashMap<String, HashMap<String, Decimal>>& subUnitModifiers) :
        id(0), r(r), g(g), b(b), navalTerrain(navalTerrain), isWater(isWater), provinceType(provinceType), combatWidth(combatWidth), combatSupportWidth(combatSupportWidth), matchValue(matchValue),
        aiTerrainImportanceFactor(aiTerrainImportanceFactor), supplyFlowPenaltyFactor(supplyFlowPenaltyFactor), soundType(soundType), name(name), buildingsMaxLevel(buildingsMaxLevel),
        modifiers(modifiers), unitModifiers(unitModifiers), subUnitModifiers(subUnitModifiers) {}

    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct GraphicalTerrain{
private:
    UnsignedInteger16 id;
    UnsignedInteger16 type;
    UnsignedInteger32 colour, texture;
    Boolean spawnCity, permSnow;
    ProvinceType provinceType;
    String name;

public:
    GraphicalTerrain() : id(0), type(0), colour(0), texture(0), spawnCity(false), permSnow(false), provinceType(Land), name("") {}
    GraphicalTerrain(const UnsignedInteger16 type, const UnsignedInteger32 colour, const UnsignedInteger32 texture, const Boolean spawnCity,
        const Boolean permSnow, const ProvinceType provinceType, const String& name) :
        id(0), type(type), colour(colour), texture(texture), spawnCity(spawnCity), permSnow(permSnow), provinceType(provinceType), name(name) {}

    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct Resource {
private:
    UnsignedInteger16 id;
    UnsignedInteger16 iconFrame;
    Decimal cic;        //Factories per 1 resource - by default 0.125 (1 factory = 8 resources)
    Decimal convoys;    //Convoys needed to transport 1 resource - default 0.1 (1 convoy = 10 resources)
    String name;

public:
    Resource() : id(0), iconFrame(0), cic("0.125"), convoys("0.1"), name("") {}
    Resource(const UnsignedInteger16 iconFrame, const Decimal cic, const Decimal convoys, const String& name) :
        id(0), iconFrame(iconFrame) ,cic(cic), convoys(convoys), name(name) {}

    String GetName();
    const String GetName() const;
    void UpdateId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct StateCategory {
private:
    UnsignedInteger16 id;
    UnsignedInteger16 buildingSlots;
    UnsignedInteger8 r, g, b;
    String name;

public:
    StateCategory() : id(0), buildingSlots(0), r(0), g(0), b(0), name("") {}
    StateCategory(const UnsignedInteger16 buildingSlots, const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b, const String& name) :
        id(0), buildingSlots(buildingSlots), r(r), g(g), b(b), name(name) {}

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
            throw std::out_of_range("Key " + key + " not found in VectorMap");
        }
    }

    const Country& operator[](const String& key) const {
        auto it = indexMap.find(key);
        if (it != indexMap.end()) {
            return array[it->second];
        }
        else {
            throw std::out_of_range("Key " + key + " not found in VectorMap");
        }
    }
};