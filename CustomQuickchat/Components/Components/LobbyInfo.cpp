#include "pch.h"
#include "LobbyInfo.hpp"


LobbyInfoComponent::LobbyInfoComponent() { }
LobbyInfoComponent::~LobbyInfoComponent() { }


void LobbyInfoComponent::Initialize(std::shared_ptr<GameWrapper> gw)
{
	gameWrapper = gw;

	// hook events
	gameWrapper->HookEventPost(Events::LoadingScreenStart, std::bind(&LobbyInfoComponent::event_we_need_to_yeet_stored_data, this, std::placeholders::_1));
	LOG("Hooked event: \"{}\"", Events::LoadingScreenStart);

	gameWrapper->HookEventPost(Events::GFxHUD_TA_Destroyed, std::bind(&LobbyInfoComponent::event_we_need_to_yeet_stored_data, this, std::placeholders::_1));
	LOG("Hooked event: \"{}\"", Events::GFxHUD_TA_Destroyed);

	

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::GFxData_Chat_TA_OnShellSet,
		std::bind(&LobbyInfoComponent::event_GFxData_Chat_TA_OnShellSet, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	LOG("Hooked event: \"{}\"", Events::GFxData_Chat_TA_OnShellSet);

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::GFxData_Chat_TA_AddChatMessage,
		std::bind(&LobbyInfoComponent::event_GFxData_Chat_TA_AddChatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	LOG("Hooked event: \"{}\"", Events::GFxData_Chat_TA_AddChatMessage);

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::GFxData_Chat_TA_AddPresetMessage,
		std::bind(&LobbyInfoComponent::event_GFxData_Chat_TA_AddChatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	LOG("Hooked event: \"{}\"", Events::GFxData_Chat_TA_AddPresetMessage);

	gameWrapper->HookEvent(Events::GFxData_Chat_TA_OnRemoved, [this](std::string event)
	{
		LOG("UGFxData_Chat_TA::OnRemoved fired");
		chat_data = nullptr;
	});
	LOG("Hooked event: \"{}\"", Events::GFxData_Chat_TA_OnRemoved);
}


// ===================================== hook callbacks =====================================

void LobbyInfoComponent::event_GFxData_Chat_TA_OnShellSet(ActorWrapper caller, void* params, std::string event)
{
	LOG("UGFxData_Chat_TA::OnShellSet fired");

	auto gfx_chat = reinterpret_cast<UGFxData_Chat_TA*>(caller.memory_address);
	if (gfx_chat)
	{
		chat_data = gfx_chat;
		LOG("Updated stored UGFxData_Chat_TA*");
	}
}


void LobbyInfoComponent::event_we_need_to_yeet_stored_data(std::string event)
{
	clear_stored_chats();
	clear_stored_ranks();
}

void LobbyInfoComponent::event_GFxData_Chat_TA_AddChatMessage(ActorWrapper caller, void* params, std::string event)
{
	LOG("Function fired: \"{}\"", event);

	chat_data = reinterpret_cast<UGFxData_Chat_TA*>(caller.memory_address);		// update stored instance

	auto chat_params = reinterpret_cast<UGFxData_Chat_TA_execAddChatMessage_Params*>(params);
	if (!chat_params)
	{ 
		LOG("ERROR: Function params are null for: \"{}\"", event);
		return;
	}

	if (UidWrapper::unreal_id_to_uid_str(chat_params->NewMessage.SenderId) == "Unknown|0|0")
	{
		LOG("Chat SenderId is empty. We aint finna save it");
		return;
	}

	match_chats.emplace_back(chat_params->NewMessage);
	LOG("Added chat to stored chats. New size: {}", match_chats.size());
}

// ==========================================================================================


void LobbyInfoComponent::clear_stored_chats()
{
	match_chats.clear();
	LOG("Cleared stored match chats");
}


void LobbyInfoComponent::clear_stored_ranks()
{
	match_ranks.clear();
	LOG("Cleared stored player ranks");
}


void LobbyInfoComponent::log_chat_data(const ChatData& chat)
{
	LOG("----------------------------------------------------");
	LOG("SenderId: {}",			chat.IdString);
	LOG("----------------------------------------------------");
	LOG("Team: {}",				chat.Team);
	LOG("PlayerName: {}",		chat.PlayerName);
	LOG("Message: {}",			chat.Message);
	LOG("ChatChannel: {}",		static_cast<uint8_t>(chat.ChatChannel));
	LOG("IsUser: {}",			chat.IsUser);
	//LOG("bTransient: {}",		chat.bTransient);
	LOG("IsQuickchat: {}",		chat.IsQuickchat);
	//LOG("MessageType: {}",	chat.MessageType);
	LOG("TimeStamp: {}",		chat.TimeStamp);
}


ChatData LobbyInfoComponent::get_last_chat_data()
{
/*
	NOTE:
		It would've been easier to just read from the game's built-in chat history data, but:
			- Apparently the max capacity for UGFxData_Chat_TA::Messages is 10 (aka only 10 most recent chats are stored at a time)
			- And the max capacity for StoredChatData_TA::Messsages is 50... which is better, but still not ideal :(
	
		So ig we need to manually store chat data on every chat...
*/

	if (match_chats.empty())
	{
		return FGFxChatMessage{};
	}

	auto user_chats_in_last_chat_cvar =		_globalCvarManager->getCvar(Cvars::user_chats_in_last_chat.name);
	auto teammate_chats_in_last_chat_cvar = _globalCvarManager->getCvar(Cvars::teammate_chats_in_last_chat.name);
	auto quickchats_in_last_chat_cvar =		_globalCvarManager->getCvar(Cvars::quickchats_in_last_chat.name);
	auto party_chats_in_last_chat_cvar =	_globalCvarManager->getCvar(Cvars::party_chats_in_last_chat.name);
	auto team_chats_in_last_chat_cvar =		_globalCvarManager->getCvar(Cvars::team_chats_in_last_chat.name);

	if (!user_chats_in_last_chat_cvar)
		return FGFxChatMessage{};
	
	const LastChatPreferences chat_preferences =
	{
		quickchats_in_last_chat_cvar.getBoolValue(),
		user_chats_in_last_chat_cvar.getBoolValue(),
		team_chats_in_last_chat_cvar.getBoolValue(),
		party_chats_in_last_chat_cvar.getBoolValue(),
		teammate_chats_in_last_chat_cvar.getBoolValue()
	};

	const uint8_t hopefully_correct_user_team = gameWrapper->GetPlayerController().GetTeamNum2();
	
	LOG("Stored chat messages size: {}", match_chats.size());
	for (int i = match_chats.size() - 1; i >= 0; --i)
	{
		const auto& chat = match_chats.at(i);

		if (chat.Message.empty())
		{
			LOG("Skipped chat at index {} because Message string was empty", i);
			continue;
		}

		if (chat.is_valid_last_chat(chat_preferences, hopefully_correct_user_team))
		{
			LOG("Found a suitable last chat: {}", chat.Message);
			return chat;
		}
	}

	LOG("We didn't find a suitable last chat. Returning a blank FGFxChatMessage...");
	return FGFxChatMessage{};
}


ChatterRanks LobbyInfoComponent::get_last_chatter_ranks()
{
	// TODO: make this UOnlineGameSkill_X* a member and update current instance using hooks (like how we do with UGFxData_Chat_TA*)
	auto skill = Instances.GetInstanceOf<UOnlineGameSkill_X>();
	if (!skill)
	{
		LOG("[ERROR] UOnlineGameSkill_X* is null");
		return ChatterRanks{};
	}

	auto chat_data = get_last_chat_data();

	auto it = match_ranks.find(chat_data.IdString);
	if (it == match_ranks.end())
	{
		ChatterRanks ranks{ chat_data, skill };
		match_ranks[chat_data.IdString] = ranks;
		LOG("Stored ranks for {}", chat_data.PlayerName);
		LOG("Stored ranks size: {}", match_ranks.size());
		return ranks;
	}
	else
	{
		return it->second;
	}
}


// NOTE: This formula is what's used for RL leaderboards and is what people refer to as "MMR"
// ... but it's not what's used internally to determine matchmaking. Apparently that would be the Microsoft TrueSkill formula: Mu - (3 * Sigma) 
float LobbyInfoComponent::get_skill_rating(float mu)
{
	return (mu * 20) + 100;
}


// Returns the id string in the following format: platform|accountId|splitScreenId
std::string LobbyInfoComponent::uid_str_from_net_id(const FUniqueNetId& id)
{
	std::string account_id = id.EpicAccountId.empty() ? std::to_string(id.Uid) : id.EpicAccountId.ToString();

	return get_platform_str(id.Platform) + "|" + account_id + "|" + std::to_string(id.SplitscreenID);
}


std::string LobbyInfoComponent::get_platform_str(uint8_t platform)
{
	switch (static_cast<EOnlinePlatform>(platform))
	{
	case EOnlinePlatform::OnlinePlatform_Steam:
		return "Steam";
	case EOnlinePlatform::OnlinePlatform_PS4:
	case EOnlinePlatform::OnlinePlatform_PS3:
		return "PlayStation";
	case EOnlinePlatform::OnlinePlatform_Dingo:
		return "Xbox";
	case EOnlinePlatform::OnlinePlatform_OldNNX:
	case EOnlinePlatform::OnlinePlatform_NNX:
		return "Nintendo";
	case EOnlinePlatform::OnlinePlatform_PsyNet:
	case EOnlinePlatform::OnlinePlatform_Epic:
		return "Epic";
	case EOnlinePlatform::OnlinePlatform_Unknown:
	case EOnlinePlatform::OnlinePlatform_QQ_DEPRECATED:
	case EOnlinePlatform::OnlinePlatform_WeGame_DEPRECATED:
	case EOnlinePlatform::OnlinePlatform_Deleted:
	case EOnlinePlatform::OnlinePlatform_END:
	default:
		return "Unknown";
	}
}


// ======================================== ChatData ========================================

bool ChatData::is_valid_last_chat(const LastChatPreferences& prefs, uint8_t user_team) const
{
	// filter in order of precedence...
	if (IsUser)
	{
		if(!prefs.UserChats)
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
		// If a party chat was sent (aka ChatChannel == 2), Team will show up as 255 (uint8_t version of -1) even if the player's actual team number is 0 or 1
		// ... which is one of the reasons why party chats are checked before teammate chats
		if (Team == user_team && !prefs.TeammateChats)
			return false;
	}

	return true;
}

// ==========================================================================================


class LobbyInfoComponent LobbyInfo {};
