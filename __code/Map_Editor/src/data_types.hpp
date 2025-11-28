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

    String ToString(SignedInteger16 precision = 3) const;
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

//Date
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
    GraphicalCulture(const UnsignedInteger16 id, const String& name) : id(id), name(name) {};

    String GetName();
    const String GetName() const;
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct Continent {
private:
    UnsignedInteger16 id;
    String name;
public:
    Continent() : id(0), name("") {};
    Continent(const String& name) : id(0), name(name) {};
    Continent(const UnsignedInteger16 id, const String& name) : id(id), name(name) {};

    String GetName();
    const String GetName() const;
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct Country {
private:
    Char tag[4];
    ColourRGB colour;
    UnsignedInteger16 id;
    UnsignedInteger16 graphicalCulture, graphicalCulture2D;

public:
    Country() : id(0), tag{ 0, 0, 0, 0 }, colour(0, 0, 0), graphicalCulture(0), graphicalCulture2D(0) {};
    //These will only get called after we call TagIsValid() on tagIn, no need to check
    Country(const String& tagIn, const ColourRGB colour, const UnsignedInteger16 graphicalCulture, const UnsignedInteger16 graphicalCulture2D) :
        id(0), colour(colour), graphicalCulture(graphicalCulture), graphicalCulture2D(graphicalCulture2D) { std::strncpy(tag, tagIn.c_str(), sizeof(tag)); };

    String GetTag();
    const String GetTag() const;
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
    void SetColour(const ColourRGB colourIn);
    ColourRGB GetColour();
    const ColourRGB GetColour() const;
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
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;

    UnsignedInteger16 GetMaxLevel();
    const UnsignedInteger16 GetMaxLevel() const;
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
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct Terrain {
private:
    UnsignedInteger16 id;
    ColourRGB colour;
    //Boolean navalTerrain, isWater;        //Store them in seperate land/sea/lake arrays so these are redundant
    //ProvinceType provinceType;
    UnsignedInteger16 combatWidth, combatSupportWidth;
    UnsignedInteger16 matchValue;
    Decimal aiTerrainImportanceFactor;
    Decimal supplyFlowPenaltyFactor;
    String soundType;
    String name;
    Vector<UnsignedInteger16> buildingsMaxLevel;        //Index = building (will always be province building), value - max level
    HashMap<String, Decimal> modifiers, unitModifiers;      //attack, movement and defence are stored as regular modifiers for unitModifiers and subUnitModifiers
    HashMap<String, HashMap<String, Decimal>> subUnitModifiers;

    //Note to self - do StringCanBecomeFloat() on terrain keys to find out if modifier or unit/subunit modifier

public:
    Terrain() : id(0), colour(0, 0, 0), combatWidth(0), combatSupportWidth(0), matchValue(0), aiTerrainImportanceFactor(1), supplyFlowPenaltyFactor(0),
        soundType(""), name(""), buildingsMaxLevel(), modifiers(), unitModifiers(), subUnitModifiers() {}
    Terrain(const ColourRGB colour, const UnsignedInteger16 combatWidth, const UnsignedInteger16 combatSupportWidth, const UnsignedInteger16 matchValue, 
        const Decimal aiTerrainImportanceFactor, const Decimal supplyFlowPenaltyFactor, const String& soundType, const String& name, const Vector<UnsignedInteger16>& buildingsMaxLevel,
        const HashMap<String, Decimal>& modifiers, const HashMap<String, Decimal>& unitModifiers, const HashMap<String, HashMap<String, Decimal>>& subUnitModifiers) :
        id(0), colour(colour), combatWidth(combatWidth), combatSupportWidth(combatSupportWidth), matchValue(matchValue), aiTerrainImportanceFactor(aiTerrainImportanceFactor), 
        supplyFlowPenaltyFactor(supplyFlowPenaltyFactor), soundType(soundType), name(name), buildingsMaxLevel(buildingsMaxLevel), modifiers(modifiers), unitModifiers(unitModifiers),
        subUnitModifiers(subUnitModifiers) {}

    String GetName();
    const String GetName() const;
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
    void SetColour(const ColourRGB colourIn);
    ColourRGB GetColour();
    const ColourRGB GetColour() const;
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
    void SetId(const UnsignedInteger16 idIn);
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
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
};

struct StateCategory {
private:
    UnsignedInteger16 id;
    UnsignedInteger16 localBuildingSlots;
    ColourRGB colour;
    String name;
    HashMap<String, Decimal> modifiers;
    Vector<UnsignedInteger16> provinceBuildingsMaxLevel;
    Vector<UnsignedInteger16> stateBuildingsMaxLevel;

public:
    StateCategory() : id(0), localBuildingSlots(0), colour(0, 0, 0), name(""), modifiers(), provinceBuildingsMaxLevel(), stateBuildingsMaxLevel() {}
    StateCategory(const UnsignedInteger16 localBuildingSlots, const ColourRGB colour, const String& name, const HashMap<String, Decimal>& modifiers,
        const Vector<UnsignedInteger16>& provinceBuildingsMaxLevel, const Vector<UnsignedInteger16>& stateBuildingsMaxLevel) :
        id(0), localBuildingSlots(localBuildingSlots), colour(colour), name(name), modifiers(modifiers), provinceBuildingsMaxLevel(provinceBuildingsMaxLevel), 
        stateBuildingsMaxLevel(stateBuildingsMaxLevel) {}

    String GetName();
    const String GetName() const;
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
    void SetColour(const ColourRGB colourIn);
    ColourRGB GetColour();
    const ColourRGB GetColour() const;
};

//The following go in Vectors instead of VectorMaps, as you index and reference them by ID
struct Province;
struct State;
struct StrategicRegion;

struct Pixel {
public:
    UnsignedInteger32 index;
    UnsignedInteger16 x, y;
    UnsignedInteger8 height;
    UnsignedInteger8 terrainIndex;

    Pixel() : index(0), x(0), y(0), height(0), terrainIndex(0) {}
    Pixel(const UnsignedInteger32 index, const UnsignedInteger16 x, const UnsignedInteger16 y, const UnsignedInteger8 height, const UnsignedInteger8 terrainIndex) : 
        index(index), x(x), y(y), height(height), terrainIndex(terrainIndex) {}
};

struct ChangeableName {
public:
    String name;
    Vector<String> nameRequirements;

    ChangeableName() : name(""), nameRequirements() {};
    ChangeableName(const String& name, const Vector<String>& nameRequirements) :
		name(name), nameRequirements(nameRequirements) {
	};
};

struct Province {
private:
    UnsignedInteger16 id;
    ColourRGB colour;
    ProvinceType type;
    Boolean coastal;
    UnsignedInteger16 terrainId, continentId, stateId, strategicRegionId;
    UnsignedInteger16 victoryPoints;
    Vector<UnsignedInteger16> buildings;

    Vector<Pixel> pixels;
    UnsignedInteger16 x0, y0, x1, y1;

    String defaultName;
    Vector<ChangeableName> nameEntries;

public:
    Province() : id(0), colour(0, 0, 0), type(Land), coastal(false), terrainId(0), continentId(0), stateId(0), strategicRegionId(0), buildings(), victoryPoints(0), 
        pixels(), x0(UINT16_MAX), y0(UINT16_MAX), x1(0), y1(0), defaultName(""), nameEntries() { pixels.reserve(64); };
    Province(const UnsignedInteger16 id, const ColourRGB colour, const ProvinceType type, const Boolean coastal, const UnsignedInteger16 terrainId, const UnsignedInteger16 continentId,
        const Vector<UnsignedInteger16>& buildings) :
        id(id), colour(colour), type(type), coastal(coastal), terrainId(terrainId), continentId(continentId), stateId(0), strategicRegionId(0), buildings(buildings), victoryPoints(0), 
        pixels(), x0(UINT16_MAX), y0(UINT16_MAX), x1(0), y1(0), defaultName(""), nameEntries() { pixels.reserve(64); };

    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
    void SetColour(const ColourRGB colourIn);
    ColourRGB GetColour();
    const ColourRGB GetColour() const;

    UnsignedInteger16 GetVictoryPoints();
    const UnsignedInteger16 GetVictoryPoints() const;
    void SetVictoryPoints(const UnsignedInteger16 vps);
    void ModifyVictoryPoints(const SignedInteger16 vps);

    UnsignedInteger16 GetStateId();
    const UnsignedInteger16 GetStateId() const;
    void SetStateId(const UnsignedInteger16 idIn);

    UnsignedInteger16 GetStrategicRegionId();
    const UnsignedInteger16 GetStrategicRegionId() const;
    void SetStrategicRegionId(const UnsignedInteger16 idIn);

    ProvinceType GetProvinceType();
    const ProvinceType GetProvinceType() const;
	UnsignedInteger16 GetTerrain();
	const UnsignedInteger16 GetTerrain() const;

    const Vector<Pixel>& GetPixels() const;
    Vector<Pixel>& GetPixels();
    void AddPixel(const Pixel& pixel);
    void EmplacePixel(const UnsignedInteger32 index, const UnsignedInteger16 x, const UnsignedInteger16 y, const UnsignedInteger8 height, const UnsignedInteger8 terrainIndex);
    void UpdateBoundingBox();
    Boolean BoundingBoxHasBeenUpdated();
    const Boolean BoundingBoxHasBeenUpdated() const;
    UnsignedInteger16 GetX0();
	const UnsignedInteger16 GetX0() const;
    UnsignedInteger16 GetX1();
	const UnsignedInteger16 GetX1() const;
    UnsignedInteger16 GetY0();
	const UnsignedInteger16 GetY0() const;
    UnsignedInteger16 GetY1();
	const UnsignedInteger16 GetY1() const;

    String GetDefaultName();
	const String GetDefaultName() const;
	void SetDefaultName(const String& name);
    Vector<ChangeableName>& GetNameEntries();
    const Vector<ChangeableName>& GetNameEntries() const;
	void SetNameEntries(const Vector<ChangeableName>& entries);
};

struct StateHistory {
public:
    Date date;
    SignedInteger32 owner, controller;
    Vector<UnsignedInteger16> stateBuildings;
    HashMap<UnsignedInteger16, Vector<UnsignedInteger16>> provinceBuildings;
    Vector<DoubleString> effects;

    StateHistory(const Date date) : date(date), owner(-1), controller(-1), stateBuildings(), provinceBuildings(), effects() {}
    StateHistory(const Date date, const SignedInteger32 owner, const SignedInteger32 controller, const Vector<UnsignedInteger16>& stateBuildings,
        const HashMap<UnsignedInteger16, Vector<UnsignedInteger16>>& provinceBuildings, const Vector<DoubleString>& effects) :
        date(date), owner(owner), controller(controller), stateBuildings(stateBuildings), provinceBuildings(provinceBuildings), effects(effects) {}
};

struct State {
private:
    UnsignedInteger16 id;
    ColourRGB colour;
    Boolean impassable;
    Boolean multipleStrategicRegions;
    UnsignedInteger16 strategicRegionId;
    UnsignedInteger16 stateCategoryId;
    UnsignedInteger16 forceOwnershipLinkTo;
    UnsignedInteger32 manpower;
    String name;
    Vector<UnsignedInteger16> provinces;
    Vector<UnsignedInteger16> resources;
    Decimal localSupplies;
    Decimal buildingsMaxLevelFactor;
    Vector<StateHistory> stateHistoriesArray;

    UnsignedInteger16 x0, y0, x1, y1;

    String defaultName;
    Vector<ChangeableName> nameEntries;

public:
    State(const UnsignedInteger16 id) :
        id(id), colour(0, 0, 0), impassable(false), multipleStrategicRegions(false), strategicRegionId(0), stateCategoryId(0), forceOwnershipLinkTo(0), manpower(0), name(""), provinces(), resources(),
        localSupplies(), buildingsMaxLevelFactor(), stateHistoriesArray(), x0(UINT16_MAX), y0(UINT16_MAX), x1(0), y1(0), defaultName(""), nameEntries() {}
    State(const UnsignedInteger16 id, const Boolean impassable, const UnsignedInteger16 stateCategoryId, const UnsignedInteger16 forceOwnershipLinkTo, const UnsignedInteger32 manpower, const String& name, 
        const Vector<UnsignedInteger16>& provinces, const Vector<UnsignedInteger16>& resources, const Decimal localSupplies, const Decimal buildingsMaxLevelFactor, const Vector<StateHistory>& stateHistoriesArray) :
        id(id), colour(0, 0, 0), impassable(impassable), multipleStrategicRegions(false), strategicRegionId(0), stateCategoryId(stateCategoryId), forceOwnershipLinkTo(forceOwnershipLinkTo), manpower(manpower), name(name), 
        provinces(provinces), resources(resources), localSupplies(localSupplies), buildingsMaxLevelFactor(buildingsMaxLevelFactor), stateHistoriesArray(stateHistoriesArray), x0(UINT16_MAX), y0(UINT16_MAX), x1(0), y1(0),
        defaultName(""), nameEntries() {}


    String GetName();
    const String GetName() const;
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
    void SetColour(const ColourRGB colourIn);
    ColourRGB GetColour();
    const ColourRGB GetColour() const;

    void SetStrategicRegionId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetStrategicRegionId();
    const UnsignedInteger16 GetStrategicRegionId() const;

    void SetMultipleStrategicRegions(const Boolean in);
    UnsignedInteger16 GetMultipleStrategicRegions();
    const UnsignedInteger16 GetMultipleStrategicRegions() const;

    void AddProvince(const UnsignedInteger16 provinceId);
    void RemoveProvince(const UnsignedInteger16 provinceId);
    const Vector<UnsignedInteger16>& GetProvinces() const;
    Vector<UnsignedInteger16>& GetProvinces();

    void UpdateBoundingBox(const Vector<Province>& provincesArray);
    Boolean BoundingBoxHasBeenUpdated();
    const Boolean BoundingBoxHasBeenUpdated() const;
    UnsignedInteger16 GetX0();
    const UnsignedInteger16 GetX0() const;
    UnsignedInteger16 GetX1();
    const UnsignedInteger16 GetX1() const;
    UnsignedInteger16 GetY0();
    const UnsignedInteger16 GetY0() const;
    UnsignedInteger16 GetY1();
    const UnsignedInteger16 GetY1() const;

    String GetDefaultName();
    const String GetDefaultName() const;
    void SetDefaultName(const String& name);
    Vector<ChangeableName>& GetNameEntries();
    const Vector<ChangeableName>& GetNameEntries() const;
    void SetNameEntries(const Vector<ChangeableName>& entries);
};

struct WeatherPeriod {
public:
    UnsignedInteger8 startDate, startMonth, endDate, endMonth;
    Decimal minTemperature, maxTemperature;
    Decimal noPhenomenon;
    Decimal rainLight, rainHeavy;
    Decimal snow, blizzard, arcticWater, mud, sandstorm;
    Decimal minSnowLevel;

    WeatherPeriod(const UnsignedInteger8 startDate, const UnsignedInteger8 startMonth, const UnsignedInteger8 endDate, const UnsignedInteger8 endMonth, const Decimal minTemperature,
        const Decimal maxTemperature, const Decimal noPhenomenon, const Decimal rainLight, const Decimal rainHeavy, const Decimal snow, const Decimal blizzard, const Decimal arcticWater,
        const Decimal mud, const Decimal sandstorm, const Decimal minSnowLevel) :
        startDate(startDate), startMonth(startMonth), endDate(endDate), endMonth(endMonth), minTemperature(minTemperature), maxTemperature(maxTemperature), noPhenomenon(noPhenomenon),
        rainLight(rainLight), rainHeavy(rainHeavy), snow(snow), blizzard(blizzard), arcticWater(arcticWater), mud(mud), sandstorm(sandstorm), minSnowLevel(minSnowLevel) {}
};

struct StrategicRegion {
private:
    UnsignedInteger16 id;
    ColourRGB colour;
    UnsignedInteger16 navalTerrainIndex;
    String name;
    Vector<UnsignedInteger16> provinces, states;
    Vector<WeatherPeriod> weather;
public:
    StrategicRegion() : id(0), colour(0, 0, 0), navalTerrainIndex(-1), name(""), provinces(), states(), weather() {}
    StrategicRegion(const UnsignedInteger16 id) : id(id), colour(0, 0, 0), navalTerrainIndex(-1), name(""), provinces(), states(), weather() {}
    StrategicRegion(const UnsignedInteger16 id, const UnsignedInteger16 navalTerrainIndex, const String& name, const Vector<UnsignedInteger16>& provinces,
        const Vector<WeatherPeriod>& weather) :
        id(id), colour(0, 0, 0), navalTerrainIndex(navalTerrainIndex), name(name), provinces(provinces), states(), weather(weather) { }

    String GetName();
    const String GetName() const;
    void SetId(const UnsignedInteger16 idIn);
    UnsignedInteger16 GetId();
    const UnsignedInteger16 GetId() const;
    void SetColour(const ColourRGB colourIn);
    ColourRGB GetColour();
    const ColourRGB GetColour() const;
    UnsignedInteger16 GetNavalTerrainIndex();
    const UnsignedInteger16 GetNavalTerrainIndex() const;

	void AddProvince(const UnsignedInteger16 provinceId);
	void RemoveProvince(const UnsignedInteger16 provinceId);
    const Vector<UnsignedInteger16>& GetProvinces() const;
    Vector<UnsignedInteger16>& GetProvinces();

	void AddState(const UnsignedInteger16 stateId);
	void RemoveState(const UnsignedInteger16 stateId);
    const Vector<UnsignedInteger16>& GetStates() const;
    Vector<UnsignedInteger16>& GetStates();
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


template struct VectorMap<GraphicalCulture>;
//template struct VectorMap<Country>;
template struct VectorMap<Building>;
template struct VectorMap<BuildingSpawnPoint>;
template struct VectorMap<Terrain>;
template struct VectorMap<GraphicalTerrain>;
template struct VectorMap<Resource>;
template struct VectorMap<StateCategory>;
template struct VectorMap<Continent>;

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
            obj.SetId(i);
            indexMap[obj.GetTag()] = i++;
        }
    }

    void PushBack(const Country& obj) {
        array.push_back(obj);
        indexMap[obj.GetTag()] = array.size() - 1;
        array.back().SetId(array.size() - 1);
    }

    void PushBack(Country&& obj) {
        array.push_back(std::move(obj));
        indexMap[array.back().GetTag()] = array.size() - 1;
        array.back().SetId(array.size() - 1);
    }

    template<typename... Args>
    void EmplaceBack(Args&&... args) {
        array.emplace_back(std::forward<Args>(args)...);
        indexMap[array.back().GetTag()] = array.size() - 1;
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

    Country& operator[](SizeT index) { return array[index]; }
    const Country& operator[](SizeT index) const { return array[index]; }

    using iterator = typename Vector<Country>::iterator;
    using const_iterator = typename Vector<Country>::const_iterator;

    iterator begin() { return array.begin(); }
    iterator end() { return array.end(); }

    const_iterator begin() const { return array.begin(); }
    const_iterator end() const { return array.end(); }

    const_iterator cbegin() const { return array.cbegin(); }
    const_iterator cend() const { return array.cend(); }

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