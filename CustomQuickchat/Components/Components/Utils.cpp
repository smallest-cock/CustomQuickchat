#include "pch.h"
#include "Utils.hpp"


// Various helper functions...
namespace Format
{
	std::string ToHexString(uintptr_t address)
	{
		// Adjust width based on the platform's pointer size
		constexpr int pointerWidth = sizeof(uintptr_t) * 2;
		return std::format("0x{:0{}}", address, pointerWidth);
	}

	std::string ToHexString(int32_t decimal_val, int32_t min_hex_digits)
	{
		return std::format("0x{:0{}X}", decimal_val, min_hex_digits);
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

	uintptr_t HexToIntPointer(const std::string& hexStr)
	{
		uintptr_t decimal = 0;
		std::stringstream stream;

		//std::string sanitizedHex = RemoveAllChars(hexStr, '#');		// Remove unnecessary characters if needed

		stream << std::hex << hexStr;							// Load the hex string into the stream for conversion

		// Attempt conversion and handle errors
		if (!(stream >> decimal))
		{
			throw std::invalid_argument("Invalid hexadecimal string: " + hexStr);
		}

		return decimal;
	}
}



namespace Files
{
	void FindPngImages(const fs::path& directory, std::unordered_map<std::string, fs::path>& imageMap)
	{
		for (const auto& entry : fs::recursive_directory_iterator(directory))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".png")
			{
				std::string filename = entry.path().stem().string();
				fs::path filepath = entry.path();
				
				imageMap[filename] = filepath;	// Add to map
			}
		}
	}


	void OpenFolder(const fs::path& folderPath)
	{
		if (fs::exists(folderPath))
		{
			ShellExecute(NULL, L"open", folderPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		else {
			LOG("Folder path does not exist: {}", folderPath.string());
		}
	}


	std::string get_text_content(const fs::path& file_path)
	{
		if (!fs::exists(file_path))
		{
			LOG("[ERROR] File doesn't exist: '{}'", file_path.string());
			return std::string();
		}

		std::ifstream Temp(file_path);
		std::stringstream Buffer;
		Buffer << Temp.rdbuf();
		return Buffer.str();
	}

	json get_json(const fs::path& file_path)
	{
		json j;

		if (!fs::exists(file_path))
		{
			LOG("[ERROR] File doesn't exist: '{}'", file_path.string());
			return j;
		}

		try
		{
			std::ifstream file(file_path);
			file >> j;
		}
		catch (const std::exception& e)
		{
			LOG("[ERROR] Unable to read '{}': {}", file_path.filename().string(), e.what());
		}

		return j;
	}

	bool write_json(const fs::path& file_path, const json& j)
	{
		try
		{
			std::ofstream file(file_path);

			if (file.is_open())
			{
				file << j.dump(4); // pretty-print with 4 spaces indentation
				file.close();
			}
			else {
				LOG("[ERROR] Couldn't open file for writing: {}", file_path.string());
			}

			return true;
		}
		catch (const std::exception& e)
		{
			LOG("[ERROR] Unable to write to '{}': {}", file_path.filename().string(), e.what());
			return false;
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


namespace Process
{
	void close_handle(HANDLE h)
	{
		if (h != NULL && h != INVALID_HANDLE_VALUE)
		{
			if (!CloseHandle(h))
			{
				LOG("CloseHandle failed with error: {}", GetLastError());
			}
		}
		else
		{
			LOG("Unable to close handle. The handle is NULL or an invalid value");
		}
	}

	void terminate(HANDLE h)
	{
		if (h != NULL && h != INVALID_HANDLE_VALUE)
		{
			if (!TerminateProcess(h, 1))
			{
				LOG("TerminateProcess failed with error: {}", GetLastError());
			}
		}
		else
		{
			LOG("Unable to terminate process. The handle is NULL or an invalid value");
		}
	}

	void terminate_created_process(ProcessHandles& pi)
	{
		terminate(pi.hProcess);
		close_handle(pi.hProcess);
		close_handle(pi.hThread);
	}

	CreateProcessResult create_process_from_command(const std::string& command)
	{
		// CreateProcess variables
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		// Initialize STARTUPINFO & PROCESS_INFORMATION
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		std::wstring wide_command = Format::ToWideString(command);

		// initialize result
		CreateProcessResult result;

		// Create the process to start python script
		if (CreateProcessW(
			NULL,								// Application name (set NULL to use command)
			wide_command.data(),				// Command
			NULL,								// Process security attributes
			NULL,								// Thread security attributes
			FALSE,								// Inherit handles from the calling process
			CREATE_NEW_CONSOLE,					// Creation flags (use CREATE_NEW_CONSOLE for async execution)
			NULL,								// Use parent's environment block
			NULL,								// Use parent's starting directory
			&si,								// Pointer to STARTUPINFO
			&pi									// Pointer to PROCESS_INFORMATION
		))
		{
			// Duplicate process handle so it remains valid even after original PROCESS_INFORMATION goes out of scope
			HANDLE duplicatedProcessHandle = NULL;
			HANDLE duplicatedThreadHandle = NULL;

			if (DuplicateHandle(GetCurrentProcess(), pi.hProcess, GetCurrentProcess(), &duplicatedProcessHandle, 0, FALSE, DUPLICATE_SAME_ACCESS) &&
				DuplicateHandle(GetCurrentProcess(), pi.hThread, GetCurrentProcess(), &duplicatedThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
			{
				result.handles.hProcess = duplicatedProcessHandle;
				result.handles.hThread = duplicatedThreadHandle;
			}
			else
			{
				result.status_code = GetLastError();
			}

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			// If CreateProcess failed, return the error code
			result.status_code = GetLastError();
		}

		return result;
	}
}
