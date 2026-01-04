#include <cstdint>
#include <string>
#include <cstring>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>

struct TrueTypeHeader {
	uint32_t sfntVersion;		//Font format, 00 01 00 00 for ttf
	uint16_t numTables;			//No. of tables in the file
	uint16_t searchRange;		//(largest power of 2 ≤ numTables) * 16
	uint16_t entrySelector;		//log2(largestPowerOf2 ≤ numTables)
	uint16_t rangeShift;		//numTables*16 - searchRange
	
	void SwapEndian() {
		sfntVersion = __builtin_bswap32(sfntVersion);
		numTables = __builtin_bswap16(numTables);
		searchRange = __builtin_bswap16(searchRange);
		entrySelector = __builtin_bswap16(entrySelector);
		rangeShift = __builtin_bswap16(rangeShift);
	}
};

struct TableEntry {
	uint32_t name;				//Table name
	uint32_t checksum;			//Table checksum
	uint32_t position;			//Index in bytes from the beginning of the file
	uint32_t size;				//Size of the table in bytes
	char* data;					//Font data
	
	void SwapEndian() {
		name = __builtin_bswap32(name);
		checksum = __builtin_bswap32(checksum);
		position = __builtin_bswap32(position);
		size = __builtin_bswap32(size);
	}
	
	~TableEntry() {
		delete[] data;
	}
};

struct OS2TableEntry {
	
};

static std::string RemoveQuotes(std::string str) {
    char first = str.front();
    char last = str.back();

    if ((first == 34 && last == 34) || (first == 39 && last == 39)) { str = str.substr(1, str.size() - 2); }
    return str;
}

static uint32_t GetChecksum(const char* data, const uint32_t length) {
    uint32_t sum = 0;
    uint32_t padded = (length + 3) & ~3;

    for (uint32_t i = 0; i < padded; i += 4) {
        uint32_t word = 0;
        for (int b = 0; b < 4; ++b) {
            uint32_t idx = i + b;
            word <<= 8;
            if (idx < length)
                word |= data[idx];
        }
        sum += word;
    }
    return sum;
}

int main() {
	while (true) {
		//Get the font file
		std::string fontPathString;
		std::filesystem::path fontPath;
		std::cout << "Enter the directory of the font to break: ";
		std::cin >> fontPathString;
		fontPath = RemoveQuotes(fontPathString);
		
		if (!std::filesystem::exists(fontPath) || fontPath.extension() != ".ttf") {
			std::cout << "\nThat is not a valid font path.\n";
			continue;
		}
		
		//Open the font file
		std::ifstream file(fontPath.string(), std::ios::binary | std::ios::ate);
		if (!file) {
			std::cout << "\nCannot open font file.\n";
			continue;
		}
		size_t fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		char* data = new char[fileSize];
		file.read(data, fileSize);
		
		//Get the header (first 12 bytes)
		TrueTypeHeader header;
		std::memcpy(&header, data, sizeof(TrueTypeHeader));
		header.SwapEndian();
		
		//Now get the table entries
		const size_t tableCount = header.numTables;
		TableEntry* tableEntries = new TableEntry[tableCount];
		
		char* checksumAdjustment = NULL;
		
		for (size_t i = 0; i < tableCount; i++) {		
			std::memcpy(&tableEntries[i], data + (i * 16) + 12, sizeof(TableEntry));
			
			tableEntries[i].SwapEndian();
			tableEntries[i].data = new char[tableEntries[i].size];
			std::memcpy(tableEntries[i].data, data + tableEntries[i].position, tableEntries[i].size);
			
			//Handle head table checksum
			if (tableEntries[i].name == 0x68656164) {
				checksumAdjustment = tableEntries[i].data + 8;
				std::memset(checksumAdjustment, 0, 4);
			}
			//Handle the OS/2 flags
			if (tableEntries[i].name == 0x4f532f32) {
				std::memset(tableEntries[i].data + 4, 0, 2);
			}
			
			uint32_t checksum = __builtin_bswap32(GetChecksum(data + tableEntries[i].position, tableEntries[i].size));
			std::memcpy(data + (i * 16) + 16,&checksum, sizeof(uint32_t));
		}
		
		//Handle checksum adjustment
		if (checksumAdjustment != NULL) {
			uint32_t tableChecksum = __builtin_bswap32(0xB1B0AFBA - GetChecksum(data, fileSize));
			
			std::memcpy(checksumAdjustment, &tableChecksum, sizeof(uint32_t));
			
			std::string fontOutName = fontPath.parent_path().string() + "\\" + fontPath.stem().string() + "_broken.ttf";
			std::ofstream fontOut(fontOutName, std::ios::binary);
			fontOut.write(data, fileSize);
			fontOut.close();
			
			//std::cout << "\n" << fontPath.parent_path() << "\n" << fontPath.filename() << "\n" << fontPath.stem() << "\n" << fontPath.extension() << "\n";
		}
		
		delete[] tableEntries;
		delete[] data;
	}
	
	return 0;
}