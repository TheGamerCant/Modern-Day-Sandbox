#pragma once
#include "functions.hpp"
#include "data_types.hpp"

void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories);
void LoadCountryFiles(Vector<Country>& countriesArray, const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<String>& errorsLog);
void LoadTerrainFiles();