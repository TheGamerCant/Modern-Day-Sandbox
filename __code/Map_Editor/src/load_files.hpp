#pragma once
#include "functions.hpp"
#include "data_types.hpp"

void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories);

void LoadGraphicalCultureFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<GraphicalCulture>& graphicalCulturesArray);

void LoadCountryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Country>& countriesArray, 
	const VectorMap<GraphicalCulture>& graphicalCulturesArray);

void LoadBuildingFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Building>& provinceBuildingsArray, 
	VectorMap<Building>& stateBuildingsArray, VectorMap<BuildingSpawnPoint>& buildingSpawnPointsArray, const VectorMap<Country>& countriesArray);

void LoadTerrainFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Terrain>& landTerrainsArray, 
	VectorMap<Terrain>& seaTerrainsArray, VectorMap<Terrain>& lakeTerrainsArray, VectorMap<GraphicalTerrain>& graphicalTerrainsArray , const VectorMap<Building>& provinceBuildingsArray);

void LoadResourceFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Resource>& resourcesArray);

void LoadStateCategoryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<StateCategory>& stateCategoriesArray);

void LoadContinentFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Continent>& continentsArray);

Date GetDefaultDate(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories);

void LoadProvinceFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<Province>& provincesArray,
	const VectorMap<Terrain>& landTerrainsArray, const VectorMap<Terrain>& seaTerrainsArray, const VectorMap<Terrain>& lakeTerrainsArray, const SizeT continentsArraySize, 
	const SizeT provinceBuildingsArraySize, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap);

void LoadStateFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<State>& statesArray, 
	Vector<Province>& provincesArray, const  VectorMap<Country>& countriesArray, const VectorMap<Building>provinceBuildingsArray, const VectorMap<Building>stateBuildingsArray, 
	const VectorMap<Resource>& resourcesArray, const VectorMap<StateCategory>& stateCategoriesArray, const Date defaultDate, HashMap<UnsignedInteger32, UnsignedInteger16>& stateColoursToIdMap);

void LoadStrategicRegionFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<StrategicRegion>& strategicRegionsArray,
	Vector<State>& statesArray, Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& strategicRegionColoursToIdMap);