#pragma once
#include "RLSDK/RLSDK_w_pch_includes/SDK_HEADERS/TAGame_parameters.hpp"
#include <string>
#include <unordered_map>

// ==================================== for imgui ====================================

const std::vector<std::string> g_possibleBindingTypes = {"Button combination", "Button sequence"};
const std::vector<std::string> g_possibleChatModes    = {"Lobby", "Team", "Party"};

// ================================== keyword stuff ==================================

enum class EKeyword : uint8_t {
	None                = 0,
	WordVariation       = 1,
	SpeechToText        = 2,
	SpeechToTextUwu     = 3,
	SpeechToTextSarcasm = 4,
	LastChat            = 5,
	LastChatUwu         = 6,
	LastChatSarcasm     = 7,
	BlastAll            = 8,
	BlastCasual         = 9,
	Blast1v1            = 10,
	Blast2v2            = 11,
	Blast3v3            = 12,
	ClosestPlayer       = 13,
	ClosestOpponent     = 14,
	ClosestTeammate     = 15,
	RumbleItem          = 16,
	Forfeit             = 17,
	ExitToMainMenu      = 18
};

enum class ETextEffect : uint8_t {
	None    = 0,
	Uwu     = 1,
	Sarcasm = 2,
};

const std::unordered_map<std::string, EKeyword> g_keywordsMap = {
    {"speechToText", EKeyword::SpeechToText},
    {"speechToText sarcasm", EKeyword::SpeechToTextSarcasm},
    {"speechToText uwu", EKeyword::SpeechToTextUwu},
    {"lastChat", EKeyword::LastChat},
    {"lastChat uwu", EKeyword::LastChatUwu},
    {"lastChat sarcasm", EKeyword::LastChatSarcasm},
    {"blast all", EKeyword::BlastAll},
    {"blast casual", EKeyword::BlastCasual},
    {"blast 1v1", EKeyword::Blast1v1},
    {"blast 2v2", EKeyword::Blast2v2},
    {"blast 3v3", EKeyword::Blast3v3},
    {"closestPlayer", EKeyword::ClosestPlayer},
    {"closestOpponent", EKeyword::ClosestOpponent},
    {"closestTeammate", EKeyword::ClosestTeammate},
    {"rumbleItem", EKeyword::RumbleItem},
    {"forfeit", EKeyword::Forfeit},
    {"menu", EKeyword::ExitToMainMenu},
};

// ============================================================================

enum class EBindingType : uint8_t { Combination = 0, Sequence = 1 };

struct ButtonPress {
	std::string                           buttonName;
	std::chrono::steady_clock::time_point pressedTime;
};

struct BindingKey {
	std::string action;
	std::string pcKey;
	std::string gamepadKey;
};

struct Binding {
	std::string              chat;
	EChatChannel             chatMode                = EChatChannel::EChatChannel_Match;
	EBindingType             bindingType             = EBindingType::Combination;
	EKeyword                 keyWord                 = EKeyword::None;
	ETextEffect              textEffect              = ETextEffect::None;
	bool                     bEnabled                = true;
	bool                     bConflictsWithDefaultQC = false;
	std::vector<std::string> buttons;

	// determine if chat contains any special keyword or text effect (once, at the time of binding creation, rather than every time binding
	// is triggered)
	void                            updateKeywordAndTextEffect(const std::string &regexPatternStr);
	static ETextEffect              getTextEffect(EKeyword keyword);
	static std::vector<std::string> getMatchedSubstrings(const std::string &str, const std::string &regexPatternStr);
};

struct VariationList {
	std::string              listName;
	std::string              unparsedString; // for ImGui
	std::vector<std::string> wordList;
	std::vector<std::string> shuffledWordList;
	int                      nextUsableIndex = 0;
	bool                     shuffleWordList = true;

	std::vector<std::string> generateShuffledWordList() const;
	void                     reshuffleWordList(); // also sets nextUsableIndex to 0
	std::string              getNextVariation();
	void                     updateDataFromUnparsedString();
};

// =================================================================================================
// ============================================= RANKS =============================================
// =================================================================================================

enum class ERankPlaylists : uint8_t { Ones = 0, Twos = 1, Threes = 2, Casual = 3 };

const std::unordered_map<int, std::string> skill_tier_to_label = {{0, "Casual"},
    {1, "B1"},
    {2, "B2"},
    {3, "B3"},
    {4, "S1"},
    {5, "S2"},
    {6, "S3"},
    {7, "G1"},
    {8, "G2"},
    {9, "G3"},
    {10, "P1"},
    {11, "P2"},
    {12, "P3"},
    {13, "D1"},
    {14, "D2"},
    {15, "D3"},
    {16, "C1"},
    {17, "C2"},
    {18, "C3"},
    {19, "GC1"},
    {20, "GC2"},
    {21, "GC3"},
    {22, "SSL"}};

enum class SkillTier {
	Unranked         = 0,
	Bronze1          = 1,
	Bronze2          = 2,
	Bronze3          = 3,
	Silver1          = 4,
	Silver2          = 5,
	Silver3          = 6,
	Gold1            = 7,
	Gold2            = 8,
	Gold3            = 9,
	Platinum1        = 10,
	Platinum2        = 11,
	Platinum3        = 12,
	Diamond1         = 13,
	Diamond2         = 14,
	Diamond3         = 15,
	Champ1           = 16,
	Champ2           = 17,
	Champ3           = 18,
	GrandChamp1      = 19,
	GrandChamp2      = 20,
	GrandChamp3      = 21,
	SupersonicLegend = 22
};

// credit to https://github.com/JulienML/BetterChat/ thx fam
inline const std::map<std::string, std::string> g_quickchatIdsToText = {
    {"Group1Message1", "I got it!"},       // Je l'ai !
    {"Group1Message2", "Need boost!"},     // Besoin de turbo !
    {"Group1Message3", "Take the shot!"},  // Prends-le !
    {"Group1Message4", "Defending."},      // Je d�fends.
    {"Group1Message5", "Go for it!"},      // Vas-y !
    {"Group1Message6", "Centering!"},      // Centre !
    {"Group1Message7", "All yours."},      // Il est pour toi.
    {"Group1Message8", "In position."},    // En position.
    {"Group1Message9", "Incoming!"},       // En approche !
    {"Group1Message10", "Faking."},        // La feinte.
    {"Group1Message11", "Bumping!"},       // Impact !
    {"Group1Message12", "On your left!"},  // Sur ta gauche !
    {"Group1Message13", "On your right!"}, // Sur ta droite !
    {"Group1Message14", "Passing!"},       // La passe !
    {"Group1Message15", "Rotating Up!"},   // Je monte !
    {"Group1Message16", "Rotating back!"}, // Je recule !
    {"Group1Message17", "You have time!"}, // Tu as le temps !

    {"Group2Message1", "Nice shot!"},    // Beau tir !
    {"Group2Message2", "Great pass!"},   // Belle passe !
    {"Group2Message3", "Thanks!"},       // Merci !
    {"Group2Message4", "What a save!"},  // Quel arr�t !
    {"Group2Message5", "Nice one!"},     // Bien vu !
    {"Group2Message6", "What a play!"},  // Quelle intensit� !
    {"Group2Message7", "Great clear!"},  // Beau d�gagement !
    {"Group2Message8", "Nice block!"},   // Super blocage !
    {"Group2Message9", "Nice bump!"},    // Bel impact !
    {"Group2Message10", "Nice demo!"},   // Jolie d�mo !
    {"Group2Message11", "We got this."}, // On assure !

    {"Group3Message1", "OMG!"},         // Oh mon dieu !
    {"Group3Message2", "Noooo!"},       // Noooon !
    {"Group3Message3", "Wow!"},         // Wow !
    {"Group3Message4", "Close one..."}, // C'�tait pas loin...
    {"Group3Message5", "No way!"},      // Pas possible !
    {"Group3Message6", "Holy cow!"},    // S�rieux ?!
    {"Group3Message7", "Whew."},        // Waouh.
    {"Group3Message8", "Siiiick!"},     // Truc de ouf !
    {"Group3Message9", "Calculated."},  // C'est pr�vu.
    {"Group3Message10", "Savage!"},     // Sauvage !
    {"Group3Message11", "Okay."},       // Ok.
    {"Group3Message12", "Yes!"},        // Oui !

    {"Group4Message1", "$#@%!"},       // $#@%!
    {"Group4Message2", "No problem."}, // Pas de probl�mes.
    {"Group4Message3", "Whoops..."},   // Oups...
    {"Group4Message4", "Sorry!"},      // D�sol� !
    {"Group4Message5", "My bad..."},   // Pardon...
    {"Group4Message6", "Oops!"},       // Oups !
    {"Group4Message7", "My fault."},   // Ma faute.

    {"Group5Message1", "gg"},               // gg
    {"Group5Message2", "Well played."},     // Bien jou�.
    {"Group5Message3", "That was fun!"},    // C'�tait cool !
    {"Group5Message4", "Rematch!"},         // On remet �a !
    {"Group5Message5", "One. More. Game."}, // Encore. Une. Partie.
    {"Group5Message6", "What a game!"},     // Quelle partie !
    {"Group5Message7", "Nice moves!"},      // Super d�placements !
    {"Group5Message8", "Everybody dance!"}, // Que tout le monde dance !
    {"Group5Message9", "Party Up?"},        // On groupe ?

    {"Group6Message4", "This is Rocket League!"} // �a c'est Rocket League !
};

struct UidWrapper {
	FUniqueNetId id;

	static inline std::string unreal_id_to_uid_str(const FUniqueNetId &id) {
		return UniqueIDWrapper::FromEpicAccountID(
		    id.EpicAccountId.ToString(), id.Uid, static_cast<OnlinePlatform>(id.Platform), id.SplitscreenID)
		    .GetIdString();
	}
};

struct FStringBase {
	wchar_t *data;
	int32_t  size;
	int32_t  capacity;
};

struct NetId {
	uint64_t    Uid;
	FSceNpId    NpId;
	std::string EpicAccountId;
	uint8_t     Platform;
	uint8_t     SplitscreenID;

	NetId() {}
	NetId(const FUniqueNetId &unreal_id)
	    : Uid(unreal_id.Uid), NpId(unreal_id.NpId), EpicAccountId(unreal_id.EpicAccountId.ToString()), Platform(unreal_id.Platform),
	      SplitscreenID(unreal_id.SplitscreenID) {}

	FUniqueNetId to_unreal_id() const;
};

struct LastChatPreferences {
	bool Quickchats;
	bool UserChats;
	bool TeamChats;
	bool PartyChats;
	bool TeammateChats;
};

struct ChatData {
	std::string  PlayerName;
	std::string  Message;
	std::string  TimeStamp;
	std::string  IdString;
	int          Team = 420;
	EChatChannel ChatChannel;
	NetId        SenderId;
	bool         IsUser      = false;
	bool         IsQuickchat = false;

public:
	ChatData() = default;
	ChatData(const FGFxChatMessage &msg);
	ChatData(const UGFxData_Chat_TA_execOnChatMessage_Params &params);

public:
	bool is_valid_last_chat(const LastChatPreferences &prefs, uint8_t user_team) const;
};

struct ChatMsgData {
	std::string uid;
	std::string uncensoredMsg;

	ChatMsgData() {}
	ChatMsgData(const FChatMessage &chat);

	static std::string generateUid(const FChatMessage &data);
	static std::string generateUid(UGFxData_Chat_TA_execOnChatMessage_Params *data);
};

struct RankData {
	FPlayerSkillRating skill_data;
	std::string        div;
	std::string        tier;

	RankData() {}
	RankData(const FPlayerSkillRating &skill) { assign(skill); }

	void        assign(const FPlayerSkillRating &skill);
	std::string get_rank_str() const;
};

struct ChatterRanks {
	std::string playerName;
	RankData    ones;
	RankData    twos;
	RankData    threes;
	RankData    casual;
	// std::unordered_map <std::string, Rank> ranks;

	ChatterRanks() {}
	ChatterRanks(const std::string &name, const RankData &ones, const RankData &twos, const RankData &threes, const RankData &cas)
	    : playerName(name), ones(ones), twos(twos), threes(threes), casual(cas) {}
	ChatterRanks(const ChatData &chat, UOnlineGameSkill_X *game_skill) { assign(chat, game_skill); }

	void        assign(const ChatData &chat, UOnlineGameSkill_X *game_skill);
	RankData    get_rank(ERankPlaylists playlist);
	std::string get_all_ranks_str() const;
	std::string get_playlist_rank_str(ERankPlaylists playlist);

	static float              calculate_skill_rating(float mu);
	static FPlayerSkillRating get_skill_rating(const FUniqueNetId &id, int playlist_id, UOnlineGameSkill_X *game_skill);
	static std::string        get_playlist_str(ERankPlaylists playlist);
};

/*
OFFICIAL NAME | CODE NAME
-----------------------
Haymaker      | BallSpring
Boot          | CarSpring
Magnetizer    | BallMagnet
Swapper       | EnemySwapper
Spikes        | BallVelcro
Grappling Hook| GrapplingHook
Power Hitter  | PowerHitter
Plunger       | BallLasso
Freezer       | BallFreeze
Disruptor     | EnemyBooster
Tornado       | Tornado
*/
inline const std::unordered_map<std::string, std::string> g_rumbleFriendlyNames = {
    {"BallSpring", "Haymaker"},
    {"CarSpring", "Boot"},
    {"BallMagnet", "Magnetizer"},
    {"EnemySwapper", "Swapper"},
    {"BallVelcro", "Spikes"},
    {"GrapplingHook", "Grappling Hook"},
    {"Powerhitter", "Power Hitter"},
    {"BallLasso", "Plunger"},
    {"BallFreeze", "Freezer"},
    {"EnemyBooster", "Disruptor"},
    {"Tornado", "Tornado"},
};
