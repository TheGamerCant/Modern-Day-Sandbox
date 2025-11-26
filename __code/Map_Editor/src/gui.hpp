#pragma once

#include "data_types.hpp"
#include "bmp.hpp"

#include "raylib.h"

void SetProvinceColoursBasedOnStateColour(Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap, 
	BitmapImage& provincesBitmap, const Vector<State>& statesArray, Texture2D& provincesTexture);

void SetProvinceColoursToRandom(Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap, 
	BitmapImage& provincesBitmap, const Vector<State>& statesArray, Texture2D& provincesTexture);