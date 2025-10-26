#pragma once
#include "functions.hpp"
#include "data_types.hpp"

void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories);
VectorMap<GraphicalCulture> LoadGraphicalCultureFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories);
VectorMap<Country> LoadCountryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, const VectorMap<GraphicalCulture>& graphicalCulturesArray);
void LoadTerrainFiles();