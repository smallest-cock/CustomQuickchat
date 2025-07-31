#pragma once
#include <string>


namespace TextEffects
{
    std::string toSarcasm(const std::string& ogText);
    std::string toSarcasmRandomized(const std::string& ogText);
    std::string toUwu(const std::string& ogText);
}