#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <array>
#include <cmath>
#include <iostream>

//  ----Rename Types----

using Char = char;
using UnsignedChar = unsigned char;

using UnsignedInteger8 = uint8_t;
using UnsignedInteger16 = uint16_t;
using UnsignedInteger32 = uint32_t;
using UnsignedInteger64 = uint64_t;

using SignedInteger8 = int8_t;
using SignedInteger16 = int16_t;
using SignedInteger32 = int32_t;
using SignedInteger64 = int64_t;

using SizeT = size_t;

using Float32 = float;
using Float64 = double;

using Boolean = bool;

using String = std::string;

using Path = std::filesystem::path;

using Timestamp = std::chrono::steady_clock::time_point;

template<typename HashKey, typename HashValue>
using HashMap = std::unordered_map<HashKey, HashValue>;

template<typename SetType>
using Set = std::unordered_set<SetType>;

template<typename VectorType>
using Vector = std::vector<VectorType>;

template<typename ArrayType, SizeT amount>
using Array = std::array<ArrayType, amount>;

struct Letter;
struct Punctuation;

struct Letter {
	UnsignedInteger16 fontSize;			//Size of the font

	UnsignedInteger32 unicodeCharacter; //The unicode character this letter represents
	Boolean controlOrSpaceCharacter;	//If true, skip checking

	UnsignedInteger8 leftAlpha;		//Alpha value of the leftmost pixel
	UnsignedInteger16 imgWidth, imgHeight;		//Image width and height
	UnsignedInteger16 realWidth, realHeight;	//Character width and height, standardised to by 255

	Float32 offsetX, offsetY;            //Character offset when drawing
	Float32 advanceX;           //Character advance position X
	Vector<UnsignedInteger8> imgData;			//Image data, alpha channel
	Vector<UnsignedInteger8> imgDataGreyAlpha;			//Image data, grey-alpha

	Letter() :
		fontSize(0), unicodeCharacter(0), controlOrSpaceCharacter(false), leftAlpha(0), imgWidth(0), imgHeight(0), realWidth(0), realHeight(0), offsetX(0.0f), offsetY(0.0f), advanceX(0.0f)
	{
	}

	Letter(const UnsignedInteger32 unicodeChar, const UnsignedInteger16 fontSize, Vector<UnsignedInteger8> imgDataIn, const UnsignedInteger16 imgWidth, const UnsignedInteger16 imgHeight, 
		const Float32 offsetX, const Float32 offsetY, const Float32 advanceX) :
		fontSize(fontSize), unicodeCharacter(unicodeChar), controlOrSpaceCharacter(false), leftAlpha(0), imgWidth(imgWidth), imgHeight(imgHeight), realWidth(0), realHeight(0),
		offsetX(offsetX), offsetY(offsetY), advanceX(advanceX)
	{
		if (unicodeChar <= 0x20 || (unicodeChar >= 0x7f && unicodeChar <= 0xa0)) {
			controlOrSpaceCharacter = true;
		}

		const UnsignedInteger32 size = imgWidth * imgHeight;
		imgData.resize(size);

		SizeT charArrayIndex = 1;
		for (SizeT imgDataIndex = 0; imgDataIndex < size; ++imgDataIndex) {
			imgData[imgDataIndex] = imgDataIn[charArrayIndex];

			charArrayIndex += 2;
		}

		imgDataGreyAlpha = Vector<UnsignedInteger8>(imgDataIn);

		if (!controlOrSpaceCharacter) {
			Crop();
			UpdateRealDimensions();
			UpdateGreyAlpha();
		}
	}

	void Crop() {
		SignedInteger32 x0 = imgWidth, y0 = imgHeight;
		SignedInteger32 x1 = -1, y1 = -1;

		// Find bounds of all non-zero alpha pixels
		for (SignedInteger32 y = 0; y < imgHeight; ++y) {
			for (SignedInteger32 x = 0; x < imgWidth; ++x) {
				UnsignedInteger8 a = imgData[y * imgWidth + x];
				if (a != 0) {
					if (x < x0) x0 = x;
					if (y < y0) y0 = y;
					if (x > x1) x1 = x;
					if (y > y1) y1 = y;
				}
			}
		}

		SignedInteger32 newW = x1 - x0 + 1;
		SignedInteger32 newH = y1 - y0 + 1;

		Vector<UnsignedInteger8> out(newW * newH);

		// Copy cropped region
		for (SignedInteger32 cy = 0; cy < newH; ++cy) {
			SignedInteger32 srcY = y0 + cy;

			std::memcpy(
				&out[cy * newW],
				&imgData[srcY * imgWidth + x0],
				newW
			);
		}

		// Replace original
		imgData.swap(out);
		imgWidth = newW;
		imgHeight = newH;

		offsetX += x0;
		offsetY += y0;
	}

	void UpdateRealDimensions() {
		const SizeT imgSize = imgWidth * imgHeight;
		UnsignedInteger8 top = 0, bottom = 0, left = 0, right = 0;

		for (SizeT i = 0; i < imgWidth; i++) {
			if (imgData[i] > top) { top = imgData[i]; }
		}
		for (SizeT i = imgSize - imgWidth; i < imgSize; i++) {
			if (imgData[i] > bottom) { bottom = imgData[i]; }
		}

		for (SizeT i = 0; i < imgSize; i += imgWidth) {
			if (imgData[i] > left) { left = imgData[i]; }
		}
		for (SizeT i = imgWidth - 1; i < imgSize; i += imgWidth) {
			if (imgData[i] > right) { right = imgData[i]; }
		}

		realWidth = ((imgWidth - 2) * 255) + left + right;
		realHeight = ((imgHeight - 2) * 255) + top + bottom;

		leftAlpha = left;
	}

	void UpdateGreyAlpha() {
		SizeT imgSize = imgWidth * imgHeight;
		imgDataGreyAlpha = Vector<UnsignedInteger8>(imgSize * 2, 255);

		SizeT index = 1;
		for (SizeT i = 0; i < imgSize; i++) {
			imgDataGreyAlpha[index] = imgData[i];

			index += 2;
		}
	}
};

struct Punctuation {
	UnsignedInteger16 fontSize;			//Size of the font

	UnsignedInteger8 leftAlpha;		//Alpha value of the leftmost pixel
	UnsignedInteger16 imgWidth, imgHeight;		//Image width and height
	UnsignedInteger16 realWidth, realHeight;	//Character width and height, standardised to by 255
	Float32 offsetY;            //Character offset when drawing
	
	Vector<UnsignedInteger8> imgData;			//Image data, alpha channel
	Vector<UnsignedInteger8> imgDataGreyAlpha;			//Image data, grey-alpha

	Punctuation() : fontSize(0), imgWidth(0), imgHeight(0), realWidth(0), realHeight(0), offsetY(0.0f), leftAlpha(0) {}

	Punctuation(const Letter& a, const Letter& b) : 
		fontSize(a.fontSize), imgWidth(a.imgWidth), imgHeight(a.imgHeight), realWidth(0), realHeight(0), offsetY(a.offsetY), leftAlpha(a.leftAlpha), imgData(), imgDataGreyAlpha() {
		if (a.offsetX != b.offsetX || a.realWidth != b.realWidth) {
			std::cout << "Letters must have the same width and x offset\n";
		}
		else if ((a.imgHeight + a.offsetY) < (b.imgHeight + b.offsetY)) {
			std::cout << "A cannot be smaller then B\n";
		}
		else {
			//Subtract letters
			SizeT bIndex = 0;
			const SizeT aIndexStart = (b.offsetY - a.offsetY) * a.imgWidth;
			const SizeT bImgSizePlusOffset = b.imgData.size() + aIndexStart;	//Should be same size as a.imgData.size(), but can be smaller
			Vector<UnsignedInteger8> resultImgData = a.imgData;

			for (SizeT i = aIndexStart; i < a.imgData.size(); i++) {
				if (i < bImgSizePlusOffset) {
					UnsignedInteger8 aa = a.imgData[i];
					UnsignedInteger8 ab = b.imgData[bIndex];
					resultImgData[i] = (a.imgData[i] > b.imgData[bIndex]) ? (a.imgData[i] - b.imgData[bIndex]) : 0;
				}
				else {
					resultImgData[i] = a.imgData[i];
				}
				bIndex++;
			}

			imgData = resultImgData;

			Crop();
			UpdateRealDimensions();
			UpdateGreyAlpha();
		}
	}

	void Crop() {
		SignedInteger32 x0 = imgWidth, y0 = imgHeight;
		SignedInteger32 x1 = -1, y1 = -1;

		// Find bounds of all non-zero alpha pixels
		for (SignedInteger32 y = 0; y < imgHeight; ++y) {
			for (SignedInteger32 x = 0; x < imgWidth; ++x) {
				UnsignedInteger8 a = imgData[y * imgWidth + x];
				if (a != 0) {
					if (x < x0) x0 = x;
					if (y < y0) y0 = y;
					if (x > x1) x1 = x;
					if (y > y1) y1 = y;
				}
			}
		}

		SignedInteger32 newW = x1 - x0 + 1;
		SignedInteger32 newH = y1 - y0 + 1;

		Vector<UnsignedInteger8> out(newW * newH);

		// Copy cropped region
		for (SignedInteger32 cy = 0; cy < newH; ++cy) {
			SignedInteger32 srcY = y0 + cy;

			std::memcpy(
				&out[cy * newW],
				&imgData[srcY * imgWidth + x0],
				newW
			);
		}

		// Replace original
		imgData.swap(out);
		imgWidth = newW;
		imgHeight = newH;

		offsetY += y0;
	}

	void UpdateRealDimensions() {
		const SizeT imgSize = imgWidth * imgHeight;
		UnsignedInteger8 top = 0, bottom = 0, left = 0, right = 0;

		for (SizeT i = 0; i < imgWidth; i++) {
			if (imgData[i] > top) { top = imgData[i]; }
		}
		for (SizeT i = imgSize - imgWidth; i < imgSize; i++) {
			if (imgData[i] > bottom) { bottom = imgData[i]; }
		}

		for (SizeT i = 0; i < imgSize; i += imgWidth) {
			if (imgData[i] > left) { left = imgData[i]; }
		}
		for (SizeT i = imgWidth - 1; i < imgSize; i += imgWidth) {
			if (imgData[i] > right) { right = imgData[i]; }
		}

		realWidth = ((imgWidth - 2) * 255) + left + right;
		realHeight = ((imgHeight - 2) * 255) + top + bottom;
		leftAlpha = left;
	}

	void UpdateGreyAlpha() {
		SizeT imgSize = imgWidth * imgHeight;
		imgDataGreyAlpha = Vector<UnsignedInteger8>(imgSize * 2, 255);

		SizeT index = 1;
		for (SizeT i = 0; i < imgSize; i++) {
			imgDataGreyAlpha[index] = imgData[i];

			index += 2;
		}
	}
};