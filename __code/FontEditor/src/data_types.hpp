#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <array>
#include <cmath>

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

struct Letter {
	UnsignedInteger16 fontSize;			//Size of the font

	UnsignedInteger16 imgWidth, imgHeight;		//Image width and height
	UnsignedInteger16 realWidth, realHeight;	//Character width and height, standardised to by 255

	SignedInteger16 offsetX, offsetY;            //Character offset when drawing
	SignedInteger16 advanceX;           //Character advance position X
	Vector<UnsignedInteger8> imgData;			//Image data, greyscale-alpha

	Letter() :
		fontSize(0), imgWidth(0), imgHeight(0), realWidth(0), realHeight(0), offsetX(0), offsetY(0), advanceX(0)
	{
	}

	Letter(const UnsignedInteger16 fontSize, const Char* imgDataIn, const UnsignedInteger16 imgWidth, const UnsignedInteger16 imgHeight, 
		const SignedInteger16 offsetX, const SignedInteger16 offsetY, const SignedInteger16 advanceX) :
		fontSize(fontSize), imgWidth(imgWidth), imgHeight(imgHeight), realWidth(0), realHeight(0), offsetX(offsetX), offsetY(offsetY), advanceX(advanceX)
	{

		const UnsignedInteger32 size = imgWidth * imgHeight;
		imgData.resize(size);

		SizeT charArrayIndex = 1;
		for (SizeT imgDataIndex = 0; imgDataIndex < size; ++imgDataIndex) {
			imgData[imgDataIndex] = imgDataIn[charArrayIndex];

			charArrayIndex += 2;
		}

		GetRealSize();
	}

	void GetRealSize() {
		const UnsignedInteger32 size = imgWidth * imgHeight;
		UnsignedInteger16 top = 0, bottom = 0, left = 0, right = 0;
		Boolean topFound = false, bottomFound = false, leftFound = false, rightFound = false;
		UnsignedInteger16 fullRows = imgHeight, fullColumns = imgWidth;

		//Top
		SignedInteger64 index = 0;
		for (SizeT y = 0; y < imgHeight; y++) {
			fullRows--;
			for (SizeT x = 0; x < imgWidth; x++) {
				if (imgData[index] > top) {
					top = imgData[index];
					topFound = true;
				}

				index++;
			}

			if (topFound) break;
		}

		//Bottom
		index = static_cast<SignedInteger64>(size) - 1;
		for (SizeT y = 0; y < imgHeight; y++) {
			fullRows--;
			for (SizeT x = 0; x < imgWidth; x++) {
				if (imgData[index] > bottom) {
					bottom = imgData[index];
					bottomFound = true;
				}

				index--;
			}

			if (bottomFound) break;
		}

		//Left
		index = 0;
		for (SizeT x = 0; x < imgWidth; x++) {
			fullColumns--;
			for (SizeT y = 0; y < imgHeight; y++) {
				if (imgData[index] > left) {
					left = imgData[index];
					leftFound = true;
				}

				index += imgWidth;
			}

			//index++;
			//index %= size;

			index -= (static_cast<SignedInteger64>(size) - 1);

			if (leftFound) break;
		}

		//Right
		index = static_cast<SignedInteger64>(size) - 1;
		for (SizeT x = 0; x < imgWidth; x++) {
			fullColumns--;
			for (SizeT y = 0; y < imgHeight; y++) {
				if (imgData[index] > right) {
					right = imgData[index];
					rightFound = true;
				}

				index -= imgWidth;
			}

			//index++;
			//index %= size;

			index += (static_cast<SignedInteger64>(size) - 1);

			if (rightFound) break;
		}

		realWidth = (fullColumns * 255) + left + right;
		realHeight = (fullRows * 255) + top + bottom;
	}

	Vector<UnsignedInteger8> GetExpandedImgData() {
		const UnsignedInteger16 fullWidth = imgWidth + std::abs(offsetX);
		const UnsignedInteger16 fullHeight = imgHeight + std::abs(offsetY);
		Vector<UnsignedInteger8> fullData(fullWidth * fullHeight, 0);

		SizeT standardIndex = 0;
		//Positive offset if negative
	}
};