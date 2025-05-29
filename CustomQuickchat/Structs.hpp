#pragma once
#include "pch.h"
#include <regex>
#include "Components/Components/Instances.hpp"

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
    None =                  0,
    WordVariation =         1,
    SpeechToText =          2,
    SpeechToTextUwu =       3,
    SpeechToTextSarcasm =   4,
    LastChat =              5,
    LastChatUwu =           6,
    LastChatSarcasm =       7,
    BlastAll =              8,
    BlastCasual =           9,
    Blast1v1 =              10,
    Blast2v2 =              11,
    Blast3v3 =              12,
    Forfeit =               13,
    ExitToMainMenu =        14
};

enum class ETextEffect : uint8_t
{
    None =          0,
    Uwu =           1,
    Sarcasm =       2,
};

const std::unordered_map<std::string, EKeyword> keywordsMap =
{
    { "speechToText",               EKeyword::SpeechToText          },
    { "speechToText sarcasm",       EKeyword::SpeechToTextSarcasm   },
    { "speechToText uwu",           EKeyword::SpeechToTextUwu       },
    { "lastChat",                   EKeyword::LastChat              },
    { "lastChat uwu",               EKeyword::LastChatUwu           },
    { "lastChat sarcasm",           EKeyword::LastChatSarcasm       },
    { "blast all",                  EKeyword::BlastAll              },
    { "blast casual",               EKeyword::BlastCasual           },
    { "blast 1v1",                  EKeyword::Blast1v1              },
    { "blast 2v2",                  EKeyword::Blast2v2              },
    { "blast 3v3",                  EKeyword::Blast3v3              },
    { "forfeit",                    EKeyword::Forfeit               },
    { "menu",                       EKeyword::ExitToMainMenu        },
};

// ----------------------------------------------------------------------------


enum class EBindingType : uint8_t
{
    Combination =   0,
    Sequence =      1
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
    EChatChannel chatMode =             EChatChannel::EChatChannel_Match;
    EBindingType bindingType =          EBindingType::Combination;
    EKeyword keyWord =                  EKeyword::None;
    ETextEffect textEffect =            ETextEffect::None;
    bool enabled =                      true;
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
            return false;   // if there's no valid binding type for some reason
        }
    }
    
    bool CheckCombination(
        const ButtonPress& buttonEvent,
        const std::unordered_map<std::string, bool>& keyStates,
        const std::chrono::steady_clock::time_point& lastBindingActivated,
        const std::chrono::duration<double>& minDelayBetweenBindings)
    {
        if (buttons.empty())
            return false;

        for (const std::string& button : buttons)
        {
            if (keyStates.contains(button))
            {
                if (!keyStates.at(button))
                    return false;
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
        if (buttons.size() < 2)
            return false;   // exit if there's not at least 2 buttons in binding

        bool button1Pressed = buttonEvent.buttonName == buttons[0];
        bool button2Pressed = buttonEvent.buttonName == buttons[1];

        if (!button1Pressed && !button2Pressed)
            return false;   // early exit if no buttons from binding have been pressed

        // if first button press data is empty...
        if (firstButtonState.buttonName.empty() || firstButtonState.pressedTime == epochTime)
        {
            if (button1Pressed)
                firstButtonState = buttonEvent;     // update first button press data then exit
            return false;
        }

        // if first button press data exists.......
        
        // if first button press data is too old... reset or update it, then exit
        if (buttonEvent.pressedTime > firstButtonState.pressedTime + maxTimeWindow)
        {
            if (button1Pressed)
                firstButtonState = buttonEvent;     // update first button press data
            else
                firstButtonState.Reset(epochTime);  // reset info bc 1st button doesn't match
            return false;
        }

        // if first button press data is still valid.......

        if (!button2Pressed)
            return false;

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

            firstButtonState.Reset(epochTime);  // binding was triggered too early, just reset it (bc it prolly wasn't meant to be triggered)
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
                keyWord = it->second;                   // update binding's keyword
                textEffect = GetTextEffect(keyWord);    // update binding's text effect (if any)
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



// ============================================= RANKS =============================================


enum class ERankPlaylists : uint8_t
{
    Ones =      0,
    Twos =      1,
    Threes =    2,
    Casual =    3
};


const std::unordered_map<int, std::string> skill_tier_to_label =
{
    { 0,    "Casual" },
    { 1,    "B1" },
    { 2,    "B2" },
    { 3,    "B3" },
    { 4,    "S1" },
    { 5,    "S2" },
    { 6,    "S3" },
    { 7,    "G1" },
    { 8,    "G2" },
    { 9,    "G3" },
    { 10,   "P1" },
    { 11,   "P2" },
    { 12,   "P3" },
    { 13,   "D1" },
    { 14,   "D2" },
    { 15,   "D3" },
    { 16,   "C1" },
    { 17,   "C2" },
    { 18,   "C3" },
    { 19,   "GC1" },
    { 20,   "GC2" },
    { 21,   "GC3" },
    { 22,   "SSL" }
};


enum class SkillTier
{
    Unranked =          0,
    Bronze1 =           1,
    Bronze2 =           2,
    Bronze3 =           3,
    Silver1 =           4,
    Silver2 =           5,
    Silver3 =           6,
    Gold1 =             7,
    Gold2 =             8,
    Gold3 =             9,
    Platinum1 =         10,
    Platinum2 =         11,
    Platinum3 =         12,
    Diamond1 =          13,
    Diamond2 =          14,
    Diamond3 =          15,
    Champ1 =            16,
    Champ2 =            17,
    Champ3 =            18,
    GrandChamp1 =       19,
    GrandChamp2 =       20,
    GrandChamp3 =       21,
    SupersonicLegend =  22
};


// credit to https://github.com/JulienML/BetterChat/ thx fam
const std::map<std::string, std::string> quickchat_ids_to_text =
{
        {"Group1Message1",  "I got it!"},               // Je l'ai !
        {"Group1Message2",  "Need boost!"},             // Besoin de turbo !
        {"Group1Message3",  "Take the shot!"},          // Prends-le !
        {"Group1Message4",  "Defending."},              // Je défends.
        {"Group1Message5",  "Go for it!"},              // Vas-y !
        {"Group1Message6",  "Centering!"},              // Centre !
        {"Group1Message7",  "All yours."},              // Il est pour toi.
        {"Group1Message8",  "In position."},            // En position.
        {"Group1Message9",  "Incoming!"},               // En approche !
        {"Group1Message10", "Faking."},                 // La feinte.
        {"Group1Message11", "Bumping!"},                // Impact !
        {"Group1Message12", "On your left!"},           // Sur ta gauche !
        {"Group1Message13", "On your right!"},          // Sur ta droite !
        {"Group1Message14", "Passing!"},                // La passe !
        {"Group1Message15", "Rotating Up!"},            // Je monte !
        {"Group1Message16", "Rotating back!"},          // Je recule !
        {"Group1Message17", "You have time!"},          // Tu as le temps !

        {"Group2Message1",  "Nice shot!"},              // Beau tir !
        {"Group2Message2",  "Great pass!"},             // Belle passe !
        {"Group2Message3",  "Thanks!"},                 // Merci !
        {"Group2Message4",  "What a save!"},            // Quel arrêt !
        {"Group2Message5",  "Nice one!"},               // Bien vu !
        {"Group2Message6",  "What a play!"},            // Quelle intensité !
        {"Group2Message7",  "Great clear!"},            // Beau dégagement !
        {"Group2Message8",  "Nice block!"},             // Super blocage !
        {"Group2Message9",  "Nice bump!"},              // Bel impact !
        {"Group2Message10", "Nice demo!"},              // Jolie démo !
        {"Group2Message11", "We got this."},            // On assure !

        {"Group3Message1",  "OMG!"},                    // Oh mon dieu !
        {"Group3Message2",  "Noooo!"},                  // Noooon !
        {"Group3Message3",  "Wow!"},                    // Wow !
        {"Group3Message4",  "Close one..."},            // C'était pas loin...
        {"Group3Message5",  "No way!"},                 // Pas possible !
        {"Group3Message6",  "Holy cow!"},               // Sérieux ?!
        {"Group3Message7",  "Whew."},                   // Waouh.
        {"Group3Message8",  "Siiiick!"},                // Truc de ouf !
        {"Group3Message9",  "Calculated."},             // C'est prévu.
        {"Group3Message10", "Savage!"},                 // Sauvage !
        {"Group3Message11", "Okay."},                   // Ok.
        {"Group3Message12", "Yes!"},                    // Oui !

        {"Group4Message1",  "$#@%!"},                   // $#@%!
        {"Group4Message2",  "No problem."},             // Pas de problèmes.
        {"Group4Message3",  "Whoops..."},               // Oups...
        {"Group4Message4",  "Sorry!"},                  // Désolé !
        {"Group4Message5",  "My bad..."},               // Pardon...
        {"Group4Message6",  "Oops!"},                   // Oups !
        {"Group4Message7",  "My fault."},               // Ma faute.

        {"Group5Message1",  "gg"},                      // gg
        {"Group5Message2",  "Well played."},            // Bien joué.
        {"Group5Message3",  "That was fun!"},           // C'était cool !
        {"Group5Message4",  "Rematch!"},                // On remet ça !
        {"Group5Message5",  "One. More. Game."},        // Encore. Une. Partie.
        {"Group5Message6",  "What a game!"},            // Quelle partie !
        {"Group5Message7",  "Nice moves!"},             // Super déplacements !
        {"Group5Message8",  "Everybody dance!"},        // Que tout le monde dance !
        {"Group5Message9",  "Party Up?"},               // On groupe ?

        {"Group6Message4",  "This is Rocket League!"}   // Ça c'est Rocket League !
};


struct UidWrapper
{
    FUniqueNetId id;

    static inline std::string unreal_id_to_uid_str(const FUniqueNetId& id)
    {
        return UniqueIDWrapper::FromEpicAccountID(id.EpicAccountId.ToString(), id.Uid, static_cast<OnlinePlatform>(id.Platform), id.SplitscreenID).GetIdString();
    }
};

struct FStringBase
{
    wchar_t*    data;
    int32_t     size;
    int32_t     capacity;
};

struct NetId
{
    uint64_t            Uid;
    FSceNpId            NpId;
    std::string         EpicAccountId;
    uint8_t             Platform;
    uint8_t             SplitscreenID;

    NetId() {}
    NetId(const FUniqueNetId& unreal_id) :
        Uid(unreal_id.Uid),
        NpId(unreal_id.NpId),
        EpicAccountId(unreal_id.EpicAccountId.ToString()),
        Platform(unreal_id.Platform),
        SplitscreenID(unreal_id.SplitscreenID) {}

    FUniqueNetId to_unreal_id() const
    {
        FUniqueNetId id;
        id.Uid =                Uid;
        id.NpId =               NpId;
        id.EpicAccountId =      StringUtils::newFString(EpicAccountId);
        id.Platform =           Platform;
        id.SplitscreenID =      SplitscreenID;
        
        return id;
    }
};


struct LastChatPreferences
{
    bool Quickchats;
    bool UserChats;
    bool TeamChats;
    bool PartyChats;
    bool TeammateChats;
};


struct ChatData
{
    std::string         PlayerName;
    std::string         Message;
    std::string         TimeStamp;
    std::string         IdString;
    int                 Team;
    EChatChannel        ChatChannel;
    NetId               SenderId;
    bool                IsUser;
    bool                IsQuickchat;

    ChatData(const FGFxChatMessage& msg)
    {
        // parse quickchat message if necessary
        std::string chat_text = msg.Message.ToString();
        if (msg.bPreset)
        {
            auto it = quickchat_ids_to_text.find(chat_text);
            if (it != quickchat_ids_to_text.end())
                chat_text = it->second;
        }

        PlayerName =        msg.PlayerName.ToString();
        Message =           chat_text;
        TimeStamp =         msg.TimeStamp.ToString();
        Team =              msg.Team;
        ChatChannel =       static_cast<EChatChannel>(msg.ChatChannel);
        SenderId =          msg.SenderId;
        IsUser =            msg.bLocalPlayer;
        IsQuickchat =       msg.bPreset;

        IdString = UidWrapper::unreal_id_to_uid_str(msg.SenderId);
    }


    bool is_valid_last_chat(const LastChatPreferences& prefs, uint8_t user_team) const;
};


struct ChatMsgData
{
    std::string uid;
    std::string uncensoredMsg;

    ChatMsgData() {}
    ChatMsgData(const FChatMessage& chat)
    {
        uncensoredMsg = chat.Message.ToString();
        uid = generateUid(chat);
    }

    static std::string generateUid(const FChatMessage& data)
    {
        return std::format("{}|{}|{}|{}",
            data.PlayerName.ToString(), data.TimeStamp.ToString(), data.Message.size(), data.ChatChannel);
    }

    static std::string generateUid(UGFxData_Chat_TA_execOnChatMessage_Params* data)
    {
        if (!data)
            return std::string();

        return std::format("{}|{}|{}|{}",
            data->PlayerName.ToString(), data->TimeStamp.ToString(), data->Message.size(), data->ChatChannel);
    }
};


struct RankData
{
    FPlayerSkillRating skill_data;
    std::string div;
    std::string tier;

    RankData() {}
    RankData(const FPlayerSkillRating& skill) { assign(skill); }

    void assign(const FPlayerSkillRating& skill)
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

    inline std::string get_rank_str() const
    {
        if (skill_data.Tier == 0 || skill_data.MatchesPlayed == 0)
        {
            return "--";
        }
        return tier + "..div" + div;
    }
};


struct ChatterRanks
{
    std::string playerName;
    RankData ones;
    RankData twos;
    RankData threes;
    RankData casual;
    //std::unordered_map <std::string, Rank> ranks;

    ChatterRanks() {}

    ChatterRanks(const std::string& name, const RankData& ones, const RankData& twos, const RankData& threes, const RankData& cas) :
    playerName(name), ones(ones), twos(twos), threes(threes), casual(cas) {}

    ChatterRanks(const ChatData& chat, UOnlineGameSkill_X* game_skill) { assign(chat, game_skill); }


    void assign(const ChatData& chat, UOnlineGameSkill_X* game_skill)
    {
        if (!game_skill)
        {
            LOG("Unable to get chatter ranks... UOnlineGameSkill_X* is null");
            return;
        }

        playerName = chat.PlayerName;

        ones =      get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::RankedSoloDuel),       game_skill);
        twos =      get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::RankedTeamDoubles),    game_skill);
        threes =    get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::RankedStandard),       game_skill);
        casual =    get_skill_rating(chat.SenderId.to_unreal_id(), static_cast<int>(PlaylistIds::Casual),               game_skill);
    }


    inline RankData get_rank(ERankPlaylists playlist)
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


    inline std::string get_all_ranks_str() const
    {
        // to make return line readable
        std::string ones_str =      ones.get_rank_str();
        std::string twos_str =      twos.get_rank_str();
        std::string threes_str =    threes.get_rank_str();

        return playerName + ": [1s] " + ones_str + " [2s] " + twos_str + " [3s] " + threes_str;
    }


    inline std::string get_playlist_rank_str(ERankPlaylists playlist)
    {
        std::string rank_str;

        RankData specificRank = get_rank(playlist);

        rank_str = playerName + " [" + ChatterRanks::get_playlist_str(playlist) + "] ";

        if (playlist == ERankPlaylists::Casual)
        {
            rank_str += std::to_string(std::lround(specificRank.skill_data.MMR)) + " mmr (" + std::to_string(specificRank.skill_data.MatchesPlayed) + " matches)";
        }
        else if (specificRank.skill_data.Tier != 0 )
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

    
    static inline FPlayerSkillRating get_skill_rating(const FUniqueNetId& id, int playlist_id, UOnlineGameSkill_X* game_skill)
    {
        if (!game_skill)
            return FPlayerSkillRating{};

        FPlayerSkillRating rank_data = game_skill->GetPlayerRating(id, playlist_id);
        rank_data.MMR = calculate_skill_rating(rank_data.Mu);       // <--- change to be the value that everyone refers to as "MMR"

        return rank_data;
    }


    // NOTE: This formula is what's used for RL leaderboards and is what people refer to as "MMR"
    // ... but it's not what is used internally to determine matchmaking. Apparently that would be the Microsoft TrueSkill formula: Mu - (3 * Sigma) 
    static inline float calculate_skill_rating(float mu)
    {
        return (mu * 20) + 100;
    }

    
    static inline std::string get_playlist_str(ERankPlaylists playlist)
    {
        switch (playlist)
        {
        case ERankPlaylists::Ones:
            //return "1v1";
            return "1s";
        case ERankPlaylists::Twos:
            //return "2v2";
            return "2s";
        case ERankPlaylists::Threes:
            //return "3v3";
            return "3s";
        case ERankPlaylists::Casual:
            return "casual";
        default:
            return "";
        }
    }
};
