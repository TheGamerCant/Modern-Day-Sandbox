#pragma once

#include "data_types.hpp"
#include "bmp.hpp"

void UpdateProvinceColoursBasedOnStateColour(Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap, 
	BitmapImage& provincesBitmap, const Vector<State>& statesArray);