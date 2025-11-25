#include "gui.hpp"
#include "functions.hpp"

void SetProvinceColoursBasedOnStateColour(Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap,
	BitmapImage& provincesBitmap, const Vector<State>& statesArray) {
	provinceColoursToIdMap.clear();
	Set<UnsignedInteger32> usedColours;

	//HashMap<UnsignedInteger32, UnsignedInteger32> oldColourToNewColour;

	for (const auto& state : statesArray) {
		//Skip oceans for now - maybe add bluish-purple tint later
		if (state.GetId() == 0) { continue; }
 
		UnsignedInteger16 provinceCount = state.GetProvinces().size();
		Vector<ColourRGB> availableColours = GenerateRandomColoursInRange(usedColours, provinceCount, state.GetColour(), 12);

		SizeT index = 0;
		for (const auto& provId : state.GetProvinces()) {
			//oldColourToNewColour[provincesArray[provId].GetColour().ToInteger()] = availableColours[index].ToInteger();
			provincesArray[provId].SetColour(availableColours[index++]);
		}
	}

	Vector<UnsignedInteger8> newRgbData(provincesBitmap.GetWidth() * provincesBitmap.GetHeight() * 3);
	ColourRGB colour;
	SizeT pixelIndex = 0;

	for (const auto& province : provincesArray) {
		colour = province.GetColour();
		for (const auto& pixel : province.GetPixels()) {
			pixelIndex = pixel.index * 3;
			newRgbData[pixelIndex] = colour.r;
			newRgbData[pixelIndex + 1] = colour.g;
			newRgbData[pixelIndex + 2] = colour.b;
		}
	}

	provincesBitmap = BitmapImage(newRgbData, provincesBitmap.GetWidth(), provincesBitmap.GetHeight());
}

void SetProvinceColoursToRandom(Vector<Province>& provincesArray, HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap,
	BitmapImage& provincesBitmap, const Vector<State>& statesArray) {
	provinceColoursToIdMap.clear();
	provincesArray[0].SetColour(ColourRGB(0, 0, 0));
	provinceColoursToIdMap[0] = 0;

	Vector<ColourRGB> randomColours = GenerateRandomColours(provincesArray.size());
	for (SizeT i = 1; i < provincesArray.size(); ++i) {
		if (provincesArray[i].GetProvinceType() == ProvinceType::Land) {
			provincesArray[i].SetColour(randomColours[i]);
			provinceColoursToIdMap[randomColours[i].ToInteger()] = i;
		}
		else {
			provinceColoursToIdMap[provincesArray[i].GetColour().ToInteger()] = i;
		}
	}

	Vector<UnsignedInteger8> newRgbData(provincesBitmap.GetWidth() * provincesBitmap.GetHeight() * 3);
	ColourRGB colour;
	SizeT pixelIndex = 0;

	for (const auto& province : provincesArray) {
		colour = province.GetColour();
		for (const auto& pixel : province.GetPixels()) {
			pixelIndex = pixel.index * 3;
			newRgbData[pixelIndex] = colour.r;
			newRgbData[pixelIndex + 1] = colour.g;
			newRgbData[pixelIndex + 2] = colour.b;
		}
	}

	provincesBitmap = BitmapImage(newRgbData, provincesBitmap.GetWidth(), provincesBitmap.GetHeight());
}