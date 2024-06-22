#pragma once
#include "pch.h"
#include <random>

FString StrToFString(const std::string& s);

std::vector<std::string> splitStringByNewline(const std::string& input);

std::string cleanString(std::string str);

std::string generateRandomString(int length);