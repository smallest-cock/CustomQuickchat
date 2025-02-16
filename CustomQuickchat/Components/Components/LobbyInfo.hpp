#pragma once
#include "Instances.hpp"
#include "Events.hpp"
#include "Cvars.hpp"
#include "Structs.hpp"


class LobbyInfoComponent
{
public:
	LobbyInfoComponent();
	~LobbyInfoComponent();

	void Initialize(std::shared_ptr<GameWrapper> gw);

public:
	static constexpr std::string_view logging_prefix = "[LobbyInfoComponent] ";

	template <typename... Args>
	static void LOG(std::string_view format_str, Args&&... args)
	{
		::LOG(std::string(logging_prefix) + std::string(format_str), std::forward<Args>(args)...);
	}

private:
	std::shared_ptr<GameWrapper> gameWrapper;
	UGFxData_Chat_TA* chat_data = nullptr;

	std::vector<ChatData> match_chats;
	std::unordered_map<std::string, ChatterRanks> match_ranks;	// bm ID string --> ChatterRanks struct

	void event_GFxData_Chat_TA_OnShellSet(ActorWrapper caller, void* params, std::string event);
	void event_we_need_to_yeet_stored_data(std::string event);
	void event_GFxData_Chat_TA_AddChatMessage(ActorWrapper caller, void* params, std::string event);


public:
	ChatData get_last_chat_data();
	static std::string uid_str_from_net_id(const FUniqueNetId& id);
	static std::string get_platform_str(uint8_t platform);
	static void log_chat_data(const ChatData& chat);
	void clear_stored_chats();


	inline size_t get_match_chats_size() { return match_chats.size(); }
	inline size_t get_match_ranks_size() { return match_ranks.size(); }

public:
	ChatterRanks get_last_chatter_ranks();
	void clear_stored_ranks();
	static inline float get_skill_rating(float mu);
};



extern class LobbyInfoComponent LobbyInfo;
