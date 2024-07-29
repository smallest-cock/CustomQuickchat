#include "pch.h"
#include "TextEffects.h"



bool isVowel(char ch)
{
    ch = std::tolower(ch);
    return (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u');
}


// Function to split a string into words based on spaces
std::vector<std::string> splitIntoWords(const std::string& input)
{
    std::vector<std::string> words;
    std::string word;

    for (char ch : input)
    {
        if (std::isspace(ch) && !word.empty())
        {
			words.push_back(word);
			word.clear();
        }
        else {
            word += ch;
        }
    }

    if (!word.empty())
    {
        words.push_back(word);
    }

    return words;
}



// ==================================================== text effects ====================================================


// sarcasm effect
std::string toSarcasm(const std::string& ogText)
{
    std::istringstream iss(ogText);
    std::ostringstream oss;
    std::string word;

    // Tokenize the input string into words based on spaces
    while (iss >> word)
    {
        bool capitalizeNext = false;  // Start with lowercase for the first letter
        std::string sarcasticWord;

        // Transform each word according to the specified rules
        for (size_t i = 0; i < word.size(); ++i)
        {
            char currentChar = word[i];

            if (std::tolower(currentChar) == 'i')
            {
                sarcasticWord += 'i';  // Keep 'i' as lowercase
            }
            else if (std::tolower(currentChar) == 'l')
            {
                sarcasticWord += 'L';  // Keep 'l' as uppercase
            }
            else if (std::isalpha(currentChar))
            {  // Check if the character is alphabetic
                if (capitalizeNext)
                {
                    sarcasticWord += std::toupper(currentChar);
                    capitalizeNext = false;  // Toggle for the next character
                }
                else {
                    sarcasticWord += std::tolower(currentChar);
                    capitalizeNext = true;  // Toggle for the next character
                }
            }
            else {
                sarcasticWord += currentChar;  // Preserve non-alphabetic characters
            }
        }

        // Append the transformed word to the output stream
        oss << sarcasticWord << " ";
    }

    // Get the resulting string from the output stream
    std::string sarcasticText = oss.str();

    // Remove trailing space at the end (if any)
    if (!sarcasticText.empty() && sarcasticText.back() == ' ')
    {
        sarcasticText.pop_back();
    }

    return sarcasticText;
}


// uwu effect
std::string translateChar(char currentChar, char previousChar, char nextChar)
{
    static const std::unordered_map<char, std::string> translations = {
        {'L', "W"},
        {'R', "W"},
        {'l', "w"},
        {'r', "w"}
    };

    //// Handle special case for 'o'
    //if ((currentChar == 'o' || currentChar == 'O') &&
    //    (previousChar == 'n' || previousChar == 'N' || previousChar == 'm' || previousChar == 'M')) {
    //    return "yo";
    //}

    // Check if the current character has a translation in the map
    if (translations.count(currentChar))
    {
        return translations.at(currentChar);
    }

    // Default: return the original character
    return std::string(1, currentChar);
}

std::string translateWord(const std::string& input)
{
    std::string result;
    char previousChar = '\0'; // Initialize to null character

    for (size_t i = 0; i < input.length(); ++i)
    {
        char currentChar = input[i];
        char nextChar = '\0';
        char previousChar = '\0';

        if (i > 0)
        {
            previousChar = input[i - 1];
        }

        if (i < input.length() - 1)
        {
            nextChar = input[i + 1];

            if (currentChar == 'r' || currentChar == 'R')
            {
				if (nextChar == 'l' || nextChar == 'L' || previousChar == 'l' || previousChar == 'L')
                {
					result += currentChar;
					continue;
				}
			}
			else if (currentChar == 'l' || currentChar == 'L')
            {
				if (nextChar == 'r' || nextChar == 'R' || previousChar == 'r' || previousChar == 'R')
                {
					result += currentChar;
					continue;
				}
			}
        }
        else {  // if it's the last character in the word ...
            result += currentChar;
            continue;
        }

        result += translateChar(currentChar, previousChar, nextChar);
    }

    return result;
}


std::string toUwu(const std::string& ogText)
{
    std::vector<std::string> words = splitIntoWords(ogText);
    std::string modifiedString;

    // Iterate through each word
    for (size_t i = 0; i < words.size(); ++i)
    {
        const std::string& word = words[i];
        std::string translatedWord = translateWord(word);

        // Append translated word to modifiedString
        modifiedString += translatedWord;

        // Append space only if it's not the last word
        if (i < words.size() - 1)
        {
            modifiedString += " ";
        }
    }

    return modifiedString;
}

