#include <iostream>
#include <thread>

#include "functions.hpp"
#include "data_types.hpp"
#include "load_files.hpp"

int main()
{
    Timestamp startTime = std::chrono::high_resolution_clock::now();

    Path vanillaDirectory, modDirectory; Vector<String> modReplaceDirectories;
    LoadFileDirectories(vanillaDirectory, modDirectory, modReplaceDirectories);

    VectorMap<GraphicalCulture> graphicalCulturesArray;
    VectorMap<Country> countriesArray;
    VectorMap<Building> provinceBuildingsArray, stateBuildingsArray; VectorMap<BuildingSpawnPoint> buildingSpawnPointsArray;
    VectorMap<Terrain> landTerrainsArray, seaTerrainsArray, lakeTerrainsArray; VectorMap<GraphicalTerrain> graphicalTerrainsArray;
    VectorMap<Resource> resourcesArray; 
    VectorMap<StateCategory> stateCategoriesArray;
    VectorMap<Continent> continentsArray;

    LoadGraphicalCultureFiles(vanillaDirectory, modDirectory, modReplaceDirectories, graphicalCulturesArray);
    LoadCountryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, countriesArray, graphicalCulturesArray);
    LoadBuildingFiles(vanillaDirectory, modDirectory, modReplaceDirectories, provinceBuildingsArray, stateBuildingsArray, buildingSpawnPointsArray, countriesArray);
    LoadTerrainFiles(vanillaDirectory, modDirectory, modReplaceDirectories, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray, graphicalTerrainsArray, provinceBuildingsArray);

    //You could put these into std::threads at the top and then join them after but it's so negligible there's no point
    LoadResourceFiles(vanillaDirectory, modDirectory, modReplaceDirectories, resourcesArray);
    LoadStateCategoryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, stateCategoriesArray);
    LoadContinentFiles(vanillaDirectory, modDirectory, modReplaceDirectories, continentsArray);
    Date defaultBookmarkDate = GetDefaultDate(vanillaDirectory, modDirectory, modReplaceDirectories);

    Vector<Province> provincesArray;
    LoadProvinceFiles(vanillaDirectory, modDirectory, modReplaceDirectories, provincesArray, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray, continentsArray.Size(), provinceBuildingsArray.Size());

    Vector<State> statesArary;
    LoadStateFiles(vanillaDirectory, modDirectory, modReplaceDirectories, statesArary, provincesArray, countriesArray, provinceBuildingsArray, stateBuildingsArray,
        resourcesArray, stateCategoriesArray, defaultBookmarkDate);
    
    std::cout << "Files took " << GetTimeElapsedFromStart(startTime) << " to load.\n";
}
