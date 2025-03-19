#pragma once

#include <string>
#include <sstream>
#include <random>
#include <cctype>


std::string to_sarcasm(const std::string& ogText);
std::string to_sarcasm_randomized(const std::string& ogText);

std::string to_uwu(const std::string& ogText);