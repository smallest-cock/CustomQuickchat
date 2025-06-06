#include "pch.h"
#include "Structs.hpp"


// ##############################################################################################################
// ##############################################    Binding    #################################################
// ##############################################################################################################

bool Binding::shouldBeTriggered(
    const ButtonPress& buttonEvent,
    const std::unordered_map<std::string, bool>& keyStates,
    const std::chrono::steady_clock::time_point& lastChatSent,
    const std::chrono::steady_clock::time_point& epochTime,
    const std::chrono::duration<double>& minDelayBetweenBindings,
    const std::chrono::duration<double>& maxTimeWindow)
{
    switch (bindingType)
    {
    case EBindingType::Combination:
        return checkCombination(buttonEvent, keyStates, lastChatSent, minDelayBetweenBindings);
    case EBindingType::Sequence:
        return checkSequence(buttonEvent, lastChatSent, epochTime, minDelayBetweenBindings, maxTimeWindow);
    default:
        return false;   // if there's no valid binding type for some reason
    }
}

bool Binding::checkCombination(
    const ButtonPress& buttonEvent,
    const std::unordered_map<std::string, bool>& keyStates,
    const std::chrono::steady_clock::time_point& lastBindingActivated,
    const std::chrono::duration<double>& minDelayBetweenBindings)
{
    if (buttons.empty())
        return false;

    for (const std::string& button : buttons)
    {
        if (keyStates.contains(button))
        {
            if (!keyStates.at(button))
                return false;
        }
    }

    // check if event happened AFTER minBindingDelay
    return buttonEvent.pressedTime > lastBindingActivated + minDelayBetweenBindings;
}

bool Binding::checkSequence(
    const ButtonPress& buttonEvent,
    const std::chrono::steady_clock::time_point& lastChatSent,
    const std::chrono::steady_clock::time_point& epochTime,
    const std::chrono::duration<double>& minDelayBetweenBindings,
    const std::chrono::duration<double>& maxTimeWindow)
{
    if (buttons.size() < 2)
        return false;   // exit if there's not at least 2 buttons in binding

    bool button1Pressed = buttonEvent.buttonName == buttons[0];
    bool button2Pressed = buttonEvent.buttonName == buttons[1];

    if (!button1Pressed && !button2Pressed)
        return false;   // early exit if no buttons from binding have been pressed

    // if first button press data is empty...
    if (firstButtonState.buttonName.empty() || firstButtonState.pressedTime == epochTime)
    {
        if (button1Pressed)
            firstButtonState = buttonEvent;     // update first button press data then exit
        return false;
    }

    // if first button press data exists.......

    // if first button press data is too old... reset or update it, then exit
    if (buttonEvent.pressedTime > firstButtonState.pressedTime + maxTimeWindow)
    {
        if (button1Pressed)
            firstButtonState = buttonEvent;     // update first button press data
        else
            firstButtonState.Reset(epochTime);  // reset info bc 1st button doesn't match
        return false;
    }

    // if first button press data is still valid.......

    if (!button2Pressed)
        return false;

    // make sure 2nd button pressed in appropriate time window (AFTER minBindingDelay and BEFORE sequenceTimeWindow)
    bool correct1stButtonPressed = firstButtonState.buttonName == buttons[0];
    bool button2PressedLateEnough = buttonEvent.pressedTime > firstButtonState.pressedTime + minDelayBetweenBindings;

    if (correct1stButtonPressed)
    {
        if (button2PressedLateEnough)
        {
            firstButtonState.Reset(epochTime);
            return true;
        }

        firstButtonState.Reset(epochTime);  // binding was triggered too early, just reset it (bc it prolly wasn't meant to be triggered)
    }

    return false;
}

void Binding::updateKeywordAndTextEffect(const std::string& regexPatternStr)
{
    std::vector<std::string> matchedSubstrings = getMatchedSubstrings(chat, regexPatternStr);

    // handle any words in double brackets, like special keywords or word variations
    for (const std::string& stringFoundInBrackets : matchedSubstrings)
    {
        auto it = keywordsMap.find(stringFoundInBrackets);

        // if a special keyword was found
        if (it != keywordsMap.end())
        {
            keyWord = it->second;                   // update binding's keyword
            textEffect = getTextEffect(keyWord);    // update binding's text effect (if any)
        }
        // if something else was found in double brackets (aka a word variation)
        else if (keyWord == EKeyword::None)
        {
            keyWord = EKeyword::WordVariation;
        }
    }
}

ETextEffect Binding::getTextEffect(EKeyword keyword)
{
    switch (keyword)
    {
    case EKeyword::LastChatUwu:
    case EKeyword::SpeechToTextUwu:
        return ETextEffect::Uwu;
    case EKeyword::LastChatSarcasm:
    case EKeyword::SpeechToTextSarcasm:
        return ETextEffect::Sarcasm;
    default:
        return ETextEffect::None;
    }
}

std::vector<std::string> Binding::getMatchedSubstrings(const std::string& str, const std::string& regexPatternStr)
{
    std::regex regexPattern(regexPatternStr);

    std::vector<std::string> matchedSubstrings;
    std::sregex_iterator it(str.begin(), str.end(), regexPattern);
    std::sregex_iterator end;

    while (it != end)
    {
        std::string matchedSubstring = (*it)[1].str();
        matchedSubstrings.push_back(matchedSubstring);
        ++it;
    }

    return matchedSubstrings;
}


// ##############################################################################################################
// ############################################    VariationList    #############################################
// ##############################################################################################################

std::vector<std::string> VariationList::generateShuffledWordList() const
{
    static std::mt19937 rng(std::random_device{}()); // seeded once, reused across calls.. for efficiency and better randomization

    std::vector<std::string> shuffledList = wordList;
    std::shuffle(shuffledList.begin(), shuffledList.end(), rng);
    return shuffledList;
}

std::string VariationList::getNextVariation()
{
    std::string nextVariation;
    if (wordList.empty())
        return nextVariation;

    if (shuffleWordList)
    {
        if (wordList.size() < 3)
        {
            LOG("ERROR: \"{}\" variation list has less than 3 items and cannot be used", listName);
            return listName;
        }

        nextVariation = shuffledWordList[nextUsableIndex];

        if (nextUsableIndex != (shuffledWordList.size() - 1))
            nextUsableIndex++;
        else
            reshuffleWordList();
    }
    else
    {
        nextVariation = wordList[nextUsableIndex];
        nextUsableIndex = (nextUsableIndex == wordList.size() - 1) ? 0 : nextUsableIndex + 1; // loop the index
    }

    return nextVariation;
}

void VariationList::reshuffleWordList()
{
    // skip the non-repetition stuff if word list has less than 4 items
    if (shuffledWordList.size() < 4)
    {
        shuffledWordList = generateShuffledWordList();
        nextUsableIndex = 0;
        return;
    }

    // save last two words from previous shuffled list
    std::vector<std::string> lastTwoWords;
    lastTwoWords.push_back(shuffledWordList[shuffledWordList.size() - 1]);
    lastTwoWords.push_back(shuffledWordList[shuffledWordList.size() - 2]);

    // create new shuffled list
    std::vector<std::string> newShuffledList = generateShuffledWordList();

    std::string newShuffled1st = "";
    std::string newShuffled2nd = "";


    // find 1st different variation
    for (int i = 0; i < newShuffledList.size(); ++i)
    {
        auto word = newShuffledList[i];

        auto it = std::find(lastTwoWords.begin(), lastTwoWords.end(), word);
        if (it == lastTwoWords.end() && newShuffled1st == "")
        {
            newShuffled1st = word;
            newShuffledList.erase(newShuffledList.begin() + i);
            break;
        }
    }

    // find 2nd different variation
    for (int i = 0; i < newShuffledList.size(); ++i)
    {
        auto word = newShuffledList[i];

        auto it = std::find(lastTwoWords.begin(), lastTwoWords.end(), word);
        if (it == lastTwoWords.end() && newShuffled2nd == "")
        {
            newShuffled2nd = word;
            newShuffledList.erase(newShuffledList.begin() + i);
            break;
        }
    }

    // insert selected words (that are diff than prev last two) at beginning of new shuffled vector
    newShuffledList.insert(newShuffledList.begin(), newShuffled1st);
    newShuffledList.insert(newShuffledList.begin() + 1, newShuffled2nd);

    // update actual variation list info
    shuffledWordList = newShuffledList;
    nextUsableIndex = 0;
}

void VariationList::updateDataFromUnparsedString()
{
    wordList = Format::SplitStrByNewline(unparsedString); // update word list
    nextUsableIndex = 0; // reset index
    shuffledWordList = generateShuffledWordList();
}