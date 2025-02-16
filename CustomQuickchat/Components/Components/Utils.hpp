#pragma once
#include "pch.h"
#include <random>


namespace Format
{
	inline std::string ToString(const std::wstring& str)
	{
		if (str.empty()) return "";
		int32_t size = WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, nullptr, 0, nullptr, nullptr);
		if (size <= 0) return "";
		std::string return_str(size - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, return_str.data(), size, nullptr, nullptr);
		return return_str;
	}

	inline std::wstring ToWideString(const std::string& str)
	{
		if (str.empty()) return L"";
		int32_t size = MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, nullptr, 0);
		if (size <= 0) return L"";
		std::wstring return_str(size - 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, return_str.data(), size);
		return return_str;
	}


	std::string ToHexString(uintptr_t address);
	std::string ToHexString(int32_t decimal_val, int32_t min_hex_digits);

	std::string ToASCIIString(std::string str);

	std::vector<std::string> SplitStrByNewline(const std::string& input);
	std::vector<std::string> SplitStr(const std::string& str, const std::string& delimiter);

	std::string EscapeBraces(const std::string& str);

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
	uintptr_t HexToIntPointer(const std::string& hexStr);
}

namespace Files
{
	void FindPngImages(const fs::path& directory, std::unordered_map<std::string, fs::path>& imageMap);
	void OpenFolder(const fs::path& folderPath);

	std::string get_text_content(const fs::path& file_path);
	json get_json(const fs::path& file_path);
	bool write_json(const fs::path& file_path, const json& j);

	void FilterLinesInFile(const fs::path& filePath, const std::string& startString);
	std::string GetCommandOutput(const char* cmd);
	std::string CleanPathStr(const std::string& path);
}


namespace Process
{
	struct ProcessHandles
	{
		HANDLE hProcess =	NULL;
		HANDLE hThread =	NULL;

		inline bool is_active() const
		{
			return (hProcess != NULL && hProcess != INVALID_HANDLE_VALUE) || (hThread != NULL && hThread != INVALID_HANDLE_VALUE);
		}
	};

	struct CreateProcessResult
	{
		DWORD status_code = ERROR_SUCCESS;
		ProcessHandles handles;
	};


	void close_handle(HANDLE h);
	void terminate(HANDLE h);
	void terminate_created_process(ProcessHandles& pi);

	CreateProcessResult create_process_from_command(const std::string& command);
}
