#pragma once
#include "pch.h"
#include <regex>


// ------------------------------ for imgui -----------------------------------

const std::vector<std::string> possibleBindingTypes =
{
	"button combination",
	"button sequence"
};

const std::vector<std::string> possibleChatModes =
{
	"lobby",
	"team",
	"party"
};



// ---------------------------- keyword stuff ---------------------------------

enum class EKeyword : uint8_t
{
	None =					0,
	WordVariation =			1,
	SpeechToText =			2,
	SpeechToTextUwu =		3,
	SpeechToTextSarcasm =	4,
	LastChat =				5,
	LastChatUwu =			6,
	LastChatSarcasm =		7,
	BlastAll =				8,
	BlastCasual =			9,
	Blast1v1 =				10,
	Blast2v2 =				11,
	Blast3v3 =				12,
	Forfeit =				13,
	ExitToMainMenu =		14
};

enum class ETextEffect : uint8_t
{
	None =			0,
	Uwu =			1,
	Sarcasm =		2,
};

const std::unordered_map<std::string, EKeyword> keywordsMap =
{
	{ "speechToText",				EKeyword::SpeechToText			},
	{ "speechToText sarcasm",		EKeyword::SpeechToTextSarcasm	},
	{ "speechToText uwu",			EKeyword::SpeechToTextUwu		},
	{ "lastChat",					EKeyword::LastChat				},
	{ "lastChat uwu",				EKeyword::LastChatUwu			},
	{ "lastChat sarcasm",			EKeyword::LastChatSarcasm		},
	{ "blast all",					EKeyword::BlastAll				},
	{ "blast casual",				EKeyword::BlastCasual			},
	{ "blast 1v1",					EKeyword::Blast1v1				},
	{ "blast 2v2",					EKeyword::Blast2v2				},
	{ "blast 3v3",					EKeyword::Blast3v3				},
	{ "forfeit",					EKeyword::Forfeit				},
	{ "menu",						EKeyword::ExitToMainMenu		},
};

// ----------------------------------------------------------------------------


enum class EBindingType : uint8_t
{
	Combination =	0,
	Sequence =		1
};


struct ButtonPress
{
	std::string buttonName;
	std::chrono::steady_clock::time_point pressedTime;


	// default (empty)
	ButtonPress()
		: buttonName(std::string()), pressedTime(std::chrono::steady_clock::time_point()) {}

	// specific button press event
	ButtonPress(const std::string& button, const std::chrono::steady_clock::time_point& time)
		: buttonName(button), pressedTime(time) {}
	
	void Reset(const std::chrono::steady_clock::time_point& epochTime)
	{
		buttonName.clear();
		pressedTime = epochTime;
	}
};


struct BindingKey
{
	std::string action;
	std::string pc_key;
	std::string gamepad_key;
};


struct Binding
{
	std::string chat;
	EChatChannel chatMode =				EChatChannel::EChatChannel_Match;
	EBindingType bindingType =			EBindingType::Combination;
	EKeyword keyWord =					EKeyword::None;
	ETextEffect textEffect =			ETextEffect::None;
	std::vector<std::string> buttons;
	ButtonPress firstButtonState;


	bool ShouldBeTriggered(
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
			return CheckCombination(buttonEvent, keyStates, lastChatSent, minDelayBetweenBindings);
		case EBindingType::Sequence:
			return CheckSequence(buttonEvent, lastChatSent, epochTime, minDelayBetweenBindings, maxTimeWindow);
		default:
			return false;	// if there's no valid binding type for some reason
		}
	}
	
	bool CheckCombination(
		const ButtonPress& buttonEvent,
		const std::unordered_map<std::string, bool>& keyStates,
		const std::chrono::steady_clock::time_point& lastBindingActivated,
		const std::chrono::duration<double>& minDelayBetweenBindings)
	{
		if (buttons.empty()) return false;

		for (const std::string& button : buttons)
		{
			if (keyStates.contains(button))
			{
				if (!keyStates.at(button)) return false;
			}
		}

		// check if event happened AFTER minBindingDelay
		return buttonEvent.pressedTime > lastBindingActivated + minDelayBetweenBindings;
	}

	bool CheckSequence(
		const ButtonPress& buttonEvent,
		const std::chrono::steady_clock::time_point& lastChatSent,
		const std::chrono::steady_clock::time_point& epochTime,
		const std::chrono::duration<double>& minDelayBetweenBindings,
		const std::chrono::duration<double>& maxTimeWindow)
	{
		if (buttons.size() < 2) return false;	// exit if there's not at least 2 buttons in binding

		bool button1Pressed = buttonEvent.buttonName == buttons[0];
		bool button2Pressed = buttonEvent.buttonName == buttons[1];

		if (!button1Pressed && !button2Pressed) return false;	// early exit if no buttons from binding have been pressed

		// if first button press data is empty...
		if (firstButtonState.buttonName.empty() || firstButtonState.pressedTime == epochTime)
		{
			if (button1Pressed)
				firstButtonState = buttonEvent;		// update first button press data then exit
			return false;
		}

		// if first button press data exists.......
		
		// if first button press data is too old... reset or update it, then exit
		if (buttonEvent.pressedTime > firstButtonState.pressedTime + maxTimeWindow)
		{
			if (button1Pressed)
				firstButtonState = buttonEvent;		// update first button press data
			else
				firstButtonState.Reset(epochTime);	// reset info bc 1st button doesn't match
			return false;
		}

		// if first button press data is still valid.......

		if (!button2Pressed) return false;

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

			firstButtonState.Reset(epochTime);	// binding was triggered too early, just reset it (bc it prolly wasn't meant to be triggered)
		}

		return false;
	}

	// determine if chat contains any special keyword or text effect (once, at the time of binding creation, rather than every time binding is triggered)
	void UpdateKeywordAndTextEffect(const std::string& regexPatternStr)
	{
		std::vector<std::string> matchedSubstrings = GetMatchedSubstrings(chat, regexPatternStr);
		
		// handle any words in double brackets, like special keywords or word variations
		for (const std::string& stringFoundInBrackets : matchedSubstrings)
		{
			auto it = keywordsMap.find(stringFoundInBrackets);

			// if a special keyword was found
			if (it != keywordsMap.end())
			{
				keyWord = it->second;					// update binding's keyword
				textEffect = GetTextEffect(keyWord);	// update binding's text effect (if any)
			}
			// if something else was found in double brackets (aka a word variation)
			else if (keyWord == EKeyword::None)
			{
				keyWord = EKeyword::WordVariation;
			}
		}
	}

	static ETextEffect GetTextEffect(EKeyword keyword)
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

	static std::vector<std::string> GetMatchedSubstrings(const std::string& str, const std::string& regexPatternStr)
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
};


struct VariationList
{
	std::string listName;
	std::string unparsedString;
	std::vector<std::string> wordList;
	std::vector<std::string> shuffledWordList;
	int nextUsableIndex = 0;
};



enum class ERankPlaylists : uint8_t
{
	Ones =		0,
	Twos =		1,
	Threes =	2,
	Casual =	3
};

struct Rank
{
	int matches = 0;
	int mmr = 0;
	std::string div;
	std::string tier;


	inline std::string get_rank_str() const
	{
		if (div == "n/a" || tier == "n/a" || matches == 0)
		{
			return "--";
		}
		return tier + "..div" + div;
	}
};


struct ChatterRanks
{
	std::string playerName;
	Rank ones;
	Rank twos;
	Rank threes;
	Rank casual;
	//std::unordered_map <std::string, Rank> ranks;

	inline Rank get_rank(ERankPlaylists playlist)
	{
		switch (playlist)
		{
		case ERankPlaylists::Ones:
			return ones;
		case ERankPlaylists::Twos:
			return twos;
		case ERankPlaylists::Threes:
			return threes;
		case ERankPlaylists::Casual:
			return casual;
		default:
			return Rank();
		}
	}


	inline std::string get_all_ranks_str() const
	{
		// to make return line readable
		std::string ones_str =		ones.get_rank_str();
		std::string twos_str =		twos.get_rank_str();
		std::string threes_str =	threes.get_rank_str();

		return playerName + ": [1s] " + ones_str + " [2s] " + twos_str + " [3s] " + threes_str;
	}


	inline std::string get_playlist_rank_str(ERankPlaylists playlist)
	{
		std::string rank_str;

		Rank specificRank = get_rank(playlist);

		rank_str = playerName + " [" + ChatterRanks::get_playlist_str(playlist) + "] ";

		if (playlist == ERankPlaylists::Casual)
		{
			rank_str += "** " + std::to_string(specificRank.matches) + " matches played **";
		}
		else if (specificRank.tier != "n/a" || specificRank.div != "n/a")
		{
			rank_str += "** " + specificRank.tier + " div" + specificRank.div;
			rank_str += specificRank.matches == 0 ? " ** (prev season MMR)" : " ** (" + std::to_string(specificRank.matches) + " matches)";
		}
		else
		{
			rank_str += "** doesnt play ** (" + std::to_string(specificRank.matches) + " matches)";
		}

		return rank_str;
	}
	
	
	static inline std::string get_playlist_str(ERankPlaylists playlist)
	{
		switch (playlist)
		{
		case ERankPlaylists::Ones:
			return "1v1";
		case ERankPlaylists::Twos:
			return "2v2";
		case ERankPlaylists::Threes:
			return "3v3";
		case ERankPlaylists::Casual:
			return "casual";
		default:
			return "";
		}
	}
};
