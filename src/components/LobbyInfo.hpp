#pragma once
#include "Structs.hpp"
#include "Component.hpp"
#include <optional>

class LobbyInfoComponent : Component<LobbyInfoComponent>
{
public:
	LobbyInfoComponent() {}
	~LobbyInfoComponent() {}

	static constexpr std::string_view componentName = "LobbyInfo";
	void                              init(const std::shared_ptr<GameWrapper>& gw);

private:
	void initFilepaths();
	void initHooks();
	void initCvars();

private:
	// cvar values
	std::shared_ptr<bool> m_userChatsInLastChat     = std::make_shared<bool>(false);
	std::shared_ptr<bool> m_teammateChatsInLastChat = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_quickchatsInLastChat    = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_partyChatsInLastChat    = std::make_shared<bool>(true);
	std::shared_ptr<bool> m_teamChatsInLastChat     = std::make_shared<bool>(true);

private:
	// Lobby Info filepaths
	fs::path m_lobbyInfoFolder;
	fs::path m_chatsJsonPath;
	fs::path m_ranksJsonPath;

	UGFxData_Chat_TA* m_gfxChatData = nullptr;

	std::vector<ChatData>                         m_matchChats;
	std::unordered_map<std::string, ChatterRanks> m_matchRanks; // bm ID string --> ChatterRanks struct

public:
	// TODO: move these to LobyInfo class
	std::string getLastChat();
	std::string getLastChatterRankStr(EKeyword keyword);

	std::optional<ChatData> getLastChatData();
	static std::string      uidStrFromNetId(const FUniqueNetId& id);
	static std::string      getPlatformStr(uint8_t platform);
	static void             logChatData(const ChatData& chat);
	void                    clearStoredChats();

	inline size_t getMatchChatsSize() { return m_matchChats.size(); }
	inline size_t getMatchRanksSize() { return m_matchRanks.size(); }

public:
	std::optional<ChatterRanks> getLastChatterRanks();
	void                        clearStoredRanks();
	static float                getSkillRating(float mu);

public:
	void clearCachedData();

public:
	void display_settings();
};

extern class LobbyInfoComponent LobbyInfo;
