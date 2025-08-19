#include "pch.h"
#include "LobbyInfo.hpp"
#include "bakkesmod/wrappers/Engine/ActorWrapper.h"
#include "Events.hpp"
#include "Cvars.hpp"
#include "Instances.hpp"
#include "HookManager.hpp"

void LobbyInfoComponent::Initialize(std::shared_ptr<GameWrapper> gw)
{
	gameWrapper = gw;

	initHooks();
}

void LobbyInfoComponent::initHooks()
{
	Hooks.hookEvent(Events::GFxHUD_TA_Destroyed, HookType::Post, [this](std::string) { clearCachedData(); });

	Hooks.hookEvent(Events::GFxData_Chat_TA_OnShellSet,
	    HookType::Post,
	    [this](ActorWrapper caller, ...)
	    {
		    auto* gfxChat = reinterpret_cast<UGFxData_Chat_TA*>(caller.memory_address);
		    if (!gfxChat)
			    return;

		    m_gfxChatData = gfxChat;
		    LOG("Updated stored UGFxData_Chat_TA*");
	    });

	auto handleGfxMsgAdded = [this](ActorWrapper Caller, void* Params, std::string event)
	{
		LOG("Function fired: \"{}\"", event);

		m_gfxChatData = reinterpret_cast<UGFxData_Chat_TA*>(Caller.memory_address);

		auto* params = reinterpret_cast<UGFxData_Chat_TA_execAddChatMessage_Params*>(Params);
		if (!params)
		{
			LOG("ERROR: Function params are null for: \"{}\"", event);
			return;
		}

		if (UidWrapper::unreal_id_to_uid_str(params->NewMessage.SenderId) == "Unknown|0|0")
		{
			LOG("Chat SenderId is empty. We aint finna save it");
			return;
		}

		m_matchChats.emplace_back(params->NewMessage);
		LOG("Added chat to stored chats. New size: {}", m_matchChats.size());
	};
	Hooks.hookEvent(Events::GFxData_Chat_TA_AddChatMessage, HookType::Post, handleGfxMsgAdded);
	Hooks.hookEvent(Events::GFxData_Chat_TA_AddPresetMessage, HookType::Post, handleGfxMsgAdded);

	Hooks.hookEvent(Events::GFxData_Chat_TA_OnRemoved,
	    HookType::Pre,
	    [this](std::string event)
	    {
		    LOG("UGFxData_Chat_TA::OnRemoved fired");
		    m_gfxChatData = nullptr;
	    });
}

void LobbyInfoComponent::clearCachedData()
{
	clearStoredChats();
	clear_stored_ranks();
}

void LobbyInfoComponent::clearStoredChats()
{
	m_matchChats.clear();
	LOG("Cleared stored match chats");
}

void LobbyInfoComponent::clear_stored_ranks()
{
	m_matchRanks.clear();
	LOG("Cleared stored player ranks");
}

void LobbyInfoComponent::logChatData(const ChatData& chat)
{
	LOG("----------------------------------------------------");
	LOG("SenderId: {}", chat.IdString);
	LOG("----------------------------------------------------");
	LOG("Team: {}", chat.Team);
	LOG("PlayerName: {}", chat.PlayerName);
	LOG("Message: {}", chat.Message);
	LOG("ChatChannel: {}", static_cast<uint8_t>(chat.ChatChannel));
	LOG("IsUser: {}", chat.IsUser);
	// LOG("bTransient: {}",     chat.bTransient);
	LOG("IsQuickchat: {}", chat.IsQuickchat);
	// LOG("MessageType: {}",    chat.MessageType);
	LOG("TimeStamp: {}", chat.TimeStamp);
}

ChatData LobbyInfoComponent::getLastChatData()
{
	/*
	    NOTE:
	        It would've been easier to just read from the game's built-in chat history data, but:
	            - Apparently the max capacity for UGFxData_Chat_TA::Messages is 10 (aka only 10 most recent chats are stored at a time)
	            - And the max capacity for StoredChatData_TA::Messsages is 50... which is better, but still not ideal :(

	        So ig we need to manually store chat data on every chat...
	*/

	if (m_matchChats.empty())
		return FGFxChatMessage{};

	auto user_chats_in_last_chat_cvar     = _globalCvarManager->getCvar(Cvars::user_chats_in_last_chat.name);
	auto teammate_chats_in_last_chat_cvar = _globalCvarManager->getCvar(Cvars::teammate_chats_in_last_chat.name);
	auto quickchats_in_last_chat_cvar     = _globalCvarManager->getCvar(Cvars::quickchats_in_last_chat.name);
	auto party_chats_in_last_chat_cvar    = _globalCvarManager->getCvar(Cvars::party_chats_in_last_chat.name);
	auto team_chats_in_last_chat_cvar     = _globalCvarManager->getCvar(Cvars::team_chats_in_last_chat.name);

	if (!user_chats_in_last_chat_cvar)
		return FGFxChatMessage{};

	const LastChatPreferences chatPreferences = {quickchats_in_last_chat_cvar.getBoolValue(),
	    user_chats_in_last_chat_cvar.getBoolValue(),
	    team_chats_in_last_chat_cvar.getBoolValue(),
	    party_chats_in_last_chat_cvar.getBoolValue(),
	    teammate_chats_in_last_chat_cvar.getBoolValue()};

	const uint8_t hopefullyCorrectUserTeam = gameWrapper->GetPlayerController().GetTeamNum2();

	LOG("Stored chat messages size: {}", m_matchChats.size());
	for (int i = m_matchChats.size() - 1; i >= 0; --i)
	{
		const auto& chat = m_matchChats.at(i);

		if (chat.Message.empty())
		{
			LOG("Skipped chat at index {} because Message string was empty", i);
			continue;
		}

		if (chat.is_valid_last_chat(chatPreferences, hopefullyCorrectUserTeam))
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

	auto chat_data = getLastChatData();

	auto it = m_matchRanks.find(chat_data.IdString);
	if (it == m_matchRanks.end())
	{
		ChatterRanks ranks{chat_data, skill};
		m_matchRanks[chat_data.IdString] = ranks;
		LOG("Stored ranks for {}", chat_data.PlayerName);
		LOG("Stored ranks size: {}", m_matchRanks.size());
		return ranks;
	}
	else
		return it->second;
}

// NOTE: This formula is what's used for RL leaderboards and is what people refer to as "MMR"
// ... but it's not what's used internally to determine matchmaking. Apparently that would be the Microsoft TrueSkill formula: Mu - (3 *
// Sigma)
float LobbyInfoComponent::get_skill_rating(float mu) { return (mu * 20) + 100; }

// Returns the id string in the following format: platform|accountId|splitScreenId
std::string LobbyInfoComponent::uidStrFromNetId(const FUniqueNetId& id)
{
	std::string account_id = id.EpicAccountId.empty() ? std::to_string(id.Uid) : id.EpicAccountId.ToString();

	return getPlatformStr(id.Platform) + "|" + account_id + "|" + std::to_string(id.SplitscreenID);
}

std::string LobbyInfoComponent::getPlatformStr(uint8_t platform)
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

class LobbyInfoComponent LobbyInfo{};
