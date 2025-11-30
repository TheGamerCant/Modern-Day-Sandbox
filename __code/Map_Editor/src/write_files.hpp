#pragma once
#include "data_types.hpp"
#include "functions.hpp"

void WriteStateAndStrategicRegionColours(const Vector<State>& statesArray, const Vector<StrategicRegion>& strategicRegionsArray);
void WriteProvinceDefinitions(const Vector<Province>& provincesArray, const VectorMap<Terrain>& landTerrainsArray, 
	const VectorMap<Terrain>& seaTerrainsArray, const VectorMap<Terrain>& lakeTerrainsArray);
void WriteNames(const Vector<Province>& provincesArray, const Vector<State>& statesArray);