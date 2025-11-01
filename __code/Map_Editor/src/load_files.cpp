#include "load_files.hpp"
#include "data_types.hpp"
#include <fstream>
#include <iostream>

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

    if (modFileCount == 0) { FatalError("Could not find any .mod files in the mod directory " + modDirectory.string()); }
    else if (modFileCount == 0) { FatalError("More than one .mod files in the mod directory " + modDirectory.string()); }

    else {
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

VectorMap<GraphicalCulture> LoadGraphicalCultureFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories) {
    VectorMap<GraphicalCulture> culturesReturnArray;
    Vector<String> culturesArray = ParseStringAsArray(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\graphicalculturetype.txt").string()));
    culturesReturnArray.Reserve(culturesArray.size() / 2);

    for (auto& culture : culturesArray) {
        if (!culturesReturnArray.NameInArray(culture)) culturesReturnArray.EmplaceBack(culture);
    }

    return culturesReturnArray;
}

static void BadColourDefinition(const String& objectName, const String& fileIn) { FatalError("Bad colour definition for object " + objectName + " in file " + fileIn); }

VectorMap<Country> LoadCountryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, const VectorMap<GraphicalCulture>& graphicalCulturesArray) {
    Vector<Path> countryTagFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\country_tags", ".txt", 4);
    VectorMap<Country> countriesReturnArray;

    for (const Path& file : countryTagFiles) {
        Vector<DoubleString> countryTagAndDefinitionFiles = ParseStringForPairsArray(LoadFileToString(file.string()), 128);
        countriesReturnArray.Reserve(countriesReturnArray.Capacity() + countryTagAndDefinitionFiles.size());

        for (auto& [tag, countryFile] : countryTagAndDefinitionFiles) {
            if (tag == "dynamic_tags" && countryFile == "yes") break;

            if (!TagIsValid(tag)) { 
                FatalError("Error in " + file.string() + ", tag " + tag + " is not a valid tag"); 
            }
        
            countryFile = RemoveQuotes(countryFile);
            countryFile = ForwardToBackslashes(countryFile);

            HashMap<String, String> countryCosmeticData = ParseStringForPairsMapUnique(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\" + countryFile).string()));
            UnsignedInteger16 cultureIndex = 0, cultureIndex2D = 0;

            if (countryCosmeticData.find("graphical_culture") == countryCosmeticData.end() || countryCosmeticData.find("graphical_culture_2d") == countryCosmeticData.end() || 
                !graphicalCulturesArray.NameInArray(countryCosmeticData.at("graphical_culture")) || !graphicalCulturesArray.NameInArray(countryCosmeticData.at("graphical_culture"))) {
                
                FatalError("Incorrect graphical cultures definition at " + countryFile);
            }
            cultureIndex = graphicalCulturesArray[countryCosmeticData.at("graphical_culture")].GetId();
            cultureIndex2D = graphicalCulturesArray[countryCosmeticData.at("graphical_culture_2d")].GetId();

            UnsignedInteger8 rgbArray[3] = { 0, 0, 0 };
            Float64 hsvArray[3] = { 0.0f, 0.0f, 0.0f };

            if (countryCosmeticData.find("color") == countryCosmeticData.end()) {
                FatalError("Color not defined at " + countryFile);
            }

            String colourData = ToLower(countryCosmeticData.at("color"));
            
            if (colourData.starts_with("rgb")) {
                colourData = colourData.substr(3);
            }
            else if (colourData.starts_with("hsv")) {
                colourData = colourData.substr(3);
            }

            Vector<String> colours = ParseStringAsArray(colourData);
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
                    SignedInteger64 colourInt = std::stoi(colour);
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

            countriesReturnArray.EmplaceBack(tag, rgbArray[0], rgbArray[1], rgbArray[2], cultureIndex, cultureIndex2D);
        }
    }
    countriesReturnArray.ShrinkToFit();
    return countriesReturnArray;
}

void LoadBuildingFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Building>& provinceBuildingsArray,
    VectorMap<Building>& stateBuildingsArray, VectorMap<BuildingSpawnPoint>& buildingSpawnPointsArray, VectorMap<Country>& countriesArray) {
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
                IntelligenceType::Enum detectingIntelType;
                if (buildingData.find("detecting_intel_type") != buildingData.end()) {
                    if (buildingData.at("detecting_intel_type") == "civilian") detectingIntelType = IntelligenceType::Civilian;
                    else if (buildingData.at("detecting_intel_type") == "army") detectingIntelType = IntelligenceType::Army;
                    else if (buildingData.at("detecting_intel_type") == "airforce") detectingIntelType = IntelligenceType::Airforce;
                    else if (buildingData.at("detecting_intel_type") == "navy") detectingIntelType = IntelligenceType::Navy;
                    else FatalError("Bad intelligence type for building " + buildingName + " in file " + file.string());
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

                if (buildingData.find("tags") != buildingData.end()) { tags = ParseStringAsArray(buildingData.at("tags")); }
                if (buildingData.find("specialization") != buildingData.end()) { specialization = ParseStringAsArray(buildingData.at("specialization")); }
                if (buildingData.find("dlc_allowed") != buildingData.end()) { 
                    Vector<DoubleString> dlcs = ParseStringForPairsArray(buildingData.at("dlc_allowed"));
                    for (const auto& dlc_entry : dlcs) { dlcAllowed.push_back(dlc_entry.b); }
                }
                if (buildingData.find("province_damage_modifiers") != buildingData.end()) { 
                    Vector<String> modifiers = ParseStringAsArray(buildingData.at("province_damage_modifiers"));
                    for (const auto& modfier : modifiers) { provinceDamageModifiers.push_back(modfier); }
                }
                if (buildingData.find("state_damage_modifier") != buildingData.end()) { 
                    Vector<String> modifiers = ParseStringAsArray(buildingData.at("state_damage_modifier"));
                    for (const auto& modfier : modifiers) { stateDamageModifier.push_back(modfier); }
                }
                if (buildingData.find("country_modifiers") != buildingData.end()) { 
                    HashMap<String, String> countryModifiersData = ParseStringForPairsMapUnique(buildingData.at("country_modifiers"));

                    if (countryModifiersData.find("modifiers") != countryModifiersData.end()) {
                        if (countryModifiersData.find("enable_for_controllers") != countryModifiersData.end()) {
                            Vector<String> countryTags = ParseStringAsArray(countryModifiersData.at("enable_for_controllers"));
                            for (const auto& tag : countryTags) { 
                                if (!countriesArray.NameInArray(tag)) FatalError("Tag defined in building " + buildingName + " does not exist");
                                countryModifiersCountries.push_back(countriesArray[tag].GetId());
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
                Vector<String> colourArray = ParseStringAsArray(terrainData.at("color"));

                if (colourArray.size() != 3) BadColourDefinition(terrainName, file.string());
                else if (!StringCanBecomeInteger(colourArray[0]) || !StringCanBecomeInteger(colourArray[1]) || !StringCanBecomeInteger(colourArray[2])) BadColourDefinition(terrainName, file.string());

                //Have to erase afterwards so we don't store them in our modifiers vectors
                UnsignedInteger8 rgbArray[3] = { std::stoi(colourArray[0]), std::stoi(colourArray[1]), std::stoi(colourArray[2]) }; terrainData.erase("color");
                Boolean navalTerrain = (terrainData.find("naval_terrain") != terrainData.end()) ? GetBoolFromYesNo(terrainData.at("naval_terrain")) : false; terrainData.erase("naval_terrain");
                Boolean isWater = (terrainData.find("is_water") != terrainData.end()) ? GetBoolFromYesNo(terrainData.at("is_water")) : false; terrainData.erase("is_water");
                ProvinceType provinceType;
                if (!navalTerrain && !isWater) { provinceType = Land; }
                else if (navalTerrain && isWater) { provinceType = Sea; }
                else if (!navalTerrain && isWater) { provinceType = Lake; }
                else { FatalError("Terrain " + terrainName + " in file " + file.string() + " cannot be have naval_terrain set to yes and is_water set to false"); }
                UnsignedInteger16 combatWidth = (terrainData.find("combat_width") != terrainData.end()) ? std::stoi(terrainData.at("combat_width")) : 0; terrainData.erase("combat_width");
                UnsignedInteger16 combatSupportWidth = (terrainData.find("combat_support_width") != terrainData.end()) ? std::stoi(terrainData.at("combat_support_width")) : 0; terrainData.erase("combat_support_width");
                UnsignedInteger16 matchValue = (terrainData.find("match_value") != terrainData.end()) ? std::stoi(terrainData.at("match_value")) : 0; terrainData.erase("match_value");
                Decimal aiTerrainImportanceFactor = (terrainData.find("ai_terrain_importance_factor") != terrainData.end()) ? terrainData.at("ai_terrain_importance_factor") : "1.0"; terrainData.erase("ai_terrain_importance_factor");
                Decimal supplyFlowPenaltyFactor = (terrainData.find("supply_flow_penalty_factor") != terrainData.end()) ? terrainData.at("supply_flow_penalty_factor") : "0.0"; terrainData.erase("supply_flow_penalty_factor");
                String soundType = (terrainData.find("sound_type") != terrainData.end()) ? terrainData.at("sound_type") : ""; terrainData.erase("sound_type");

                HashMap<UnsignedInteger16, UnsignedInteger16> buildingsMaxLevel;
                if (terrainData.find("buildings_max_level") != terrainData.end()) {
                    HashMap<String, String> buildingsData = ParseStringForPairsMapUnique(terrainData.at("buildings_max_level"));
                    for (const auto& [building, level] : buildingsData) {
                        if (!provinceBuildingsArray.NameInArray(building)) FatalError("Building \"" + building + "\" in file " + file.string() + " is not a valid province building");
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
                    landTerrainsArray.EmplaceBack(rgbArray[0], rgbArray[1], rgbArray[2], navalTerrain, isWater, provinceType, combatWidth, combatSupportWidth, matchValue, aiTerrainImportanceFactor,
                        supplyFlowPenaltyFactor, soundType, terrainName, buildingsMaxLevel, modifiers, unitModifiers, subUnitModifiers);
                }
                else if (navalTerrain && isWater) {
                    seaTerrainsArray.EmplaceBack(rgbArray[0], rgbArray[1], rgbArray[2], navalTerrain, isWater, provinceType, combatWidth, combatSupportWidth, matchValue, aiTerrainImportanceFactor,
                        supplyFlowPenaltyFactor, soundType, terrainName, buildingsMaxLevel, modifiers, unitModifiers, subUnitModifiers);
                }
                //We've already checked for the possibility of !isWater && navalTerrain, no need to check again
                else{
                    lakeTerrainsArray.EmplaceBack(rgbArray[0], rgbArray[1], rgbArray[2], navalTerrain, isWater, provinceType, combatWidth, combatSupportWidth, matchValue, aiTerrainImportanceFactor,
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
                if (graphicalTerrainsData.find("type") == graphicalTerrainsData.end()) { FatalError("No type defined for graphical terrain " + graphicalTerrainName + " in file" + file.string()); }
                if (landTerrainsArray.NameInArray(graphicalTerrainsData.at("type"))) { type = landTerrainsArray[graphicalTerrainsData.at("type")].GetId(); provinceType = Land; }
                else if (seaTerrainsArray.NameInArray(graphicalTerrainsData.at("type"))) { type = seaTerrainsArray[graphicalTerrainsData.at("type")].GetId(); provinceType = Sea; }
                else if (lakeTerrainsArray.NameInArray(graphicalTerrainsData.at("type"))) { type = lakeTerrainsArray[graphicalTerrainsData.at("type")].GetId(); provinceType = Lake; }
                else { FatalError("Incorrect type defined for graphical terrain " + graphicalTerrainName + " in file" + file.string()); }

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
                if (resourceData.find("icon_frame") == resourceData.end() || !StringCanBecomeInteger(resourceData.at("icon_frame"))) { FatalError("No icon frame defined for " + resourceDataWhole + " in file" + file.string()); }
                iconFrame = std::stoi(resourceData.at("icon_frame"));
                Decimal cic = (resourceData.find("cic") != resourceData.end()) ? resourceData.at("cic") : "0.125";
                Decimal convoys = (resourceData.find("convoys") != resourceData.end()) ? resourceData.at("convoys") : "0.1";

                resourcesArray.EmplaceBack(iconFrame, cic, convoys, resourceName);
            }
        }
    }
    resourcesArray.ShrinkToFit();
}

void LoadStateCategoryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<StateCategory>& stateCategoriesArray) {

}