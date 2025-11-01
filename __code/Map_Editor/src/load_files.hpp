#pragma once
#include "functions.hpp"
#include "data_types.hpp"

void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories);

VectorMap<GraphicalCulture> LoadGraphicalCultureFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories);

VectorMap<Country> LoadCountryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, const VectorMap<GraphicalCulture>& graphicalCulturesArray);

void LoadBuildingFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Building>& provinceBuildingsArray, 
	VectorMap<Building>& stateBuildingsArray, VectorMap<BuildingSpawnPoint>& buildingSpawnPointsArray, VectorMap<Country>& countriesArray);

void LoadTerrainFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Terrain>& landTerrainsArray, 
	VectorMap<Terrain>& seaTerrainsArray, VectorMap<Terrain>& lakeTerrainsArray, VectorMap<GraphicalTerrain>& graphicalTerrainsArray , const VectorMap<Building>& provinceBuildingsArray);

void LoadResourceFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<Resource>& resourcesArray);

void LoadStateCategoryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, VectorMap<StateCategory>& stateCategoriesArray);