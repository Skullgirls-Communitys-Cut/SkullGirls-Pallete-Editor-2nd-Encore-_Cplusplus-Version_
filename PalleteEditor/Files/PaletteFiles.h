#pragma once

class PalleteFile {
public:
	static bool LoadFromFile(std::string filePath);
	static bool SaveToFile(std::string filePath);
};