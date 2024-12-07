#pragma once
#include "pch.h"
#include <random>


namespace Format
{
	// ----------------------- custom -------------------------

	wchar_t* ToWcharString(const std::string& str);	// can handle special characters

	std::wstring ToWString(const std::string& str);

	FName StringToFName(const std::string& str);

	std::string ToASCIIString(std::string str);

	std::vector<std::string> SplitStrByNewline(const std::string& input);
	std::vector<std::string> SplitStr(const std::string& str, const std::string& delimiter);

	std::string EscapeCharacter(const std::string& input, char charToEscape);

	std::string EscapeBraces(const std::string& str);

	std::string EscapeBackslashes(const std::string& input);
	std::string ReplaceBackslashesWithForwardSlash(const std::string& input);

	std::string GenRandomString(int length);

	// --------------------------------------------------------


	std::string ToLower(std::string str);
	void ToLowerInline(std::string& str);
	std::string RemoveAllChars(std::string str, char character);
	void RemoveAllCharsInline(std::string& str, char character);

	bool IsStringHexadecimal(std::string str);
	std::string ToHex(void* address, bool bNotation = true);
	std::string ToHex(uint64_t decimal, size_t width, bool bNotation = true);
	uint64_t ToDecimal(const std::string& hexStr);
	std::string ToDecimal(uint64_t hex, size_t width);

	std::string ColorToHex(float colorsArray[3], bool bNotation);
	uint64_t HexToDecimal(const std::string& hexStr);
}

namespace Files
{
	void FindPngImages(const fs::path& directory, std::unordered_map<std::string, fs::path>& imageMap);
	void OpenFolder(const fs::path& folderPath);
	void FilterLinesInFile(const fs::path& filePath, const std::string& startString);
	std::string GetCommandOutput(const char* cmd);
	std::string CleanPathStr(const std::string& path);
}
