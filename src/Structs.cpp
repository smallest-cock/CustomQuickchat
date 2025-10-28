#include "pch.h"
#include "Structs.hpp"
#include <regex>
#include <random>
#include <ModUtils/util/Utils.hpp>

// ##############################################################################################################
// ##############################################    Binding    #################################################
// ##############################################################################################################

void Binding::updateKeywordAndTextEffect(const std::string& regexPatternStr)
{
	std::vector<std::string> matchedSubstrings = getMatchedSubstrings(chat, regexPatternStr);

	// handle any words in double brackets, like special keywords or word variations
	for (const std::string& stringFoundInBrackets : matchedSubstrings)
	{
		auto it = g_keywordsMap.find(stringFoundInBrackets);

		// if a special keyword was found
		if (it != g_keywordsMap.end())
		{
			keyWord    = it->second;             // update binding's keyword
			textEffect = getTextEffect(keyWord); // update binding's text effect (if any)
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
	std::sregex_iterator     it(str.begin(), str.end(), regexPattern);
	std::sregex_iterator     end;

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
		nextVariation   = wordList[nextUsableIndex];
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
		nextUsableIndex  = 0;
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
	nextUsableIndex  = 0;
}

void VariationList::updateDataFromUnparsedString()
{
	wordList         = Format::SplitStrByNewline(unparsedString); // update word list
	nextUsableIndex  = 0;                                         // reset index
	shuffledWordList = generateShuffledWordList();
}

// ##############################################################################################################
// ###############################################    ...    ####################################################
// ##############################################################################################################

FUniqueNetId NetId::to_unreal_id() const
{
	FUniqueNetId id;
	id.Uid           = Uid;
	id.NpId          = NpId;
	id.EpicAccountId = FString::create(EpicAccountId);
	id.Platform      = Platform;
	id.SplitscreenID = SplitscreenID;

	return id;
}

// ChatData
ChatData::ChatData(const FGFxChatMessage& msg)
{
	// parse quickchat message if necessary
	std::string chat_text = msg.Message.ToString();
	if (msg.bPreset)
	{
		auto it = g_quickchatIdsToText.find(chat_text);
		if (it != g_quickchatIdsToText.end())
			chat_text = it->second;
	}

	PlayerName  = msg.PlayerName.ToString();
	Message     = chat_text;
	TimeStamp   = msg.TimeStamp.ToString();
	Team        = msg.Team;
	ChatChannel = static_cast<EChatChannel>(msg.ChatChannel);
	SenderId    = msg.SenderId;
	IsUser      = msg.bLocalPlayer;
	IsQuickchat = msg.bPreset;
	IdString    = UidWrapper::unreal_id_to_uid_str(msg.SenderId);
}

ChatData::ChatData(const UGFxData_Chat_TA_execOnChatMessage_Params& params)
{
	// Determine if message is a quickchat. If so, update chatText and IsQuickchat flag
	std::string chatText = params.Message.ToString();
	auto        it       = g_quickchatIdsToText.find(chatText);
	if (it != g_quickchatIdsToText.end())
	{
		chatText    = it->second;
		IsQuickchat = true;
	}

	PlayerName  = params.PlayerName.ToString();
	Message     = chatText;
	TimeStamp   = params.TimeStamp.ToString();
	Team        = params.Team;
	ChatChannel = static_cast<EChatChannel>(params.ChatChannel);
	SenderId    = params.SenderId;
	IsUser      = params.bLocalPlayer;
	IdString    = UidWrapper::unreal_id_to_uid_str(params.SenderId);
}

bool ChatData::is_valid_last_chat(const LastChatPreferences& prefs, uint8_t user_team) const
{
	// filter in order of precedence...
	if (IsUser)
	{
		if (!prefs.UserChats)
			return false;
		if (IsQuickchat && !prefs.Quickchats)
			return false;
	}
	else
	{
		if (IsQuickchat && !prefs.Quickchats)
			return false;
		if (ChatChannel == EChatChannel::EChatChannel_Party && !prefs.PartyChats)
			return false;
		if (ChatChannel == EChatChannel::EChatChannel_Team && !prefs.TeamChats)
			return false;

		// NOTE:
		// If a party chat was sent (aka ChatChannel == 2), Team will show up as 255 (uint8_t version of -1) even if the player's actual
		// team number is 0 or 1
		// ... which is one of the reasons why party chats are checked before teammate chats
		if (Team == user_team && !prefs.TeammateChats)
			return false;
	}

	return true;
}

// ChatMsgData
ChatMsgData::ChatMsgData(const FChatMessage& chat)
{
	uncensoredMsg = chat.Message.ToString();
	uid           = generateUid(chat);
}

std::string ChatMsgData::generateUid(const FChatMessage& data)
{
	return std::format("{}|{}|{}|{}", data.PlayerName.ToString(), data.TimeStamp.ToString(), data.Message.size(), data.ChatChannel);
}

std::string ChatMsgData::generateUid(UGFxData_Chat_TA_execOnChatMessage_Params* data)
{
	if (!data)
		return std::string();

	return std::format("{}|{}|{}|{}", data->PlayerName.ToString(), data->TimeStamp.ToString(), data->Message.size(), data->ChatChannel);
}

// RankData
void RankData::assign(const FPlayerSkillRating& skill)
{
	skill_data = skill;

	// update tier string
	auto it = skill_tier_to_label.find(skill.Tier);
	if (it != skill_tier_to_label.end())
	{
		tier = it->second;
	}

	// update div string
	div = std::to_string(skill.Division + 1);
}

std::string RankData::get_rank_str() const
{
	if (skill_data.Tier == 0 || skill_data.MatchesPlayed == 0)
	{
		return "--";
	}
	return tier + "..div" + div;
}

// ChatterRanks
void ChatterRanks::assign(const ChatData& chat, UOnlineGameSkill_X* game_skill)
{
	if (!game_skill)
	{
		LOG("Unable to get chatter ranks... UOnlineGameSkill_X* is null");
		return;
	}

	playerName = chat.PlayerName;

	ones   = get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::RankedSoloDuel), game_skill);
	twos   = get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::RankedTeamDoubles), game_skill);
	threes = get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::RankedStandard), game_skill);
	casual = get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::Casual), game_skill);
}

RankData ChatterRanks::get_rank(ERankPlaylists playlist)
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
		return RankData();
	}
}

std::string ChatterRanks::get_all_ranks_str() const
{
	// to make return line readable
	std::string ones_str   = ones.get_rank_str();
	std::string twos_str   = twos.get_rank_str();
	std::string threes_str = threes.get_rank_str();

	return playerName + ": [1s] " + ones_str + " [2s] " + twos_str + " [3s] " + threes_str;
}

std::string ChatterRanks::get_playlist_rank_str(ERankPlaylists playlist)
{
	std::string rank_str;

	RankData specificRank = get_rank(playlist);

	rank_str = playerName + " [" + ChatterRanks::get_playlist_str(playlist) + "] ";

	if (playlist == ERankPlaylists::Casual)
	{
		rank_str += std::to_string(std::lround(specificRank.skill_data.MMR)) + " mmr (" +
		            std::to_string(specificRank.skill_data.MatchesPlayed) + " matches)";
	}
	else if (specificRank.skill_data.Tier != 0)
	{
		rank_str += specificRank.tier + "..div" + specificRank.div;

		if (specificRank.skill_data.MatchesPlayed != 0)
		{
			rank_str += " (" + std::to_string(specificRank.skill_data.MatchesPlayed) + " matches)";
		}
		else if (specificRank.skill_data.PlacementMatchesPlayed != 0)
		{
			rank_str += " (" + std::to_string(specificRank.skill_data.PlacementMatchesPlayed) + " placement matches)";
		}
		else
		{
			rank_str += " (prev season)";
		}
	}
	else
	{
		rank_str += "** doesnt play ** (" + std::to_string(specificRank.skill_data.PlacementMatchesPlayed) + " placement matches)";
	}

	return rank_str;
}

FPlayerSkillRating ChatterRanks::get_skill_rating(const FUniqueNetId& id, int playlist_id, UOnlineGameSkill_X* game_skill)
{
	if (!game_skill)
		return FPlayerSkillRating{};

	FPlayerSkillRating rank_data = game_skill->GetPlayerRating(id, playlist_id);
	rank_data.MMR                = calculate_skill_rating(rank_data.Mu); // <--- change to be the value that everyone refers to as "MMR"

	return rank_data;
}

// NOTE: This formula is what's used for RL leaderboards and is what people refer to as "MMR"
// ... but it's not what is used internally to determine matchmaking. Apparently that would be the Microsoft TrueSkill formula: Mu - (3 *
// Sigma)
float ChatterRanks::calculate_skill_rating(float mu) { return (mu * 20) + 100; }

std::string ChatterRanks::get_playlist_str(ERankPlaylists playlist)
{
	switch (playlist)
	{
	case ERankPlaylists::Ones:
		return "1s";
	case ERankPlaylists::Twos:
		return "2s";
	case ERankPlaylists::Threes:
		return "3s";
	case ERankPlaylists::Casual:
		return "casual";
	default:
		return "";
	}
}