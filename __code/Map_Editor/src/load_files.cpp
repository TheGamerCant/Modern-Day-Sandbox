#include "load_files.hpp"
#include "data_types.hpp"
#include <fstream>
#include <iostream>
#include <thread>
#include <future>
#include <algorithm>


void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories) {
    HashMap<String, String> directoriesMap = ParseStringForPairsMapUnique(LoadFileToString("file_directories.txt"));

    //Remove any trailing quotation marks we may have
    for (auto& [key, value] : directoriesMap) {
        if (value.size() >= 2) { value = RemoveQuotes(value); }
    }

    if (directoriesMap.find("mod_directory") == directoriesMap.end()) FatalError("Mod directory is not defined in file_directories.txt");
    if (directoriesMap.find("vanilla_directory") == directoriesMap.end()) FatalError("Base game directory is not defined in file_directories.txt");


    if (std::filesystem::exists(directoriesMap.at("mod_directory")) && std::filesystem::is_directory(directoriesMap.at("mod_directory"))) modDirectory = directoriesMap.at("mod_directory");
    else FatalError(directoriesMap.at("mod_directory") + " is not a valid path");

    if (std::filesystem::exists(directoriesMap.at("vanilla_directory")) && std::filesystem::is_directory(directoriesMap.at("vanilla_directory"))) vanillaDirectory = directoriesMap.at("vanilla_directory");
    else FatalError(directoriesMap.at("vanilla_directory") + " is not a valid path");


    //Now get all replace_path entries in the .mod folder
    UnsignedInteger32 modFileCount = 0; 
    Path modFilePath;

    for (const auto& entry : std::filesystem::directory_iterator(modDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mod") {
            ++modFileCount;
            modFilePath = entry.path();
        }
    }

    //If zero we can load vanilla
    if (modFileCount == 1) {
        modReplaceDirectories.reserve(64);
        HashMap<String, Vector<String>> modFolderMap = ParseStringForPairsMapRepeat(LoadFileToString(modFilePath.string()));

        if (modFolderMap.find("replace_path") != modFolderMap.end()) {
            for (auto& pathToReplace : modFolderMap.at("replace_path")) {
                if (pathToReplace.size() >= 2) {
                    pathToReplace = RemoveQuotes(pathToReplace);
                    pathToReplace = ForwardToBackslashes(pathToReplace);

                    modReplaceDirectories.push_back(pathToReplace);
                }
            }
        }
    }

    std::sort(modReplaceDirectories.begin(), modReplaceDirectories.end());
}

void LoadGraphicalCultureFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<GraphicalCulture>& graphicalCulturesArray) {
    Vector<String> culturesArray = ParseStringAsStringArray(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\graphicalculturetype.txt").string()));
    graphicalCulturesArray.Reserve(culturesArray.size());

    for (auto& culture : culturesArray) {
        if (!graphicalCulturesArray.NameInArray(culture)) graphicalCulturesArray.EmplaceBack(culture);
    }
}

static void BadColourDefinition(const String& objectName, const String& fileIn) { FatalError("Bad colour definition for object " + objectName + " in file " + fileIn); }

void LoadCountryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Country>& countriesArray,
    const VectorMap<GraphicalCulture>& graphicalCulturesArray) {
    Vector<Path> countryTagFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\country_tags", ".txt", 4);

    for (const Path& file : countryTagFiles) {
        Vector<DoubleString> countryTagAndDefinitionFiles = ParseStringForPairsArray(LoadFileToString(file.string()), 128);
        countriesArray.Reserve(countriesArray.Capacity() + countryTagAndDefinitionFiles.size());

        for (auto& [tag, countryFile] : countryTagAndDefinitionFiles) {
            if (tag == "dynamic_tags" && countryFile == "yes") break;

            if (!TagIsValid(tag)) { 
                String outString = "Tag " + tag + " is not a valid tag in file " + file.string() + "\n";
                std::cout << outString;
                continue;
            }
        
            countryFile = RemoveQuotes(countryFile);
            countryFile = ForwardToBackslashes(countryFile);

            HashMap<String, String> countryCosmeticData = ParseStringForPairsMapUnique(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\" + countryFile).string()));
            UnsignedInteger16 cultureIndex = 0, cultureIndex2D = 0;

            //Special case for Guanxi clique vanilla file
            if (countryCosmeticData.find("graphical_culture") != countryCosmeticData.end() && countryCosmeticData.at("graphical_culture") == "asian_european_gfx") { 
                cultureIndex = graphicalCulturesArray["asian_2d"].GetId();
            }
            else if (countryCosmeticData.find("graphical_culture") == countryCosmeticData.end() || countryCosmeticData.find("graphical_culture_2d") == countryCosmeticData.end() || 
                !graphicalCulturesArray.NameInArray(countryCosmeticData.at("graphical_culture")) || !graphicalCulturesArray.NameInArray(countryCosmeticData.at("graphical_culture"))) {
                FatalError("Bad graphical cultures definition in " + countryFile);
            }
            else {
                cultureIndex = graphicalCulturesArray[countryCosmeticData.at("graphical_culture")].GetId();
                cultureIndex2D = graphicalCulturesArray[countryCosmeticData.at("graphical_culture_2d")].GetId();
            }

            UnsignedInteger8 rgbArray[3] = { 0, 0, 0 };
            Float64 hsvArray[3] = { 0.0f, 0.0f, 0.0f };

            if (countryCosmeticData.find("color") == countryCosmeticData.end()) {
                FatalError("No colour defined in " + countryFile);
            }

            String colourData = ToLower(countryCosmeticData.at("color"));
            
            if (colourData.starts_with("rgb")) {
                colourData = colourData.substr(3);
            }
            else if (colourData.starts_with("hsv")) {
                colourData = colourData.substr(3);
            }

            Vector<String> colours = ParseStringAsStringArray(colourData);
            if (colours.size() != 3) { BadColourDefinition(tag, countryFile); }
            
            //Must all be the same type (all ints or all floats)
            if (!((StringCanBecomeInteger(colours[0]) && StringCanBecomeInteger(colours[1]) && StringCanBecomeInteger(colours[2])) ||
                ((!StringCanBecomeInteger(colours[0]) && StringCanBecomeFloat(colours[0])) && (!StringCanBecomeInteger(colours[1])
                    && StringCanBecomeFloat(colours[1])) && (!StringCanBecomeInteger(colours[2]) && StringCanBecomeFloat(colours[2]))))) {
                BadColourDefinition(tag, countryFile);
            }

            Boolean HSVbool = false;
            UnsignedInteger8 i = 0;
            for (const auto& colour : colours) {
                if (StringCanBecomeInteger(colour)) {
                    SignedInteger64 colourInt = std::stoll(colour);
                    if (colourInt > 255 || colourInt < 0) { BadColourDefinition(tag, countryFile); }
                    rgbArray[i++] = static_cast<UnsignedInteger8>(colourInt);
                }
                else if (StringCanBecomeFloat(colour)) {
                    Float64 colourFloat = std::stod(colour);
                    if (colourFloat > 1.0f || colourFloat < 0.0f) { BadColourDefinition(tag, countryFile); }
                    hsvArray[i++] = colourFloat;
                    HSVbool = true;
                }
                else { BadColourDefinition(tag, countryFile); }
            }

            if (HSVbool) { HSVToRGB(rgbArray[0], rgbArray[1], rgbArray[2], hsvArray[0], hsvArray[1], hsvArray[2]); }
            ColourRGB colour(rgbArray[0], rgbArray[1], rgbArray[2]);

            countriesArray.EmplaceBack(tag, colour, cultureIndex, cultureIndex2D);
        }
    }
    countriesArray.ShrinkToFit();
}

void LoadBuildingFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Building>& provinceBuildingsArray,
    VectorMap<Building>& stateBuildingsArray, VectorMap<BuildingSpawnPoint>& buildingSpawnPointsArray, const VectorMap<Country>& countriesArray) {
    Vector<Path> buildingsFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\buildings", ".txt", 4);

    //Store exclusives here and convert the string names to IDs at the end
    HashMap<String, String> exclusives;

    for (const auto& file : buildingsFiles) {
        HashMap<String, String> buildingsFileLvl1 = ParseStringForPairsMapUnique(LoadFileToString(file.string()));

        if (buildingsFileLvl1.find("buildings") != buildingsFileLvl1.end()) {
            Vector<DoubleString> buildings = ParseStringForPairsArray(buildingsFileLvl1.at("buildings"));

            provinceBuildingsArray.Reserve(provinceBuildingsArray.Capacity() + buildings.size());
            stateBuildingsArray.Reserve(stateBuildingsArray.Capacity() + buildings.size());

            for (const auto& [buildingName, buildingDataWhole] : buildings) {
                HashMap<String, String> buildingData = ParseStringForPairsMapUnique(buildingDataWhole);

                UnsignedInteger16 value = (buildingData.find("value") != buildingData.end()) ? std::stoi(buildingData.at("value")) : 0;
                UnsignedInteger32 baseCost = (buildingData.find("base_cost") != buildingData.end()) ? std::stoi(buildingData.at("base_cost")) : 0;
                UnsignedInteger32 baseCostConversion = (buildingData.find("base_cost_conversion") != buildingData.end()) ? std::stoi(buildingData.at("base_cost_conversion")) : 0;
                UnsignedInteger32 perLevelExtraCost = (buildingData.find("per_level_extra_cost") != buildingData.end()) ? std::stoi(buildingData.at("per_level_extra_cost")) : 0;
                UnsignedInteger32 perControlledBuildingExtraCost = (buildingData.find("per_controlled_building_extra_cost") != buildingData.end()) ? std::stoi(buildingData.at("per_controlled_building_extra_cost")) : 0;
                SignedInteger16 iconFrame = (buildingData.find("icon_frame") != buildingData.end()) ? std::stoi(buildingData.at("icon_frame")) : -1;
                UnsignedInteger8 landFort = (buildingData.find("land_fort") != buildingData.end()) ? std::stoi(buildingData.at("land_fort")) : 0;
                UnsignedInteger8 navalFort = (buildingData.find("naval_fort") != buildingData.end()) ? std::stoi(buildingData.at("naval_fort")) : 0;
                UnsignedInteger8 rocketProduction = (buildingData.find("rocket_production") != buildingData.end()) ? std::stoi(buildingData.at("rocket_production")) : 0;
                UnsignedInteger8 rocketLaunchCapacity = (buildingData.find("rocket_launch_capacity") != buildingData.end()) ? std::stoi(buildingData.at("rocket_launch_capacity")) : 0;

                //Could do this with an array such as { "infrastructure", "air_base" } etc. but that doesn't really matter
                BuildingMetadata::Enum buildingMetadata = BuildingMetadata::None;

                if (buildingData.find("infrastructure") != buildingData.end() && GetBoolFromYesNo(buildingData.at("infrastructure"))) { buildingMetadata = BuildingMetadata::Infrastructure; }
                else if (buildingData.find("air_base") != buildingData.end() && GetBoolFromYesNo(buildingData.at("air_base"))) { buildingMetadata = BuildingMetadata::AirBase; }
                else if (buildingData.find("supply_node") != buildingData.end() && GetBoolFromYesNo(buildingData.at("supply_node"))) { buildingMetadata = BuildingMetadata::SupplyNode; }
                else if (buildingData.find("is_port") != buildingData.end() && GetBoolFromYesNo(buildingData.at("is_port"))) { buildingMetadata = BuildingMetadata::IsPort; }
                else if (buildingData.find("anti_air") != buildingData.end() && GetBoolFromYesNo(buildingData.at("anti_air"))) { buildingMetadata = BuildingMetadata::AntiAir; }
                else if (buildingData.find("refinery") != buildingData.end() && GetBoolFromYesNo(buildingData.at("refinery"))) { buildingMetadata = BuildingMetadata::Refinery; }
                else if (buildingData.find("fuel_silo") != buildingData.end() && GetBoolFromYesNo(buildingData.at("fuel_silo"))) { buildingMetadata = BuildingMetadata::FuelSilo; }
                else if (buildingData.find("radar") != buildingData.end() && GetBoolFromYesNo(buildingData.at("radar"))) { buildingMetadata = BuildingMetadata::Radar; }
                else if (buildingData.find("nuclear_reactor") != buildingData.end() && GetBoolFromYesNo(buildingData.at("nuclear_reactor"))) { buildingMetadata = BuildingMetadata::NuclearReactor; }
                else if (buildingData.find("gun_emplacement") != buildingData.end() && GetBoolFromYesNo(buildingData.at("gun_emplacement"))) { buildingMetadata = BuildingMetadata::GunEmplacement; }

                Boolean showModifier = (buildingData.find("show_modifier") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("show_modifier")) : false;
                Boolean alliedBuild = (buildingData.find("allied_build") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("allied_build")) : false;
                Boolean infrastructureConstructionEffect = (buildingData.find("infrastructure_construction_effect ") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("infrastructure_construction_effect ")) : false;
                Boolean onlyCoastal = (buildingData.find("only_costal") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("only_costal")) : false;      //Spelt wrong on purpose
                Boolean disabledInDmz = (buildingData.find("disabled_in_dmz") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("disabled_in_dmz")) : false;
                Boolean needSupply = (buildingData.find("need_supply") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("need_supply")) : false;
                Boolean needDetection = (buildingData.find("need_detection") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("need_detection")) : false;
                Boolean hideIfMissingTech = (buildingData.find("hide_if_missing_tech") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("hide_if_missing_tech")) : false;
                Boolean onlyDisplayIfExists = (buildingData.find("only_display_if_exists") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("only_display_if_exists")) : false;
                Boolean isBuildable = (buildingData.find("is_buildable") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("is_buildable")) : true;
                UnsignedInteger8 showOnMap = (buildingData.find("show_on_map") != buildingData.end()) ? std::stoi(buildingData.at("show_on_map")) : 0;
                UnsignedInteger8 showOnMapMeshes = (buildingData.find("show_on_map_meshes") != buildingData.end()) ? std::stoi(buildingData.at("show_on_map_meshes")) : 1;
                Boolean alwaysShown = (buildingData.find("always_shown") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("always_shown")) : false;
                Boolean hasDestroyedMesh = (buildingData.find("has_destroyed_mesh") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("has_destroyed_mesh")) : false;
                Boolean centered = (buildingData.find("centered") != buildingData.end()) ? GetBoolFromYesNo(buildingData.at("centered")) : false;
                IntelligenceType::Enum detectingIntelType = IntelligenceType::None;
                if (buildingData.find("detecting_intel_type") != buildingData.end()) {
                    if (buildingData.at("detecting_intel_type") == "civilian") detectingIntelType = IntelligenceType::Civilian;
                    else if (buildingData.at("detecting_intel_type") == "army") detectingIntelType = IntelligenceType::Army;
                    else if (buildingData.at("detecting_intel_type") == "airforce") detectingIntelType = IntelligenceType::Airforce;
                    else if (buildingData.at("detecting_intel_type") == "navy") detectingIntelType = IntelligenceType::Navy;
                    else {
                        String outString = "Bad intelligence type for building " + buildingName + " in file " + file.string() + "\n";
                        std::cout << outString;
                    }
                }

                Boolean levelCapSharesSlots = false;
                UnsignedInteger16 levelCapProvinceMax = 0, levelCapStateMax = 15;
                SignedInteger32 levelCapExclusiveWith = -1;
                String levelCapGroupBy = "";

                if (buildingData.find("level_cap") != buildingData.end()) {
                    HashMap<String, String> levelCapData = ParseStringForPairsMapUnique(buildingData.at("level_cap"));

                    levelCapSharesSlots = (levelCapData.find("shares_slots") != levelCapData.end()) ? GetBoolFromYesNo(levelCapData.at("shares_slots")) : false;
                    levelCapProvinceMax = (levelCapData.find("province_max") != levelCapData.end()) ? std::stoi(levelCapData.at("province_max")) : 0;
                    levelCapStateMax = (levelCapData.find("state_max") != levelCapData.end()) ? std::stoi(levelCapData.at("state_max")) : 15;
                    levelCapGroupBy = (levelCapData.find("group_by") != levelCapData.end()) ? levelCapData.at("group_by") : "";
                    if (levelCapData.find("exclusive_with") != levelCapData.end()) { exclusives[buildingName] = levelCapData.at("exclusive_with"); }
                    
                }

                String specialIcon = (buildingData.find("special_icon") != buildingData.end()) ? buildingData.at("special_icon") : "";
                Decimal damageFactor = (buildingData.find("damage_factor") != buildingData.end()) ? buildingData.at("damage_factor") : "1.0";
                Decimal militaryProduction = (buildingData.find("military_production") != buildingData.end()) ? buildingData.at("military_production") : "0.0";
                Decimal generalProduction = (buildingData.find("general_production") != buildingData.end()) ? buildingData.at("general_production") : "0.0";
                Decimal navalProduction = (buildingData.find("naval_production") != buildingData.end()) ? buildingData.at("naval_production") : "0.0";
                Vector<String> tags;
                Vector<String> specialization;
                Vector<String> dlcAllowed;
                Vector<String> provinceDamageModifiers, stateDamageModifier;
                HashMap<String, Decimal> countryModifiers, stateModifiers;
                Vector<UnsignedInteger16> countryModifiersCountries;
                HashMap<String, String> missingTechLoc;

                if (buildingData.find("tags") != buildingData.end()) { tags = ParseStringAsStringArray(buildingData.at("tags")); }
                if (buildingData.find("specialization") != buildingData.end()) { specialization = ParseStringAsStringArray(buildingData.at("specialization")); }
                if (buildingData.find("dlc_allowed") != buildingData.end()) { 
                    Vector<DoubleString> dlcs = ParseStringForPairsArray(buildingData.at("dlc_allowed"));
                    for (const auto& dlc_entry : dlcs) { dlcAllowed.push_back(dlc_entry.b); }
                }
                if (buildingData.find("province_damage_modifiers") != buildingData.end()) { 
                    Vector<String> modifiers = ParseStringAsStringArray(buildingData.at("province_damage_modifiers"));
                    for (const auto& modfier : modifiers) { provinceDamageModifiers.push_back(modfier); }
                }
                if (buildingData.find("state_damage_modifier") != buildingData.end()) { 
                    Vector<String> modifiers = ParseStringAsStringArray(buildingData.at("state_damage_modifier"));
                    for (const auto& modfier : modifiers) { stateDamageModifier.push_back(modfier); }
                }
                if (buildingData.find("country_modifiers") != buildingData.end()) { 
                    HashMap<String, String> countryModifiersData = ParseStringForPairsMapUnique(buildingData.at("country_modifiers"));

                    if (countryModifiersData.find("modifiers") != countryModifiersData.end()) {
                        if (countryModifiersData.find("enable_for_controllers") != countryModifiersData.end()) {
                            Vector<String> countryTags = ParseStringAsStringArray(countryModifiersData.at("enable_for_controllers"));
                            for (const auto& tag : countryTags) { 
                                if (countriesArray.NameInArray(tag)) {
                                    countryModifiersCountries.push_back(countriesArray[tag].GetId());
                                }
                                else {
                                    String outString = "Tag defined in building " + buildingName + " in file " + file.string() + " does not exist\n";
                                    std::cout << outString;
                                }
                            }
                        }
                        HashMap<String, String> countryModifiersString = ParseStringForPairsMapUnique(countryModifiersData.at("modifiers"));
                        for (const auto& modifier : countryModifiersString) countryModifiers[modifier.first] = modifier.second;
                    }
                }
                if (buildingData.find("state_modifiers") != buildingData.end()) {
                    HashMap<String, String> stateModifiersString = ParseStringForPairsMapUnique(buildingData.at("state_modifiers"));
                    for (const auto& modifier : stateModifiersString) stateModifiers[modifier.first] = modifier.second;
                }
                if (buildingData.find("missing_tech_loc") != buildingData.end()) {
                    missingTechLoc = ParseStringForPairsMapUnique(buildingData.at("missing_tech_loc"));
                }

                if (levelCapProvinceMax == 0) {
                    stateBuildingsArray.EmplaceBack(value, baseCost, baseCostConversion, perLevelExtraCost, perControlledBuildingExtraCost, iconFrame, landFort, navalFort, rocketProduction,
                        rocketLaunchCapacity, buildingMetadata, showModifier, alliedBuild, infrastructureConstructionEffect, onlyCoastal, disabledInDmz, needSupply, needDetection, hideIfMissingTech, 
                        onlyDisplayIfExists, isBuildable, showOnMap, showOnMapMeshes, alwaysShown, hasDestroyedMesh, centered, detectingIntelType, levelCapSharesSlots, levelCapProvinceMax,
                        levelCapStateMax, levelCapExclusiveWith, levelCapGroupBy, buildingName, specialIcon, damageFactor, militaryProduction, generalProduction, navalProduction, tags, specialization,
                        dlcAllowed, provinceDamageModifiers, stateDamageModifier, countryModifiers, stateModifiers, countryModifiersCountries, missingTechLoc);
                }
                else {
                    provinceBuildingsArray.EmplaceBack(value, baseCost, baseCostConversion, perLevelExtraCost, perControlledBuildingExtraCost, iconFrame, landFort, navalFort, rocketProduction,
                        rocketLaunchCapacity, buildingMetadata, showModifier, alliedBuild, infrastructureConstructionEffect, onlyCoastal, disabledInDmz, needSupply, needDetection, hideIfMissingTech,
                        onlyDisplayIfExists, isBuildable, showOnMap, showOnMapMeshes, alwaysShown, hasDestroyedMesh, centered, detectingIntelType, levelCapSharesSlots, levelCapProvinceMax, 
                        levelCapStateMax, levelCapExclusiveWith, levelCapGroupBy, buildingName, specialIcon, damageFactor, militaryProduction, generalProduction, navalProduction, tags, specialization,
                        dlcAllowed, provinceDamageModifiers, stateDamageModifier, countryModifiers, stateModifiers, countryModifiersCountries, missingTechLoc);
                }
            }
        }

        if (buildingsFileLvl1.find("spawn_points") != buildingsFileLvl1.end()) {
            Vector<DoubleString> spawnPoints = ParseStringForPairsArray(buildingsFileLvl1.at("spawn_points"));

            buildingSpawnPointsArray.Reserve(buildingSpawnPointsArray.Capacity() + spawnPoints.size());

            for (const auto& [spawnPointName, spawnPointDataWhole] : spawnPoints) {
                HashMap<String, String> spawnPointData = ParseStringForPairsMapUnique(spawnPointDataWhole);

                UnsignedInteger16 max = (spawnPointData.find("max") != spawnPointData.end()) ? std::stoi(spawnPointData.at("max")) : 1;
                Boolean typeState = false, typeProvince = false;
                Boolean onlyCoastal = (spawnPointData.find("only_costal") != spawnPointData.end()) ? GetBoolFromYesNo(spawnPointData.at("only_costal")) : false;
                Boolean disableAutoNudging = (spawnPointData.find("disable_auto_nudging") != spawnPointData.end()) ? GetBoolFromYesNo(spawnPointData.at("disable_auto_nudging")) : false;

                if (spawnPointData.find("type") == spawnPointData.end()) { FatalError("Spawn point " + spawnPointName + " must have a type defined"); }

                if (spawnPointData.at("type") == "state") { typeState = true; }
                else if (spawnPointData.at("type") == "province") { typeProvince = true; }
                else { FatalError("Spawn point " + spawnPointName + " must have type \"state\" or \"province\""); }

                buildingSpawnPointsArray.EmplaceBack(max, typeState, typeProvince, onlyCoastal, disableAutoNudging, spawnPointName);
            }
        }
    }

    for (const auto& exclusiveBuildings : exclusives) {
        if (provinceBuildingsArray.NameInArray(exclusiveBuildings.first) && provinceBuildingsArray.NameInArray(exclusiveBuildings.second)) {
            provinceBuildingsArray[exclusiveBuildings.first].setExclusive(provinceBuildingsArray[exclusiveBuildings.second].GetId());
        }
        else if (stateBuildingsArray.NameInArray(exclusiveBuildings.first) && stateBuildingsArray.NameInArray(exclusiveBuildings.second)) {
            stateBuildingsArray[exclusiveBuildings.first].setExclusive(stateBuildingsArray[exclusiveBuildings.second].GetId());
        }
    }

    provinceBuildingsArray.ShrinkToFit();
    stateBuildingsArray.ShrinkToFit();
}

void LoadTerrainFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Terrain>& landTerrainsArray,
    VectorMap<Terrain>& seaTerrainsArray, VectorMap<Terrain>& lakeTerrainsArray, VectorMap<GraphicalTerrain>& graphicalTerrainsArray, const VectorMap<Building>& provinceBuildingsArray) {
    Vector<Path> terrainFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\terrain", ".txt", 4);

    Vector<UnsignedInteger16> baseBuildingsMaxLevel(provinceBuildingsArray.Size(), 0);
    for (SizeT i = 0; i < provinceBuildingsArray.Size(); ++i) { baseBuildingsMaxLevel[i] = provinceBuildingsArray[i].GetMaxLevel(); }

    for (const auto& file : terrainFiles) {
        HashMap<String, String> terrainFileLvl1 = ParseStringForPairsMapUnique(LoadFileToString(file.string()));

        if (terrainFileLvl1.find("categories") != terrainFileLvl1.end()) {
            Vector<DoubleString> terrains = ParseStringForPairsArray(terrainFileLvl1.at("categories"));

            landTerrainsArray.Reserve(landTerrainsArray.Capacity() + 12);
            seaTerrainsArray.Reserve(seaTerrainsArray.Capacity() + 8);
            lakeTerrainsArray.Reserve(lakeTerrainsArray.Capacity() + 2);

            for (const auto& [terrainName, terrainDataWhole] : terrains) {
                HashMap<String, String> terrainData = ParseStringForPairsMapUnique(terrainDataWhole);

                if (terrainData.find("color") == terrainData.end()) FatalError("No colour defined for terrain " + terrainName);
                ColourRGB colour(terrainData.at("color")); terrainData.erase("color");
                Boolean navalTerrain = (terrainData.find("naval_terrain") != terrainData.end()) ? GetBoolFromYesNo(terrainData.at("naval_terrain")) : false; terrainData.erase("naval_terrain");
                Boolean isWater = (terrainData.find("is_water") != terrainData.end()) ? GetBoolFromYesNo(terrainData.at("is_water")) : false; terrainData.erase("is_water");
                //ProvinceType provinceType;
                //if (!navalTerrain && !isWater) { provinceType = Land; }
                //else if (navalTerrain && isWater) { provinceType = Sea; }
                //else if (!navalTerrain && isWater) { provinceType = Lake; }
                //else { FatalError("Terrain " + terrainName + " in file " + file.string() + " cannot be have naval_terrain set to yes and is_water set to false"); }
                if (navalTerrain && !isWater) { FatalError("Terrain " + terrainName + " cannot be have naval_terrain set to yes and is_water set to false"); }

                UnsignedInteger16 combatWidth = (terrainData.find("combat_width") != terrainData.end()) ? std::stoi(terrainData.at("combat_width")) : 0; terrainData.erase("combat_width");
                UnsignedInteger16 combatSupportWidth = (terrainData.find("combat_support_width") != terrainData.end()) ? std::stoi(terrainData.at("combat_support_width")) : 0; terrainData.erase("combat_support_width");
                UnsignedInteger16 matchValue = (terrainData.find("match_value") != terrainData.end()) ? std::stoi(terrainData.at("match_value")) : 0; terrainData.erase("match_value");
                Decimal aiTerrainImportanceFactor = (terrainData.find("ai_terrain_importance_factor") != terrainData.end()) ? terrainData.at("ai_terrain_importance_factor") : "1.0"; terrainData.erase("ai_terrain_importance_factor");
                Decimal supplyFlowPenaltyFactor = (terrainData.find("supply_flow_penalty_factor") != terrainData.end()) ? terrainData.at("supply_flow_penalty_factor") : "0.0"; terrainData.erase("supply_flow_penalty_factor");
                String soundType = (terrainData.find("sound_type") != terrainData.end()) ? terrainData.at("sound_type") : ""; terrainData.erase("sound_type");

                Vector<UnsignedInteger16> buildingsMaxLevel = baseBuildingsMaxLevel;
                if (terrainData.find("buildings_max_level") != terrainData.end()) {
                    HashMap<String, String> buildingsData = ParseStringForPairsMapUnique(terrainData.at("buildings_max_level"));
                    for (const auto& [building, level] : buildingsData) {
                        if (!provinceBuildingsArray.NameInArray(building)) FatalError("Building " + building + " in terrain " + terrainName + " is not a valid province building");
                        buildingsMaxLevel[provinceBuildingsArray[building].GetId()] = std::stoi(level);
                    }
                    terrainData.erase("buildings_max_level");
                }

                HashMap<String, Decimal> modifiers;
                HashMap<String, Decimal> unitModifiers;
                HashMap<String, HashMap<String, Decimal>> subUnitModifiers;

                for (const auto& [modifierName, modifierData] : terrainData) {
                    if (StringCanBecomeFloat(modifierData)) {
                        modifiers[modifierName] = Decimal(modifierData);
                    }
                    else if (modifierName == "units") {
                        Vector<DoubleString> unitsData =  ParseStringForPairsArray(modifierData, 3);
                        for (const auto& modifier : unitsData) {
                            unitModifiers[modifier.a] = Decimal(modifier.b);
                        }
                    }
                    else {
                        HashMap<String, String> subUnitModifiersData = ParseStringForPairsMapUnique(modifierData);
                        if (subUnitModifiersData.find("units") != subUnitModifiersData.end()) {
                            Vector<DoubleString> unitsData = ParseStringForPairsArray(subUnitModifiersData.at("units"), 3);
                            for (const auto& modifier : unitsData) {
                                subUnitModifiers[modifierName][modifier.a] = Decimal(modifier.b);
                            }
                            subUnitModifiersData.erase("units");
                        }

                        for (const auto& modifier : subUnitModifiersData) {
                            subUnitModifiers[modifierName][modifier.first] = Decimal(modifier.second);
                        }
                    }
                }


                if (!navalTerrain && !isWater) {
                    landTerrainsArray.EmplaceBack(colour, combatWidth, combatSupportWidth, matchValue, aiTerrainImportanceFactor,
                        supplyFlowPenaltyFactor, soundType, terrainName, buildingsMaxLevel, modifiers, unitModifiers, subUnitModifiers);
                }
                else if (navalTerrain && isWater) {
                    seaTerrainsArray.EmplaceBack(colour, combatWidth, combatSupportWidth, matchValue, aiTerrainImportanceFactor,
                        supplyFlowPenaltyFactor, soundType, terrainName, buildingsMaxLevel, modifiers, unitModifiers, subUnitModifiers);
                }
                //We've already checked for the possibility of !isWater && navalTerrain, no need to check again
                else{
                    lakeTerrainsArray.EmplaceBack(colour, combatWidth, combatSupportWidth, matchValue, aiTerrainImportanceFactor,
                        supplyFlowPenaltyFactor, soundType, terrainName, buildingsMaxLevel, modifiers, unitModifiers, subUnitModifiers);
                }
            }
        }


        if (terrainFileLvl1.find("terrain") != terrainFileLvl1.end()) {
            Vector<DoubleString> graphicalTerrains = ParseStringForPairsArray(terrainFileLvl1.at("terrain"));

            graphicalTerrainsArray.Reserve(graphicalTerrainsArray.Capacity() + graphicalTerrains.size());
            for (const auto& [graphicalTerrainName, graphicalTerrainDataWhole] : graphicalTerrains) {
                HashMap<String, String> graphicalTerrainsData = ParseStringForPairsMapUnique(graphicalTerrainDataWhole);

                UnsignedInteger16 type; ProvinceType provinceType;
                if (graphicalTerrainsData.find("type") == graphicalTerrainsData.end()) { FatalError("No type defined for graphical terrain " + graphicalTerrainName); }
                if (landTerrainsArray.NameInArray(graphicalTerrainsData.at("type"))) { type = landTerrainsArray[graphicalTerrainsData.at("type")].GetId(); provinceType = Land; }
                else if (seaTerrainsArray.NameInArray(graphicalTerrainsData.at("type"))) { type = seaTerrainsArray[graphicalTerrainsData.at("type")].GetId(); provinceType = Sea; }
                else if (lakeTerrainsArray.NameInArray(graphicalTerrainsData.at("type"))) { type = lakeTerrainsArray[graphicalTerrainsData.at("type")].GetId(); provinceType = Lake; }
                else { FatalError("Incorrect type defined for graphical terrain " + graphicalTerrainName); }

                if (graphicalTerrainsData.find("color") == graphicalTerrainsData.end()) FatalError("No colour defined for graphical terrain " + graphicalTerrainName);
                UnsignedInteger32 colour = (graphicalTerrainsData.find("color") != graphicalTerrainsData.end()) ? std::stoi(graphicalTerrainsData.at("color")) : 0;
                UnsignedInteger32 texture = (graphicalTerrainsData.find("texture") != graphicalTerrainsData.end()) ? std::stoi(graphicalTerrainsData.at("texture")) : 0;
                Boolean spawnCity = (graphicalTerrainsData.find("spawn_city") != graphicalTerrainsData.end()) ? GetBoolFromYesNo(graphicalTerrainsData.at("spawn_city")) : false;
                Boolean permSnow = (graphicalTerrainsData.find("perm_snow") != graphicalTerrainsData.end()) ? GetBoolFromYesNo(graphicalTerrainsData.at("perm_snow")) : false;

                graphicalTerrainsArray.EmplaceBack(type, colour, texture, spawnCity, permSnow, provinceType, graphicalTerrainName);
            }
        }
    }

    landTerrainsArray.ShrinkToFit();
    seaTerrainsArray.ShrinkToFit();
    lakeTerrainsArray.ShrinkToFit();
    graphicalTerrainsArray.ShrinkToFit();
}

void LoadResourceFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Resource>& resourcesArray) {
    Vector<Path> resourceFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\resources", ".txt", 4);

    for (const auto& file : resourceFiles) {
        HashMap<String, String> resourceFileLvl1 = ParseStringForPairsMapUnique(LoadFileToString(file.string()));

        if (resourceFileLvl1.find("resources") != resourceFileLvl1.end()) {
            Vector<DoubleString> resources = ParseStringForPairsArray(resourceFileLvl1.at("resources"));

            resourcesArray.Reserve(resourcesArray.Capacity() + resources.size());

            for (const auto& [resourceName, resourceDataWhole] : resources) {
                HashMap<String, String> resourceData = ParseStringForPairsMapUnique(resourceDataWhole);

                UnsignedInteger16 iconFrame;
                if (resourceData.find("icon_frame") == resourceData.end() || !StringCanBecomeInteger(resourceData.at("icon_frame"))) { FatalError("No icon frame defined for " + resourceDataWhole); }
                iconFrame = std::stoi(resourceData.at("icon_frame"));
                Decimal cic = (resourceData.find("cic") != resourceData.end()) ? resourceData.at("cic") : "0.125";
                Decimal convoys = (resourceData.find("convoys") != resourceData.end()) ? resourceData.at("convoys") : "0.1";

                resourcesArray.EmplaceBack(iconFrame, cic, convoys, resourceName);
            }
        }
    }
    resourcesArray.ShrinkToFit();
}

void LoadStateCategoryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<StateCategory>& stateCategoriesArray,
    const VectorMap<Building>& provinceBuildingsArray, const VectorMap<Building>& stateBuildingsArray) {
    Vector<Path> stateCategoryFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\state_category", ".txt", 12);
    stateCategoriesArray.Reserve(12);

    Vector<UnsignedInteger16> baseProvinceBuildingsMaxLevel(provinceBuildingsArray.Size(), 0);
    for (SizeT i = 0; i < provinceBuildingsArray.Size(); ++i) { baseProvinceBuildingsMaxLevel[i] = provinceBuildingsArray[i].GetMaxLevel(); }
    Vector<UnsignedInteger16> baseStateBuildingsMaxLevel(stateBuildingsArray.Size(), 0);
    for (SizeT i = 0; i < stateBuildingsArray.Size(); ++i) { baseStateBuildingsMaxLevel[i] = stateBuildingsArray[i].GetMaxLevel(); }

    for (const auto& file : stateCategoryFiles) {
        HashMap<String, String> stateCategoryFileLvl1 = ParseStringForPairsMapUnique(LoadFileToString(file.string()));

        if (stateCategoryFileLvl1.find("state_categories") != stateCategoryFileLvl1.end()) {
            Vector<DoubleString> stateCategories = ParseStringForPairsArray(stateCategoryFileLvl1.at("state_categories"));

            for (const auto& [stateCategoryName, stateCategoryDataWhole] : stateCategories) {
                HashMap<String, String> stateCategoryData = ParseStringForPairsMapUnique(stateCategoryDataWhole);

                if (stateCategoryData.find("local_building_slots") == stateCategoryData.end() || !StringCanBecomeInteger(stateCategoryData.at("local_building_slots"))) {
                    FatalError("Bad local building slots definition for state category " + stateCategoryName);
                }
                UnsignedInteger16 localBuildingSlots = std::stoi(stateCategoryData.at("local_building_slots")); stateCategoryData.erase("local_building_slots");

                if (stateCategoryData.find("color") == stateCategoryData.end()) FatalError("No colour defined for " + stateCategoryName);
                ColourRGB colour(stateCategoryData.at("color")); stateCategoryData.erase("color");

                Vector<UnsignedInteger16> provinceBuildingsMaxLevel = baseProvinceBuildingsMaxLevel;
                Vector<UnsignedInteger16> stateBuildingsMaxLevel = baseStateBuildingsMaxLevel;

                if (stateCategoryData.find("buildings_max_level") != stateCategoryData.end()) {
                    HashMap<String, String> buildingsData = ParseStringForPairsMapUnique(stateCategoryData.at("buildings_max_level"));
                    for (const auto& [building, level] : buildingsData) {
                        if (provinceBuildingsArray.NameInArray(building)) provinceBuildingsMaxLevel[provinceBuildingsArray[building].GetId()] = std::stoi(level);
                        else if (stateBuildingsArray.NameInArray(building)) stateBuildingsMaxLevel[stateBuildingsArray[building].GetId()] = std::stoi(level);
                        else FatalError("Building " + building + " in terrain " + stateCategoryName + " is not a valid province building");
                    }
                    stateCategoryData.erase("buildings_max_level");
                }

                HashMap<String, Decimal> modifiers;
                for (const auto& [modifierName, modifierData] : stateCategoryData) {
                    modifiers[modifierName] = modifierData;
                }
                
                stateCategoriesArray.EmplaceBack(localBuildingSlots, colour, stateCategoryName, modifiers, provinceBuildingsMaxLevel, stateBuildingsMaxLevel);
            }
        }
    }
    stateCategoriesArray.ShrinkToFit();
}

void LoadContinentFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Continent>& continentsArray) {
    HashMap<String, String> continentsFile = ParseStringForPairsMapUnique(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "map\\continent.txt").string()));
    continentsArray.EmplaceBack("ocean");

    if (continentsFile.find("continents") != continentsFile.end()) {
        Vector<String> continents = ParseStringAsStringArray(continentsFile.at("continents"));

        for (const auto& continent : continents) {
            continentsArray.EmplaceBack(continent);
        }
    }
}

Date GetDefaultDate(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories) {
    Vector<Path> bookmarkFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\bookmarks", ".txt", 2);

    for (const auto& file : bookmarkFiles) {
        HashMap<String, String> bookmarkDataLvl1 = ParseStringForPairsMapUnique(LoadFileToString(file.string()));

        if (bookmarkDataLvl1.find("bookmarks") == bookmarkDataLvl1.end()) continue;

        HashMap<String, Vector<String>> bookmarkDataLvl2 = ParseStringForPairsMapRepeat(bookmarkDataLvl1.at("bookmarks"));

        if (bookmarkDataLvl2.find("bookmark") == bookmarkDataLvl2.end()) continue;
        Vector<String> bookmarks = bookmarkDataLvl2.at("bookmark");
        for (const auto& bookmark : bookmarks) {
            HashMap<String, String> bookmarkData = ParseStringForPairsMapUnique(bookmark);

            if (bookmarkData.find("default") != bookmarkData.end() && bookmarkData.at("default") == "yes" && bookmarkData.find("date") != bookmarkData.end()) { 
                return bookmarkData.at("date");
            }
        }
    }

    return Date(0);
}

void LoadProvinceFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<Province>& provincesArray,
    const VectorMap<Terrain>& landTerrainsArray, const VectorMap<Terrain>& seaTerrainsArray, const VectorMap<Terrain>& lakeTerrainsArray, const SizeT continentsArraySize,
    const SizeT provinceBuildingsArraySize, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap) {
    //Removes all comments and unnecessary whitespace
    String provinceDefinitions = RemoveStringWhitespace(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "map\\definition.csv").string()));

    UnsignedInteger64 newLinesCount = 1;
    for (const auto& c : provinceDefinitions) if (c == ' ') { ++newLinesCount; }
    provincesArray.reserve(newLinesCount + 1);

    Char csvEntryArray[256]{};
    UnsignedInteger16 currentStringLength = 0;
    String entry;

    //UnsignedInteger16 id = 0;     //Use currentLine instead
    ColourRGB colour(0, 0, 0);
    ProvinceType type = Land;
    Boolean coastal = false;
    UnsignedInteger16 terrain = 0;
    UnsignedInteger16 continent = 0;

    //Empty vectors
    Vector<UnsignedInteger16> buildings(provinceBuildingsArraySize, 0);

    UnsignedInteger8 column = 0;
    UnsignedInteger16 currentLine = 0;

    for (const auto& c : provinceDefinitions) {


        if (c == ' ') {
            if (column == 7) {
                csvEntryArray[currentStringLength++] = 0;
                entry = String(csvEntryArray);

                if (!StringCanBecomeInteger(entry)) { FatalError("Bad continent definition in definition.csv at line " + std::to_string(currentLine)); }
                continent = std::stoi(entry);
                if (continent >= continentsArraySize) { FatalError("Bad continent definition in definition.csv at line " + std::to_string(currentLine)); }
            }
            else if (column < 7) { FatalError("Column in definition.csv at line " + std::to_string(currentLine) + " is too short"); }

            provincesArray.emplace_back(currentLine, colour, type, coastal, terrain, continent, buildings);

            currentLine++; column = 0; currentStringLength = 0;
        }
        else if (c == ';') {
            csvEntryArray[currentStringLength++] = 0;
            entry = String(csvEntryArray);

            switch (column) {
                //ID - ignore this and just use currentLine
                case 0:
                    break;

                //Red
                case 1:
                    if (StringCanBecomeInteger(entry)) { colour.r = std::stoi(entry); }
                    else { FatalError("Bad colour definition in definition.csv at line " + std::to_string(currentLine)); }
                    break;

                //Green
                case 2:
                    if (StringCanBecomeInteger(entry)) { colour.g = std::stoi(entry); }
                    else { FatalError("Bad colour definition in definition.csv at line " + std::to_string(currentLine)); }
                    break;

                //Blue
                case 3:
                    if (StringCanBecomeInteger(entry)) { colour.b = std::stoi(entry); }
                    else { FatalError("Bad colour definition in definition.csv at line " + std::to_string(currentLine)); }
                    break;

                //Land/Sea/Lake
                case 4:
                    if (entry == "land") { type = Land; }
                    else if (entry == "sea") { type = Sea; }
                    else if (entry == "lake") { type = Lake; }
                    else { FatalError("Bad province type definition in definition.csv at line " + std::to_string(currentLine)); }
                    break;

                //Coastal
                case 5:
                    if (entry == "false") { coastal = false; }
                    else if (entry == "true") { coastal = true; }
                    else { FatalError("Bad coastal in definition.csv at line " + std::to_string(currentLine)); }
                    break;

                //Terrain
                case 6:
                    switch (type) {
                        case Land:
                            if (!landTerrainsArray.NameInArray(entry)) { FatalError("Bad terrain in definition.csv at line " + std::to_string(currentLine)); }
                            terrain = landTerrainsArray[entry].GetId();
                            break;
                        case Sea:
                            if (!seaTerrainsArray.NameInArray(entry)) { FatalError("Bad terrain in definition.csv at line " + std::to_string(currentLine)); }
                            terrain = seaTerrainsArray[entry].GetId();
                            break;
                        case Lake:
                            if (!lakeTerrainsArray.NameInArray(entry)) { FatalError("Bad terrain in definition.csv at line " + std::to_string(currentLine)); }
                            terrain = lakeTerrainsArray[entry].GetId();
                            break;
                        default:
                            break;
                    }

                    break;

                //Continent
                case 7:
                    if (!StringCanBecomeInteger(entry)) { FatalError("Bad continent definition in definition.csv at line " + std::to_string(currentLine)); }
                    continent = std::stoi(entry);
                    if (continent >= continentsArraySize) { FatalError("Bad continent definition in definition.csv at line " + std::to_string(currentLine)); }
                    break;

                //Just ignore anything else
                default:
                    break;
            }

            column++;
            currentStringLength = 0;
        }
        else {
            if (currentStringLength > 254) { FatalError("Entry too long in definition.csv at line " + std::to_string(currentLine)); }
            csvEntryArray[currentStringLength++] = c;
        }
    }

    if (column != 0) {
        if (column == 7) {
            csvEntryArray[currentStringLength++] = 0;
            entry = String(csvEntryArray);

            if (!StringCanBecomeInteger(entry)) { FatalError("Bad continent definition in definition.csv at line " + std::to_string(currentLine)); }
            continent = std::stoi(entry);
            if (continent >= continentsArraySize) { FatalError("Bad continent definition in definition.csv at line " + std::to_string(currentLine)); }
        }
        else if (column < 7) { FatalError("Column in definition.csv at line " + std::to_string(currentLine) + " is too short"); }

        provincesArray.emplace_back(currentLine, colour, type, coastal, terrain, continent, buildings);
    }

    UnsignedInteger32 currentColour = 0;
    for (const auto& province : provincesArray) {
        currentColour = province.GetColour().ToInteger();
        if (provinceColoursToIdMap.find(currentColour) != provinceColoursToIdMap.end()) {
            FatalError("Provinces " + std::to_string(provinceColoursToIdMap.at(currentColour)) + " and " + std::to_string(province.GetId()) + " have the same colour (" + province.GetColour().ToHex() + ")");
        }

        provinceColoursToIdMap[currentColour] = province.GetId();
    }

	Vector<DoubleString> vpNames = ParseStringForPairsArray(LoadFileToString("names_victory_points.txt"));
    
    HashMap<UnsignedInteger16, HashMap<String, Vector<String>>> vpNameEntries;
    for (const auto& [vp, data] : vpNames) {
        if (StringCanBecomeInteger(vp)) {
            UnsignedInteger16 vpId = std::stoi(vp);
            vpNameEntries[vpId] = ParseStringForPairsMapRepeat(data);
		}
        else {
			String outString = "Bad victory point ID defined in names_victory_points.txt\n";
            std::cout << outString;
        }
    }

    for (const auto& [vp, dataHashMap] : vpNameEntries) {
        if (dataHashMap.find("default") == dataHashMap.end()) {
            String outString = "No default name defined for victory point " + std::to_string(vp) + "\n";
			std::cout << outString;
            continue;
        }

        if (dataHashMap.at("default").size() > 1) {
            String outString = "More than one default name defined for " + std::to_string(vp) + ", the last one will be used\n";
            std::cout << outString;
        }

		String defaultName = RemoveQuotes(dataHashMap.at("default")[dataHashMap.at("default").size() - 1]);
        Vector<ChangeableName> nameEntries;

        if (dataHashMap.find("entry") != dataHashMap.end() && dataHashMap.at("entry").size() > 0) {
			Vector<String> entriesDataVector = dataHashMap.at("entry");
            for (const auto& entryDataWhole : entriesDataVector) {
				HashMap<String, String> entryData = ParseStringForPairsMapUnique(entryDataWhole);

                if (entryData.find("name") == entryData.end()) {
                    String outString = "Name entry for victory point " + std::to_string(vp) + " does not have a name defined.\n";
                    std::cout << outString;
                    continue;
                }
                String changedName = RemoveQuotes(entryData.at("name"));

                if (entryData.find("requirements") == entryData.end()) {
                    String outString = "Name entry for victory point " + std::to_string(vp) + " does not have a requirements defined.\n";
                    std::cout << outString;
                    continue;
                }
                Vector<String> nameRequirements = ParseStringAsStringArray(entryData.at("requirements"));

                nameEntries.emplace_back(changedName, nameRequirements);
            }
        }

        if (provincesArray.size() >= vp) {
            provincesArray[vp].SetDefaultName(defaultName);
            provincesArray[vp].SetNameEntries(nameEntries);
        }
    }
}

struct ProvinceVictoryPoint {
    UnsignedInteger16 provinceId, victoryPointValue;
};

static void ProcessStateFilesVector(const Vector<Path>& stateFiles, Vector<State>& statesArray, Vector<ProvinceVictoryPoint>& victoryPoints, const VectorMap<Country>& countriesArray,
    const VectorMap<Building>& provinceBuildingsArray, const VectorMap<Building>& stateBuildingsArray, const VectorMap<Resource>& resourcesArray, const VectorMap<StateCategory>& stateCategoriesArray, 
    const Date defaultDate) {

    for (const auto& file : stateFiles) {
        HashMap<String, Vector<String>> states = ParseStringForPairsMapRepeat(LoadFileToString(file.string()));
        if (states.find("state") != states.end()) {
            Vector<String> statesDataWhole = states.at("state");
            for (const auto& stateDataWhole : statesDataWhole) {
                HashMap<String, Vector<String>> stateData = ParseStringForPairsMapRepeat(stateDataWhole);

                if (stateData.find("id") == stateData.end() || !StringCanBecomeInteger(stateData.at("id")[0])) { FatalError("No/invalid ID defined in " + file.string()); }
                UnsignedInteger16 id = std::stoi(stateData.at("id")[stateData.at("id").size() - 1]);
                String stringId = std::to_string(id);

                if (stateData.find("name") == stateData.end()) { FatalError("No name defined for state " + stringId); }
                String name = RemoveQuotes(stateData.at("name")[stateData.at("name").size() - 1]);

                if (stateData.find("manpower") == stateData.end() || !StringCanBecomeInteger(stateData.at("manpower")[stateData.at("manpower").size() - 1]))
                    { FatalError("Bad manpower definition for state " + stringId + " in " + file.string()); }
                UnsignedInteger32 manpower = std::stoi(stateData.at("manpower")[stateData.at("manpower").size() - 1]);

                if (stateData.find("state_category") == stateData.end() || !stateCategoriesArray.NameInArray(RemoveQuotes(stateData.at("state_category")[stateData.at("state_category").size() - 1])))
                    { FatalError("Bad state category definition for state " + stringId + " in " + file.string()); }
                UnsignedInteger16 stateCategory = stateCategoriesArray[RemoveQuotes(stateData.at("state_category")[stateData.at("state_category").size() - 1])].GetId();

                if (stateData.find("provinces") == stateData.end() || stateData.at("provinces").size() != 1) { FatalError("No provinces defined for state " + stringId); }
                Vector<UnsignedInteger16> provinces = ParseStringAsUnsignedInteger16Array(stateData.at("provinces")[0]);

                Boolean impassable = (stateData.find("impassable") != stateData.end()) ? GetBoolFromYesNo(stateData.at("impassable")[stateData.at("impassable").size() - 1]) : false;
                UnsignedInteger16 forceOwnershipLinkTo = (stateData.find("force_link_ownership_to") != stateData.end()) ? std::stoi(stateData.at("force_link_ownership_to")[stateData.at("force_link_ownership_to").size() - 1]) : 0;

                Vector<UnsignedInteger16> resources(resourcesArray.Size(), 0);
                if (stateData.find("resources") != stateData.end()) {
                    for (const auto& resourcesData : stateData.at("resources")) {
                        Vector<DoubleString> resourcesEntries = ParseStringForPairsArray(resourcesData);

                        for (const auto& [resource, resourceValue] : resourcesEntries) {
                            if (!resourcesArray.NameInArray(resource) || !StringCanBecomeFloat(resourceValue)) {
                                String outString = "Bad resource defined in file " + file.string() + "\n";
                                std::cout << outString;
                                continue;
                            }
                            Float64 resourceValueDouble = std::stod(resourceValue);
                            if (std::floor(resourceValueDouble) != resourceValueDouble || resourceValueDouble < 0.0f || resourceValueDouble > 65536.0f) {
                                String outString = "Bad resource defined in file " + file.string() + "\n";
                                std::cout << outString;
                                continue;
                            }

                            resources[resourcesArray[resource].GetId()] += UnsignedInteger16(resourceValueDouble);
                            
                        }
                    }
                }
              
                Decimal localSupplies = (stateData.find("local_supplies") != stateData.end() && stateData.at("local_supplies").size() == 1) ? stateData.at("local_supplies")[0] : "0.0";
                Decimal buildingsMaxLevelFactor = (stateData.find("buildings_max_level_factor") != stateData.end() && stateData.at("buildings_max_level_factor").size() == 1) ? stateData.at("buildings_max_level_factor")[0] : "1.0";

                Vector<StateHistory> stateHistoriesArray;
                if (stateData.find("history") != stateData.end() && stateData.at("history").size() < 2) {
                    Vector<DoubleString> stateHistoryEntries = ParseStringForPairsArray(stateData.at("history")[0]);

                    HashMap<SignedInteger64, String> historyEntriesMap;
                    Set<String> stringsToRemove = { "victory_points" };
                    for (const auto& entry : stateHistoryEntries) {
                        if (entry.a == "victory_points") {
                            //I could do a char array here as there should only be two but I'm lazy :3
                            Vector<Float64> vpData = ParseStringAsFloat64Array(entry.b);

                            if (vpData.size() == 2
                                && std::floor(vpData[0]) == vpData[0] && vpData[0] >= 0.0f || vpData[0] <= 65536.0f
                                && std::floor(vpData[1]) == vpData[1] && vpData[1] >= 0.0f || vpData[1] <= 65536.0f
                                ) {
                                victoryPoints.emplace_back(UnsignedInteger16(vpData[0]), UnsignedInteger16(vpData[1]));
                            }
                            else {
                                String outString = "Bad victory point definition in state " + stringId + "\n";
                                std::cout << outString;
                            }
                        }

                        else if (StringCanBecomeDate(entry.a)) {
                            if (std::find(stringsToRemove.begin(), stringsToRemove.end(), entry.a) != stringsToRemove.end()) {
                                String outString = "More than one entry for date " + entry.a + " found in state " + stringId + ", only the first entry will be read\n";
                                std::cout << outString;
                            }
                            else {
                                historyEntriesMap[Date(entry.a).GetHoursSinceStart()] = entry.b;
                                stringsToRemove.insert(entry.a);
                            }
                        }
                    }

                    stateHistoryEntries.erase(
                        std::remove_if(stateHistoryEntries.begin(), stateHistoryEntries.end(),
                            [&](const DoubleString& s) {
                                return stringsToRemove.count(s.a) != 0;
                            }),
                        stateHistoryEntries.end()
                    );
                    
                    String baseHistoryString = "";
                    for (const auto& historyEntry : stateHistoryEntries) {
                        baseHistoryString += historyEntry.a + "={" + historyEntry.b + "}";
                    }

                    historyEntriesMap[0] = baseHistoryString;
                    stateHistoriesArray.reserve(historyEntriesMap.size());
                     
                    for (const auto& historyEntry : historyEntriesMap) {
                        Vector<DoubleString> historyData = ParseStringForPairsArray(historyEntry.second);

                        SignedInteger32 owner = -1;
                        for (SignedInteger32 i = historyData.size() - 1; i >= 0; --i) {
                            if (historyData[i].a == "owner") {
                                owner = countriesArray[historyData[i].b].GetId();
                                break;
                            }
                        }

                        SignedInteger32 controller = -1;
                        for (SignedInteger32 i = historyData.size() - 1; i >= 0; --i) {
                            if (historyData[i].a == "controller") {
                                owner = countriesArray[historyData[i].b].GetId();
                                break;
                            }
                        }

                        Vector<UnsignedInteger16> stateBuildings(stateBuildingsArray.Size(), 0);
                        Vector<UnsignedInteger16> provinceBuildingsBase(provinceBuildingsArray.Size(), 0);
                        HashMap<UnsignedInteger16, Vector<UnsignedInteger16>> provinceBuildings;

                        HashMap<String, String> buildingsData;
                        for (SignedInteger32 i = historyData.size() - 1; i >= 0; --i) {
                            if (historyData[i].a == "buildings") {
                                HashMap<String, String> buildingsDataLocal = ParseStringForPairsMapUnique(historyData[i].b);

                                for (const auto& [key, value] : buildingsDataLocal) {
                                    if (buildingsData.find(key) != buildingsData.end()) {
                                        String outString = "Building " + key + " has already been defined for this bookmark in state " + stringId + ", overwriting.\n";
                                        std::cout << outString;
                                    }
                                    buildingsData[key] = value;
                                }
                            }
                        }

                        for (const auto& buildingEntry : buildingsData) {
                            if (StringCanBecomeInteger(buildingEntry.first)) {
                                HashMap<String, String> provinceBuildingsData = ParseStringForPairsMapUnique(buildingEntry.second);
                                Vector<UnsignedInteger16> provinceBuildingsCopy = provinceBuildingsBase;

                                for (const auto& provinceBuildingEntry : provinceBuildingsData) {
                                    if (!provinceBuildingsArray.NameInArray(provinceBuildingEntry.first) || !StringCanBecomeInteger(provinceBuildingEntry.second)) {
                                        String outString = "Bad building definition \"" + provinceBuildingEntry.first + " = " + provinceBuildingEntry.second + " in state " + stringId + "\n";
                                    }
                                    else {
                                        provinceBuildingsCopy[provinceBuildingsArray[provinceBuildingEntry.first].GetId()] = std::stoi(provinceBuildingEntry.second);
                                    }
                                }
                                provinceBuildings[std::stoi(buildingEntry.first)] = provinceBuildingsCopy;
                            }
                            else {
                                if (!stateBuildingsArray.NameInArray(buildingEntry.first) || !StringCanBecomeInteger(buildingEntry.second)) {
                                    String outString = "Bad building definition \"" + buildingEntry.first + " = " + buildingEntry.second + " in state " + stringId + "\n";
                                }
                                else {
                                    stateBuildings[stateBuildingsArray[buildingEntry.first].GetId()] = std::stoi(buildingEntry.second);
                                }
                            }
                        }
                        

                        Set<String> removeSet = { "buildings", "owner", "controller" };

                        historyData.erase(
                            std::remove_if(historyData.begin(), historyData.end(),
                                [&](const DoubleString& s) {
                                    return removeSet.count(s.a) != 0;
                                }),
                            historyData.end()
                        );

                        stateHistoriesArray.emplace_back(Date(historyEntry.first), owner, controller, stateBuildings, provinceBuildings, historyData);
                    }
                }
                else if (stateData.find("history") != stateData.end() && stateData.at("history").size() < 2) { FatalError("State " + stringId + " cannot have more than one history definitions"); }

                if (stateHistoriesArray.size() > 1) {
                    std::sort(stateHistoriesArray.begin(), stateHistoriesArray.end(), [](const StateHistory& a, const StateHistory& b) { return a.date.GetHoursSinceStart() < b.date.GetHoursSinceStart(); });
                }

                statesArray.emplace_back(id, impassable, stateCategory, forceOwnershipLinkTo, manpower, name, provinces, resources, localSupplies, buildingsMaxLevelFactor, stateHistoriesArray);
            }
        }
    }
}

void LoadStateFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<State>& statesArray,
    Vector<Province>& provincesArray, const  VectorMap<Country>& countriesArray, const VectorMap<Building>provinceBuildingsArray, const VectorMap<Building>stateBuildingsArray,
    const VectorMap<Resource>& resourcesArray, const VectorMap<StateCategory>& stateCategoriesArray, const Date defaultDate, HashMap<UnsignedInteger32, UnsignedInteger16>& stateColoursToIdMap) {

    Vector<Path> stateFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "history\\states", ".txt", 1200);
    Float32 coresCount = std::thread::hardware_concurrency();
    //SizeT vectorsToDivideInto = std::floor(std::max(coresCount * 0.5f, 3.0f));
    SizeT vectorsToDivideInto = 2;      //Dividing into many vectors and setting up threads requires overhead - 2 seems to be best 

    statesArray.reserve(stateFiles.size() + 1);
    Vector<Vector<Path>> stateFilesSubVectors = SplitVector(stateFiles, vectorsToDivideInto);
    Vector<Vector<State>> statesArray2D(vectorsToDivideInto);
    Vector<Vector<ProvinceVictoryPoint>> provinceVictoryPointsArray2D(vectorsToDivideInto);

    for (SizeT i = 0; i < stateFilesSubVectors.size(); ++i) { statesArray2D[i].reserve(stateFilesSubVectors[i].size()); }

    
    Vector<std::future<void>> futures;

    for (SizeT i = 0; i < vectorsToDivideInto; ++i) {
        futures.push_back(std::async(std::launch::async, ProcessStateFilesVector, std::cref(stateFilesSubVectors[i]), std::ref(statesArray2D[i]), std::ref(provinceVictoryPointsArray2D[i]),
            std::cref(countriesArray), std::cref(provinceBuildingsArray), std::cref(stateBuildingsArray), std::cref(resourcesArray), std::cref(stateCategoriesArray), std::cref(defaultDate)));
    }

    for (auto& f : futures) f.get();
    

    
    statesArray.emplace_back(0);
    for (const auto& a : statesArray2D) {

        auto it = a.begin();
        const SizeT arraySize = a.size();
        for (SizeT i = 0; i < arraySize; ++i) {
            statesArray.push_back(std::move(*it));
            ++it;
        }
    }

    std::sort(statesArray.begin(), statesArray.end(), [](const State& a, const State& b) { return a.GetId() < b.GetId(); });

	//Assign victory points and state IDs to provinces

    for (const auto& a : provinceVictoryPointsArray2D) {
        for (const auto& b : a) {
            provincesArray[b.provinceId].SetVictoryPoints(b.victoryPointValue);
        }
    }

    for (const auto& state : statesArray) {
        for (auto& provinceId : state.GetProvinces()) {
            if (provincesArray[provinceId].GetStateId() != 0) {
                String outString = "Province " + std::to_string(provincesArray[provinceId].GetId()) + " is in both state " + std::to_string(provincesArray[provinceId].GetStateId())
                    + " and state " + std::to_string(state.GetId()) + ", will be overwritten to be the latter\n";
                std::cout << outString;
            }
            provincesArray[provinceId].SetStateId(state.GetId());
        }
    }

    for (const auto& province : provincesArray) {
        if (province.GetId() == 0) continue;

        if (province.GetProvinceType() == ProvinceType::Land && province.GetStateId() == 0) {
            FatalError("Province " + std::to_string(province.GetId()) + " is not in any state");
        }
    }

    //Load state colours

	Boolean generateRandomColours = true;
    if (std::filesystem::exists("stateColours.raw")) {
        std::ifstream file("stateColours.raw", std::ios::binary | std::ios::ate);

        if (!file) { FatalError("Cannot open file stateColours.raw"); }

        SizeT fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        Char* data = new Char[fileSize];
        file.read(data, fileSize);
        SizeT parse = 0;

        if (fileSize == statesArray.size() * 3) {
            generateRandomColours = false;

            for (SizeT i = 0; i < statesArray.size(); ++i) {
                ColourRGB colour { UnsignedInteger8(data[parse]), UnsignedInteger8(data[parse + 1]), UnsignedInteger8(data[parse + 2]) };
				statesArray[i].SetColour(colour);
                parse += 3;
                stateColoursToIdMap[colour.ToInteger()] = i;
            }
        }

        delete[] data;
    }
    
    if (generateRandomColours){
		statesArray[0].SetColour(ColourRGB(0, 0, 0));
        stateColoursToIdMap[0] = 0;

        Vector<ColourRGB> randomColours = GenerateRandomColours(statesArray.size());
        for (SizeT i = 1; i < statesArray.size(); ++i) { 
            statesArray[i].SetColour(randomColours[i]); 
			stateColoursToIdMap[randomColours[i].ToInteger()] = i;
        }
    }
}

void LoadStrategicRegionFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<StrategicRegion>& strategicRegionsArray,
    Vector<State>& statesArray, Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& strategicRegionColoursToIdMap, const VectorMap<Terrain>& seaTerrainsArray) {
    Vector<Path> strategicRegionFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "map\\strategicregions", ".txt", 256);
    strategicRegionsArray.reserve(strategicRegionFiles.size() + 1);
    strategicRegionsArray.emplace_back(0);

    for (const auto& file : strategicRegionFiles) {
        HashMap<String, Vector<String>> strategicRegions = ParseStringForPairsMapRepeat(LoadFileToString(file.string()));
        if (strategicRegions.find("strategic_region") != strategicRegions.end()) {
            for (const auto& strategicRegionDataWhole : strategicRegions.at("strategic_region")) {
                HashMap<String, String> strategicRegionData = ParseStringForPairsMapUnique(strategicRegionDataWhole);

                if (strategicRegionData.find("id") == strategicRegionData.end() || !StringCanBecomeInteger(strategicRegionData.at("id"))) {
                    FatalError("Strategic region has no ID in file " + file.string());
                }
                UnsignedInteger16 id = std::stoi(strategicRegionData.at("id"));

                if (strategicRegionData.find("name") == strategicRegionData.end()) {
                    FatalError("Strategic region " + std::to_string(id) + " has no name in file " + file.string());
                }
                String name = RemoveQuotes(strategicRegionData.at("name"));

                UnsignedInteger16 navalTerrainIndex = 0;
                if (strategicRegionData.find("naval_terrain") != strategicRegionData.end()) {
                    String navalTerrainName = strategicRegionData.at("naval_terrain");
                    if (!seaTerrainsArray.NameInArray(navalTerrainName)) {
                        String outString = "Strategic region " + std::to_string(id) + " has an invalid naval terrain\n";
                        std::cout << outString;
                    }
                    else {
                        navalTerrainIndex = seaTerrainsArray[navalTerrainName].GetId();
                    }
				}

                if (strategicRegionData.find("provinces") == strategicRegionData.end()) {
                    FatalError("Strategic region " + std::to_string(id) + " has no provinces in file " + file.string());
                }
                Vector<UnsignedInteger16> provinces = ParseStringAsUnsignedInteger16Array(strategicRegionData.at("provinces"));

                Vector<WeatherPeriod> weather; weather.reserve(12);
                if (strategicRegionData.find("weather") != strategicRegionData.end()) {
                    HashMap<String, Vector<String>> periodsData = ParseStringForPairsMapRepeat(strategicRegionData.at("weather"));

                    if (periodsData.find("period") != periodsData.end()) {
                        for (const auto& periodDataWhole : periodsData.at("period")) {
                            HashMap<String, String> periodData = ParseStringForPairsMapUnique(periodDataWhole);
                            if (periodData.find("between") == periodData.end()) { 
                                String outString = "Weather period in strategic region " + std::to_string(id) + " does not have a 'between' argument defined\n";
                                std::cout << outString;
                                continue;
                            }

                            String betweenWhole = periodData.at("between");
                            for (auto& c : betweenWhole) if (c == '.') c = ' ';
                            Vector<UnsignedInteger16> between = ParseStringAsUnsignedInteger16Array(betweenWhole);

                            if (between.size() != 4) {
                                String outString = "Weather period in strategic region " + std::to_string(id) + " must have a 'between' argument in the format { DD.MM DD.MM }\n";
                                std::cout << outString;
                                continue;
                            }

                            if (!ValidDateMonth(between[1] + 1, between[0] + 1) || !ValidDateMonth(between[3] + 1, between[2] + 1)) {
                                String outString = "Bad date argument in weather period in strategic region " + std::to_string(id) + "\n";
                                std::cout << outString;
                                continue;
                            }

                            if (periodData.find("temperature") == periodData.end()) {
                                String outString = "Weather period in strategic region " + std::to_string(id) + " does not have a 'temperature' argument defined\n";
                                std::cout << outString;
                                continue;
                            }

                            Vector<Decimal> temperature = ParseStringAsDecimalArray(periodData.at("temperature"));

                            if (temperature.size() != 2) {
                                String outString = "Weather period in strategic region " + std::to_string(id) + " has a 'temperature' argument with " + std::to_string(temperature.size()) + "entries when only 2 are allowed\n";
                                std::cout << outString;
                                continue;
                            }

                            String requiredArguments[9] = { "no_phenomenon", "rain_light", "rain_heavy", "snow", "blizzard", "arctic_water", "mud", "sandstorm", "min_snow_level"};
                            Boolean missingArg = false;
                            for (const auto& arg : requiredArguments) {
                                if (periodData.find(arg) == periodData.end()) {
                                    String outString = "Weather period in strategic region " + std::to_string(id) + " does not have a '" + arg + "' argument defined\n";
                                    std::cout << outString;
                                    missingArg = true;
                                }
                                else if (!StringCanBecomeFloat(periodData.at(arg))) {
                                    String outString = "Weather period in strategic region " + std::to_string(id) + " has a non-decimal '" + arg + "' argument defined\n";
                                    std::cout << outString;
                                    missingArg = true;
                                }
                            }
                            
                            if (missingArg == true) continue;

                            weather.emplace_back(between[0], between[1], between[2], between[3], temperature[0], temperature[1], Decimal(periodData.at("no_phenomenon")), 
                                Decimal(periodData.at("rain_light")), Decimal(periodData.at("rain_heavy")), Decimal(periodData.at("snow")), Decimal(periodData.at("blizzard")),
                                Decimal(periodData.at("arctic_water")), Decimal(periodData.at("mud")), Decimal(periodData.at("sandstorm")), Decimal(periodData.at("min_snow_level")));
                        }
                    }
                }

                strategicRegionsArray.emplace_back(id, navalTerrainIndex, name, provinces, weather);
            }
        }
    }

    std::sort(strategicRegionsArray.begin(), strategicRegionsArray.end(), [](const StrategicRegion& a, const StrategicRegion& b) { return a.GetId() < b.GetId(); });

    for (const auto& strategicRegion : strategicRegionsArray) {
        for (auto& provinceId : strategicRegion.GetProvinces()) {
            if (provincesArray[provinceId].GetStrategicRegionId() != 0) {
                String outString = "Province " + std::to_string(provincesArray[provinceId].GetId()) + " is in both strategic region " + std::to_string(provincesArray[provinceId].GetStrategicRegionId())
                    + " and state " + std::to_string(strategicRegion.GetId()) + ", will be overwritten to be the latter\n";
                std::cout << outString;
            }
            provincesArray[provinceId].SetStrategicRegionId(strategicRegion.GetId());
        }
    }

    UnsignedInteger16 stateId = 0;
    for (const auto& province : provincesArray) {
        stateId = province.GetStateId();

        if (province.GetProvinceType() == ProvinceType::Land) {
            if (statesArray[stateId].GetStrategicRegionId() != province.GetStrategicRegionId() && statesArray[stateId].GetStrategicRegionId() != 0) {
                String outString = "State " + std::to_string(stateId) + " has provinces in two strategic regions, " + std::to_string(statesArray[stateId].GetStrategicRegionId()) + " and " +
                    std::to_string(province.GetStrategicRegionId()) + "\n";
                std::cout << outString;
                statesArray[stateId].SetMultipleStrategicRegions(true);
            }

            statesArray[stateId].SetStrategicRegionId(province.GetStrategicRegionId());
        }
    }

    for (const auto& state : statesArray) {
        if (state.GetId() == 0) continue;
        strategicRegionsArray[state.GetStrategicRegionId()].AddState(state.GetId());
    }

    Boolean generateRandomColours = true;
    if (std::filesystem::exists("strategicRegionColours.raw")) {
        std::ifstream file("strategicRegionColours.raw", std::ios::binary | std::ios::ate);

        if (!file) { FatalError("Cannot open file strategicRegionColours.raw"); }

        SizeT fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        Char* data = new Char[fileSize];
        file.read(data, fileSize);
        SizeT parse = 0;

        if (fileSize == strategicRegionsArray.size() * 3) {
            generateRandomColours = false;

            for (SizeT i = 0; i < strategicRegionsArray.size(); ++i) {
                ColourRGB colour{ UnsignedInteger8(data[parse]), UnsignedInteger8(data[parse + 1]), UnsignedInteger8(data[parse + 2]) };
                strategicRegionsArray[i].SetColour(colour);
                parse += 3;
                strategicRegionColoursToIdMap[colour.ToInteger()] = i;
            }
        }

        delete[] data;
    }

    if (generateRandomColours) {
        strategicRegionsArray[0].SetColour(ColourRGB(0, 0, 0));
        strategicRegionColoursToIdMap[0] = 0;

        Vector<ColourRGB> randomColours = GenerateRandomColours(strategicRegionsArray.size());
        for (SizeT i = 1; i < strategicRegionsArray.size(); ++i) {
            strategicRegionsArray[i].SetColour(randomColours[i]);
            strategicRegionColoursToIdMap[randomColours[i].ToInteger()] = i;
        }
    }
}

void LoadProvincePixelData(Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap, Vector<State>& statesArray,
    const Vector<StrategicRegion>& strategicRegionsArray, const BitmapImage& provincesBitmap, BitmapImage& terrainBitmap, const BitmapImage& heightmapBitmap, BitmapImage& statesBitmap,
    BitmapImage& provinceTerrainsBitmap, const VectorMap<Terrain>& landTerrainsArray, const VectorMap<Terrain>& seaTerrainsArray, const VectorMap<Terrain>& lakeTerrainsArray) {
    const UnsignedInteger64 width = provincesBitmap.GetWidth();
    const UnsignedInteger64 height = provincesBitmap.GetHeight();

    UnsignedInteger64 currentHeightTimesWidth = 0, index = 0, index4 = 0;
    ColourRGB colour(0, 0, 0);
    UnsignedInteger32 colourInteger = 0;
    UnsignedInteger16 provinceId = 0;


    for (SizeT i = 0; i < height; ++i) {
        currentHeightTimesWidth = i * width;
        for (SizeT j = 0; j < provincesBitmap.GetWidth(); ++j) {
            index = currentHeightTimesWidth + j;
            index4 = index * 4;

            colour = provincesBitmap.GetColourFromIndexPremultiplied(index4);
            colourInteger = colour.ToInteger();

            if (provinceColoursToIdMap.find(colourInteger) == provinceColoursToIdMap.end()) {
                FatalError("No province with colour " + colour.ToHex() + " exists");
            }

			provinceId = provinceColoursToIdMap.at(colourInteger);
            provincesArray[provinceId].EmplacePixel(index, j, i, heightmapBitmap.GetValueFromIndex(index), terrainBitmap.GetRawDataFromIndex(index));
        }
    }

    for (auto& prov : provincesArray) { prov.UpdateBoundingBox(); }
    for (auto& state : statesArray) { state.UpdateBoundingBox(provincesArray); }

	const SizeT dimensions = provincesBitmap.GetWidth() * provincesBitmap.GetHeight() * 4;
    ColourRGB currentProvinceColour (0, 0, 0);
    ColourRGB prevProvinceColour (0, 0, 0);
	ColourRGB currentStateColour(0, 0, 0);
    Vector<UnsignedInteger8> statesRgbData(dimensions, 0);

	ColourRGB currentTerrainColour(0, 0, 0);
    Vector<UnsignedInteger8> terrainsRgbData(dimensions, 0);

    for (SizeT i = 0; i < dimensions; i += 4) {
        currentProvinceColour = provincesBitmap.GetColourFromIndexPremultiplied(i);
        if (currentProvinceColour != prevProvinceColour) {
            prevProvinceColour = currentProvinceColour;
			provinceId = provinceColoursToIdMap.at(currentProvinceColour.ToInteger());

			currentStateColour = statesArray[provincesArray[provinceId].GetStateId()].GetColour();
            switch (provincesArray[provinceId].GetProvinceType()) {
                case ProvinceType::Lake:
                    currentTerrainColour = lakeTerrainsArray[provincesArray[provinceId].GetTerrain()].GetColour();
					break;
                case ProvinceType::Sea:
                    currentTerrainColour = seaTerrainsArray[strategicRegionsArray[provincesArray[provinceId].GetStrategicRegionId()].GetNavalTerrainIndex()].GetColour();
					break;
                default:
                    currentTerrainColour = landTerrainsArray[provincesArray[provinceId].GetTerrain()].GetColour();
                    break;
            }
        }
        statesRgbData[i] = currentStateColour.r;
        statesRgbData[i + 1] = currentStateColour.g;
        statesRgbData[i + 2] = currentStateColour.b;
        statesRgbData[i + 3] = 255;

        terrainsRgbData[i] = currentTerrainColour.r;
        terrainsRgbData[i + 1] = currentTerrainColour.g;
        terrainsRgbData[i + 2] = currentTerrainColour.b;
        terrainsRgbData[i + 3] = 255;
    }

    statesBitmap = BitmapImage(statesRgbData, provincesBitmap.GetWidth(), provincesBitmap.GetHeight(), RGBA);
    provinceTerrainsBitmap = BitmapImage(terrainsRgbData, provincesBitmap.GetWidth(), provincesBitmap.GetHeight(), RGBA);
}