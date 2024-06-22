#include "pch.h"
#include "Utils.hpp"


FString StrToFString(const std::string& s) {
	wchar_t* p = new wchar_t[s.size() + 1];
	for (std::string::size_type i = 0; i < s.size(); ++i)
		p[i] = s[i];
	p[s.size()] = '\0';
	return FString(p);
}



std::vector<std::string> splitStringByNewline(const std::string& input) {
	std::vector<std::string> lines;
	std::istringstream iss(input);
	std::string line;

	// Read each line using std::getline with '\n' as delimiter
	while (std::getline(iss, line)) {
		lines.push_back(line);
	}

	return lines;
}


std::string cleanString(std::string str) {
	// Remove non-ASCII characters
	str.erase(std::remove_if(str.begin(), str.end(),
		[](unsigned char c) { return c > 127; }),
		str.end());

	return str;
}


std::string generateRandomString(int length) {
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