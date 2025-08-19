#pragma once
#include "Structs.hpp"

class LobbyInfoComponent
{
public:
	LobbyInfoComponent() {}
	~LobbyInfoComponent() {}

	void Initialize(std::shared_ptr<GameWrapper> gw);
	void initHooks();

public:
	static constexpr std::string_view logging_prefix = "[LobbyInfoComponent] ";

	template <typename... Args> static void LOG(std::string_view format_str, Args&&... args)
	{
		::LOG(std::string(logging_prefix) + std::string(format_str), std::forward<Args>(args)...);
	}

	template <typename... Args> static void LOGERROR(std::string_view format_str, Args&&... args)
	{
		::LOG(std::string(logging_prefix) + "ERROR: " + std::string(format_str), std::forward<Args>(args)...);
	}

private:
	std::shared_ptr<GameWrapper> gameWrapper;
	UGFxData_Chat_TA*            m_gfxChatData = nullptr;

	std::vector<ChatData>                         m_matchChats;
	std::unordered_map<std::string, ChatterRanks> m_matchRanks; // bm ID string --> ChatterRanks struct

public:
	ChatData           getLastChatData();
	static std::string uidStrFromNetId(const FUniqueNetId& id);
	static std::string getPlatformStr(uint8_t platform);
	static void        logChatData(const ChatData& chat);
	void               clearStoredChats();

	inline size_t getMatchChatsSize() { return m_matchChats.size(); }
	inline size_t getMatchRanksSize() { return m_matchRanks.size(); }

public:
	ChatterRanks        get_last_chatter_ranks();
	void                clear_stored_ranks();
	static inline float get_skill_rating(float mu);

public:
	void clearCachedData();
};

extern class LobbyInfoComponent LobbyInfo;
