#pragma once
#include "mapTypes.hpp"
#include <filesystem>

void loadMap(
	const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath,

	PDX::vectorStringIndexMap<PDX::terrain>& terrainArray,
	PDX::vectorStringIndexMap<PDX::building>& stateBuildingsArray,
	PDX::vectorStringIndexMap<PDX::building>& provinceBuildingsArray,
	PDX::vectorStringIndexMap<PDX::resource>& resourcesArray,
	PDX::vectorStringIndexMap<PDX::state_category>& stateCategoryArray,
	PDX::vectorStringIndexMap<PDX::country>& countriesArray,
	std::vector<PDX::province>& provincesArray,
	std::vector<PDX::state>& statesArray,
	std::vector<PDX::strategic_region>& strategicRegionsArray,

	BMP::bitmapImage& provincesBMP,
	BMP::bitmapImage& riversBMP,
	BMP::bitmapImage& heightmapBMP,
	BMP::bitmapImage& statesBMP,
	BMP::bitmapImage& stateBordersBMP,

	std::unordered_map<std::tuple<uint8_t, uint8_t, uint8_t>, uint16_t, RGBHash>& provinceColourToProvinceIDMap,
	std::unordered_map<std::tuple<uint8_t, uint8_t, uint8_t>, uint16_t, RGBHash>& stateColourToStateIDMap
);