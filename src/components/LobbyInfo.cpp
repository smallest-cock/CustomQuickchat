#include "pch.h"
#include "ModUtils/gui/GuiTools.hpp"
#include "LobbyInfo.hpp"
#include "bakkesmod/wrappers/Engine/ActorWrapper.h"
#include "Events.hpp"
#include "Cvars.hpp"
#include "Macros.hpp"
#include "Instances.hpp"
#include "HookManager.hpp"
#include <optional>

void LobbyInfoComponent::init(const std::shared_ptr<GameWrapper>& gw)
{
	gameWrapper = gw;

	initFilepaths();
	initCvars();
	initHooks();
}

void LobbyInfoComponent::initFilepaths()
{
	fs::path bmDataFolderFilePath = gameWrapper->GetDataFolder();
	fs::path pluginFolder         = bmDataFolderFilePath / "CustomQuickchat";

	m_lobbyInfoFolder = bmDataFolderFilePath / "Lobby Info";
	m_chatsJsonPath   = m_lobbyInfoFolder / "Chats.json";
	m_ranksJsonPath   = m_lobbyInfoFolder / "Ranks.json";
}

void LobbyInfoComponent::initCvars()
{
	registerCvar_bool(Cvars::userChatsInLastChat, false).bindTo(m_userChatsInLastChat);
	registerCvar_bool(Cvars::teammateChatsInLastChat, true).bindTo(m_teammateChatsInLastChat);
	registerCvar_bool(Cvars::quickchatsInLastChat, true).bindTo(m_quickchatsInLastChat);
	registerCvar_bool(Cvars::partyChatsInLastChat, true).bindTo(m_partyChatsInLastChat);
	registerCvar_bool(Cvars::teamChatsInLastChat, true).bindTo(m_teamChatsInLastChat);
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

std::string LobbyInfoComponent::getLastChat()
{
	auto chat = getLastChatData();
	if (!chat)
	{
		LOGERROR("Unable to get last chat data");
		return "";
	}
	if (chat->Message.empty())
	{
		LOG("[ERROR] Message is empty string from last chat data");
		return "";
	}

	return chat->Message;
}

std::string LobbyInfoComponent::getLastChatterRankStr(EKeyword keyword)
{
	auto chatterRanks = getLastChatterRanks();
	if (!chatterRanks)
	{
		LOGERROR("Unable to get last chatter's ranks");
		return "";
	}
	if (chatterRanks->playerName.empty())
	{
		LOGERROR("ChatterRanks::playerName is empty string");
		return std::string();
	}

	switch (keyword)
	{
	case EKeyword::BlastAll:
		return chatterRanks->get_all_ranks_str();
	case EKeyword::BlastCasual:
		return chatterRanks->get_playlist_rank_str(ERankPlaylists::Casual);
	case EKeyword::Blast1v1:
		return chatterRanks->get_playlist_rank_str(ERankPlaylists::Ones);
	case EKeyword::Blast2v2:
		return chatterRanks->get_playlist_rank_str(ERankPlaylists::Twos);
	case EKeyword::Blast3v3:
		return chatterRanks->get_playlist_rank_str(ERankPlaylists::Threes);
	default:
		return "";
	}
}

void LobbyInfoComponent::clearCachedData()
{
	clearStoredChats();
	clearStoredRanks();
}

void LobbyInfoComponent::clearStoredChats()
{
	m_matchChats.clear();
	LOG("Cleared stored match chats");
}

void LobbyInfoComponent::clearStoredRanks()
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

std::optional<ChatData> LobbyInfoComponent::getLastChatData()
{
	/*
	    NOTE:
	        It would've been easier to just read from the game's built-in chat history data, but:
	            - Apparently the max capacity for UGFxData_Chat_TA::Messages is 10 (aka only 10 most recent chats are stored at a time)
	            - And the max capacity for StoredChatData_TA::Messsages is 50... which is better, but still not ideal :(

	        So ig we need to manually store chat data on every chat...
	*/

	if (m_matchChats.empty())
		return std::nullopt;

	const LastChatPreferences chatPreferences = {
	    *m_quickchatsInLastChat, *m_userChatsInLastChat, *m_teamChatsInLastChat, *m_partyChatsInLastChat, *m_teammateChatsInLastChat};

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
	return std::nullopt;
}

std::optional<ChatterRanks> LobbyInfoComponent::getLastChatterRanks()
{
	// TODO: make this UOnlineGameSkill_X* a member and update current instance using hooks (like how we do with UGFxData_Chat_TA*)
	auto* skill = Instances.GetInstanceOf<UOnlineGameSkill_X>();
	if (!skill)
	{
		LOG("[ERROR] UOnlineGameSkill_X* is null");
		return std::nullopt;
	}

	auto chatData = getLastChatData();
	if (!chatData)
	{
		LOGERROR("Unable to get last chat data");
		return std::nullopt;
	}

	auto it = m_matchRanks.find(chatData->IdString);
	if (it == m_matchRanks.end())
	{
		ChatterRanks ranks{*chatData, skill};
		m_matchRanks[chatData->IdString] = ranks;
		LOG("Stored ranks for {}", chatData->PlayerName);
		LOG("Stored ranks size: {}", m_matchRanks.size());
		return ranks;
	}
	else
		return it->second;
}

// NOTE: This formula is what's used for RL leaderboards and is what people refer to as "MMR"
// ... but it's not what's used internally to determine matchmaking. Apparently that would be the Microsoft TrueSkill formula: Mu - (3 *
// Sigma)
float LobbyInfoComponent::getSkillRating(float mu) { return (mu * 20) + 100; }

// Returns an ID string in the following format: platform|accountId|splitScreenId
std::string LobbyInfoComponent::uidStrFromNetId(const FUniqueNetId& id)
{
	std::string acctId = id.EpicAccountId.empty() ? std::to_string(id.Uid) : id.EpicAccountId.ToString();
	return std::format("{}|{}|{}", getPlatformStr(id.Platform), acctId, id.SplitscreenID);
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

// ##############################################################################################################
// ##########################################   DISPLAY FUNCTIONS    ############################################
// ##############################################################################################################

void LobbyInfoComponent::display_settings()
{
	auto userChatsInLastChat_cvar     = getCvar(Cvars::userChatsInLastChat);
	auto quickchatsInLastChat_cvar    = getCvar(Cvars::quickchatsInLastChat);
	auto teammateChatsInLastChat_cvar = getCvar(Cvars::teammateChatsInLastChat);
	auto partyChatsInLastChat_cvar    = getCvar(Cvars::partyChatsInLastChat);
	auto teamChatsInLastChat_cvar     = getCvar(Cvars::teamChatsInLastChat);
	if (!userChatsInLastChat_cvar)
		return;

	bool userChatsInLastChat     = userChatsInLastChat_cvar.getBoolValue();
	bool quickchatsInLastChat    = quickchatsInLastChat_cvar.getBoolValue();
	bool teammateChatsInLastChat = teammateChatsInLastChat_cvar.getBoolValue();
	bool partyChatsInLastChat    = partyChatsInLastChat_cvar.getBoolValue();
	bool teamChatsInLastChat     = teamChatsInLastChat_cvar.getBoolValue();

	GUI::Spacing(2);

	GUI::ClickableLink("Keywords guide",
	    "https://github.com/smallest-cock/CustomQuickchat/blob/main/docs/Settings.md#special-effects",
	    GUI::Colors::BlueGreen);

	GUI::Spacing(2);

	ImGui::TextColored(GUI::Colors::Yellow, "Chats to be included when searching for the last chat:");
	GUI::ToolTip("Searching for last chat sent happens for [[lastChat]] and [[blast ...]]\n\nMore info can be found in the "
	             "keywords guide above");

	GUI::Spacing(2);

	if (ImGui::Checkbox("User chats", &userChatsInLastChat))
		userChatsInLastChat_cvar.setValue(userChatsInLastChat);

	if (ImGui::Checkbox("Quickchats", &quickchatsInLastChat))
		quickchatsInLastChat_cvar.setValue(quickchatsInLastChat);

	if (ImGui::Checkbox("Teammate chats", &teammateChatsInLastChat))
		teammateChatsInLastChat_cvar.setValue(teammateChatsInLastChat);

	if (ImGui::Checkbox("Party chats", &partyChatsInLastChat))
		partyChatsInLastChat_cvar.setValue(partyChatsInLastChat);

	if (ImGui::Checkbox("Team chats", &teamChatsInLastChat))
		teamChatsInLastChat_cvar.setValue(teamChatsInLastChat);

	GUI::Spacing(4);

	ImGui::Text("Cached chats: %zu", getMatchChatsSize());

	GUI::SameLineSpacing_absolute(150);

	if (ImGui::Button("Clear##chatLog"))
		GAME_THREAD_EXECUTE({ clearStoredChats(); });

	ImGui::Text("Cached player ranks: %zu", getMatchRanksSize());

	GUI::SameLineSpacing_absolute(150);

	if (ImGui::Button("Clear##playerRanks"))
		GAME_THREAD_EXECUTE({ clearStoredRanks(); });
}

class LobbyInfoComponent LobbyInfo{};
