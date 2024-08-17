#pragma once
#include "pch.h"



const std::vector<std::string> possibleBindingTypes = {
	"button combination",
	"button sequence"
};

const std::vector<std::string> possibleChatModes = {
	"lobby",
	"team",
	"party"
};


enum ChatMode {
	Lobby = 0,
	Team = 1,
	Party = 2
};


struct Binding {
	std::vector<int> buttonNameIndexes;
	std::string chat;
	int typeNameIndex;
	int chatMode;
};


struct VariationList {
	std::string listName;
	std::string unparsedString;
	std::vector<std::string> wordList;
	std::vector<std::string> shuffledWordList;
	int nextUsableIndex;
};


struct Rank {
	int matches;
	std::string div;
	std::string tier;
	int mmr;
};


struct ChatterRanks {
	std::string playerName;
	std::unordered_map <std::string, Rank> ranks;
};


struct ButtonPress {
	std::string buttonName;
	std::chrono::steady_clock::time_point pressedTime;
};