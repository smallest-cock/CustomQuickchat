#include "pch.h"
#include "Utils.hpp"


// Various helper functions...
namespace Format
{
	// ------------------------------------ custom --------------------------------------

	wchar_t* ToWcharString(const std::string& str)
	{
		// Determine the required buffer size for the wide string
		int wcharCount = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

		// Allocate memory for the wchar_t string
		wchar_t* wStr = new wchar_t[wcharCount];

		// Convert the multibyte string (UTF-8) to a wide string
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wStr, wcharCount);

		return wStr;
	}

	std::wstring ToWString(const std::string& str)
	{
		// Determine the required buffer size for the wide string
		int wcharCount = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);

		// Allocate memory for the wchar_t string
		wchar_t* wStr = new wchar_t[wcharCount];

		// Convert the multibyte string (UTF-8) to a wide string
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wStr, wcharCount);

		return std::wstring(wStr);
	}

	FName StringToFName(const std::string& str)
	{
		return FName(ToWcharString(str));
	}

	std::string ToASCIIString(std::string str)
	{
		// Remove non-ASCII characters
		str.erase(std::remove_if(str.begin(), str.end(),
			[](unsigned char c) { return c > 127; }),
			str.end());

		return str;
	}

	std::vector<std::string> SplitStrByNewline(const std::string& input)
	{
		std::vector<std::string> lines;
		std::istringstream iss(input);
		std::string line;

		// Read each line using std::getline with '\n' as delimiter
		while (std::getline(iss, line)) {
			lines.push_back(line);
		}

		return lines;
	}

	std::vector<std::string> SplitStr(const std::string& str, const std::string& delimiter)
	{
		std::vector<std::string> tokens;
		size_t start = 0;
		size_t end = str.find(delimiter);

		while (end != std::string::npos) {
			tokens.push_back(str.substr(start, end - start));
			start = end + delimiter.length();
			end = str.find(delimiter, start);
		}

		// Add the last token
		tokens.push_back(str.substr(start, end - start));

		return tokens;
	}

	std::string EscapeCharacter(const std::string& input, char charToEscape)
	{
		std::string escapedString;
		escapedString.reserve(input.size()); // Reserve space for performance

		for (char c : input)
		{
			// Add a backslash before the character to escape
			if (c == charToEscape)
			{
				escapedString += '\\';
			}
			escapedString += c;
		}
		return escapedString;
	}

	std::string EscapeBraces(const std::string& str)
	{
		std::string escaped;
		for (char ch : str) {
			if (ch == '{' || ch == '}') {
				escaped += ch;  // Add an extra brace to escape it
			}
			escaped += ch;
		}
		return escaped;
	}

	// important "sanitizer" when using strings to initialize fs::path (a sneaky trailing newline char will make the filepath invalid)
	std::string TrimBeginningAndEndWhitespace(const std::string& str)
	{
		if (str.empty()) return str;

		// Find the position of the first non-whitespace character (leading whitespace)
		size_t start = str.find_first_not_of(" \t\n\r");

		// If the string is all whitespace, return an empty string
		if (start == std::string::npos) {
			return "";
		}

		// Find the position of the last non-whitespace character (trailing whitespace)
		size_t end = str.find_last_not_of(" \t\n\r");

		// Create a substring from the first non-whitespace to the last non-whitespace
		return str.substr(start, end - start + 1);
	}

	std::string EscapeBackslashes(const std::string& input)
	{
		std::string output;
		output.reserve(input.length());  // Pre-allocate space to avoid reallocation
		for (char ch : input) {
			if (ch == '\\') {
				output += "\\\\";  // Replace single backslash with double
			}
			else {
				output += ch;
			}
		}
		return output;
	}

	std::string ReplaceBackslashesWithForwardSlash(const std::string& input)
	{
		std::string output;
		output.reserve(input.length());  // Pre-allocate space to avoid reallocation
		for (char ch : input) {
			if (ch == '\\') {
				output += '/';  // Replace single backslash with double
			}
			else {
				output += ch;
			}
		}
		return output;
	}

	std::string GenRandomString(int length)
	{
		// Define character set
		const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

		// Initialize random number generator
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> distr(0, charset.length() - 1);

		// Generate random string
		std::string randomString;
		randomString.reserve(length);
		for (int i = 0; i < length; ++i) {
			randomString += charset[distr(gen)];
		}

		return randomString;
	}

	// ----------------------------------------------------------------------------------


	std::string ToLower(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), tolower);
		return str;
	}

	void ToLowerInline(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), tolower);
	}

    std::string RemoveAllChars(std::string str, char character)
    {
        str.erase(std::remove(str.begin(), str.end(), character), str.end());
        return str;
    }

	void RemoveAllCharsInline(std::string& str, char character)
	{
		str.erase(std::remove(str.begin(), str.end(), character), str.end());
	}


	bool IsStringHexadecimal(std::string str)
	{
		if (str.empty())
		{
			return false;
		}

		ToLowerInline(str);

		bool first = true;
		bool negative = false;
		bool found = false;

		for (char c : str)
		{
			if (first)
			{
				first = false;
				negative = (c == '-');
			}

			if (std::isxdigit(c))
			{
				found = true;
			}
			else if (!negative)
			{
				return false;
			}
		}

		return found;
	}

    std::string ToHex(void* address, bool bNotation)
    {
        return ToHex(reinterpret_cast<uint64_t>(address), sizeof(uint64_t), bNotation);
    }

    std::string ToHex(uint64_t decimal, size_t width, bool bNotation)
    {
        std::ostringstream stream;
        if (bNotation) { stream << "0x"; }
        stream << std::setfill('0') << std::setw(width) << std::right << std::uppercase << std::hex << decimal;
        return stream.str();
    }

	uint64_t ToDecimal(const std::string& hexStr)
	{
		uint64_t decimal = 0;
		std::stringstream stream;
		stream << std::right << std::uppercase << std::hex << RemoveAllChars(hexStr, '#');
		stream >> decimal;
		return decimal;
	}

	std::string ToDecimal(uint64_t hex, size_t width)
	{
		std::ostringstream stream;
		stream << std::setfill('0') << std::setw(width) << std::right << std::uppercase << std::dec << hex;
		return stream.str();
	}


    std::string ColorToHex(float colorArray[3], bool bNotation)
    {
        std::string hexStr = (bNotation ? "#" : "");
        hexStr += Format::ToHex(static_cast<uint64_t>(colorArray[0]), 2, false);
        hexStr += Format::ToHex(static_cast<uint64_t>(colorArray[1]), 2, false);
        hexStr += Format::ToHex(static_cast<uint64_t>(colorArray[2]), 2, false);
        return hexStr;
    }

    uint64_t HexToDecimal(const std::string& hexStr)
    {
        uint64_t decimal = 0;
        std::stringstream stream;
        stream << std::right << std::uppercase << std::hex << RemoveAllChars(hexStr, '#');
        stream >> decimal;
        return decimal;
    }

}


namespace Files
{
	void FindPngImages(const fs::path& directory, std::unordered_map<std::string, fs::path>& imageMap) {
		for (const auto& entry : fs::recursive_directory_iterator(directory)) {
			if (entry.is_regular_file() && entry.path().extension() == ".png") {
				// Get the filename without extension
				std::string filename = entry.path().stem().string();
				// Get the full path
				fs::path filepath = entry.path();
				// Add to map
				imageMap[filename] = filepath;
			}
		}
	}


	void OpenFolder(const fs::path& folderPath)
	{
		if (fs::exists(folderPath)) {
			ShellExecute(NULL, L"open", folderPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		else {
			LOG("Folder path does not exist: {}", folderPath.string());
		}
	}


	void FilterLinesInFile(const fs::path& filePath, const std::string& startString)
	{
		std::fstream file(filePath, std::ios::in | std::ios::out);

		if (!file.is_open())
		{
			LOG("Error: Unable to open file {}", filePath.string());
			return;
		}

		std::string line;
		std::ofstream tempFile("temp.txt"); // temp file to store filtered lines

		if (!tempFile.is_open())
		{
			LOG("Error: Unable to create temporary file");
			return;
		}

		while (std::getline(file, line))
		{
			if (line.substr(0, startString.length()) == startString)
			{
				tempFile << line << '\n';	// write the line to temp file if it starts with the given string
			}
		}

		file.close();
		tempFile.close();

		// replace original file with the temp file
		fs::remove(filePath); // remove original file
		fs::rename("temp.txt", filePath); // rename temp file to original file

		LOG("Filtered lines saved to {}", filePath.string());
	}


	std::string GetCommandOutput(const char* cmd)
	{
		std::array<char, 128> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);

		if (!pipe) return std::string();

		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
		{
			result += buffer.data();
		}

		return Format::TrimBeginningAndEndWhitespace(result);	// trim whitespace characters from beginning/end (it'll make filepaths invalid)
	}

	// Function to trim whitespace and invisible characters (such as newlines)
	std::string CleanPathStr(const std::string& path)
	{
		std::string cleanedPath = path;

		// Remove trailing whitespace or invisible characters like '\n', '\r'
		cleanedPath.erase(std::remove_if(cleanedPath.begin(), cleanedPath.end(), [](unsigned char c) { return std::isspace(c); }), cleanedPath.end());

		return cleanedPath;
	}
}
