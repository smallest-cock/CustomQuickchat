#include "pch.h"
#include "CustomQuickchat.hpp"
#include "components/Instances.hpp"
#include "Events.hpp"


BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


void CustomQuickchat::onLoad()
{
    _globalCvarManager = cvarManager;
    
    if (!Instances.InitGlobals())
        return;

    // ====================================== cvars ===========================================

    // bools
    auto enabled_cvar =                         registerCvar_Bool(Cvars::enabled,                       true);
    auto overrideDefaultQuickchats_cvar =       registerCvar_Bool(Cvars::overrideDefaultQuickchats,     true);
    auto blockDefaultQuickchats_cvar =          registerCvar_Bool(Cvars::blockDefaultQuickchats,        false);
    auto disablePostMatchQuickchats_cvar =      registerCvar_Bool(Cvars::disablePostMatchQuickchats,    false);
    auto disableChatTimeout_cvar =              registerCvar_Bool(Cvars::disableChatTimeout,            true);
    auto useCustomChatTimeoutMsg_cvar =         registerCvar_Bool(Cvars::useCustomChatTimeoutMsg,       true);
    auto removeTimestamps_cvar =                registerCvar_Bool(Cvars::removeTimestamps,              true);
    auto enableSTTNotifications_cvar =          registerCvar_Bool(Cvars::enableSTTNotifications,        true);
    auto autoCalibrateMic_cvar =                registerCvar_Bool(Cvars::autoCalibrateMic,              true);
    auto user_chats_in_last_chat_cvar =         registerCvar_Bool(Cvars::user_chats_in_last_chat,       false);
    auto teammate_chats_in_last_chat_cvar =     registerCvar_Bool(Cvars::teammate_chats_in_last_chat,   true);
    auto quickchats_in_last_chat_cvar =         registerCvar_Bool(Cvars::quickchats_in_last_chat,       true);
    auto party_chats_in_last_chat_cvar =        registerCvar_Bool(Cvars::party_chats_in_last_chat,      true);
    auto team_chats_in_last_chat_cvar =         registerCvar_Bool(Cvars::team_chats_in_last_chat,       true);
    auto randomize_sarcasm_cvar =               registerCvar_Bool(Cvars::randomize_sarcasm,             true);
    auto uncensorChats_cvar =                   registerCvar_Bool(Cvars::uncensorChats,                 true);

    removeTimestamps_cvar.bindTo(m_removeTimestamps);
    uncensorChats_cvar.bindTo(m_uncensorChats);

    // numbers
    auto micEnergyThreshold_cvar =              registerCvar_Number(Cvars::micEnergyThreshold,          420);
    auto sequenceTimeWindow_cvar =              registerCvar_Number(Cvars::sequenceTimeWindow,          2,      true, 0,    10);
    auto minBindingDelay_cvar =                 registerCvar_Number(Cvars::minBindingDelay,             0.05,   true, 0,    1);
    auto notificationDuration_cvar =            registerCvar_Number(Cvars::notificationDuration,        3,      true, 1.5,  10);
    auto beginSpeechTimeout_cvar =              registerCvar_Number(Cvars::beginSpeechTimeout,          3,      true, 1.5,  10);
    auto speechProcessingTimeout_cvar =         registerCvar_Number(Cvars::speechProcessingTimeout,     10,     true, 3,    500);
    auto micCalibrationTimeout_cvar =           registerCvar_Number(Cvars::micCalibrationTimeout,       10,     true, 3,    20);
    auto websocket_port_cvar =                  registerCvar_Number(Cvars::websocket_port,              8003,   true, 0,    65535);

    // strings
    auto customChatTimeoutMsg_cvar =            registerCvar_String(Cvars::customChatTimeoutMsg, "Wait [Time] seconds lil bro");


    // cvar change callbacks
    enabled_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enabled,
        this, std::placeholders::_1, std::placeholders::_2));

    enableSTTNotifications_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enableSTTNotifications,
        this, std::placeholders::_1, std::placeholders::_2));

    overrideDefaultQuickchats_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_overrideDefaultQuickchats,
        this, std::placeholders::_1, std::placeholders::_2));

    blockDefaultQuickchats_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_blockDefaultQuickchats,
        this, std::placeholders::_1, std::placeholders::_2));

    useCustomChatTimeoutMsg_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_useCustomChatTimeoutMsg,
        this, std::placeholders::_1, std::placeholders::_2));

    customChatTimeoutMsg_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_customChatTimeoutMsg,
        this, std::placeholders::_1, std::placeholders::_2));


    // ===================================== commands =========================================

    registerCommand(Commands::toggleEnabled,            std::bind(&CustomQuickchat::cmd_toggleEnabled, this, std::placeholders::_1));
    registerCommand(Commands::listBindings,             std::bind(&CustomQuickchat::cmd_listBindings, this, std::placeholders::_1));
    registerCommand(Commands::list_custom_chat_labels,  std::bind(&CustomQuickchat::cmd_list_custom_chat_labels, this, std::placeholders::_1));
    registerCommand(Commands::list_playlist_info,       std::bind(&CustomQuickchat::cmd_list_playlist_info, this, std::placeholders::_1));
    registerCommand(Commands::exitToMainMenu,           std::bind(&CustomQuickchat::cmd_exitToMainMenu, this, std::placeholders::_1));
    registerCommand(Commands::forfeit,                  std::bind(&CustomQuickchat::cmd_forfeit, this, std::placeholders::_1));
    registerCommand(Commands::test,                     std::bind(&CustomQuickchat::cmd_test, this, std::placeholders::_1));
    registerCommand(Commands::test2,                    std::bind(&CustomQuickchat::cmd_test2, this, std::placeholders::_1));


    // ======================================= hooks ==========================================

    gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::KeyPressed,
        std::bind(&CustomQuickchat::Event_KeyPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    gameWrapper->HookEventWithCaller<ActorWrapper>(Events::GFxHUD_TA_ChatPreset,
        std::bind(&CustomQuickchat::Event_GFxHUD_TA_ChatPreset, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::ApplyChatSpamFilter,
        std::bind(&CustomQuickchat::Event_ApplyChatSpamFilter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    gameWrapper->HookEventWithCaller<ActorWrapper>(Events::GFxHUD_TA_NotifyChatDisabled,
        std::bind(&CustomQuickchat::Event_GFxHUD_TA_NotifyChatDisabled, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::PushMenu,
        std::bind(&CustomQuickchat::Event_PushMenu, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::PopMenu,
        std::bind(&CustomQuickchat::Event_PopMenu, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::PlayerController_EnterStartState,
        std::bind(&CustomQuickchat::Event_PlayerController_EnterStartState, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    gameWrapper->HookEventPost(Events::LoadingScreenStart,
        std::bind(&CustomQuickchat::Event_LoadingScreenStart, this, std::placeholders::_1));

    gameWrapper->HookEventPost(Events::MatchEnded, [this](std::string eventName) { m_matchEnded = true; });

    // track the state of the chatbox UI
    auto set_chatbox_open = [this](std::string eventName) { m_chatboxOpen = true; };
    gameWrapper->HookEventPost(Events::GFxData_Chat_TA_OpenChat,        set_chatbox_open);
    gameWrapper->HookEventPost(Events::GFxData_Chat_TA_OpenTeamChat,    set_chatbox_open);
    gameWrapper->HookEventPost(Events::GFxData_Chat_TA_OpenPartyChat,   set_chatbox_open);
    gameWrapper->HookEventPost(Events::GFxData_Chat_TA_ClearDistracted, [this](std::string eventName) { m_chatboxOpen = false; });


    hookEvent(Events::SendChatPresetMessage, [this](std::string eventName)
    {
        // reset/update data for all bindings
        lastBindingActivated = std::chrono::steady_clock::now();
        ResetAllFirstButtonStates();
    });

    // determine custom chat labels based on user's bindings
    hookWithCallerPost(Events::InitUIBindings,
        std::bind(&CustomQuickchat::Event_InitUIBindings, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // apply custom chat labels to ui
    hookWithCaller(Events::OnPressChatPreset,
        std::bind(&CustomQuickchat::Event_OnPressChatPreset, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	hookWithCaller(Events::HUDBase_TA_OnChatMessage,
        std::bind(&CustomQuickchat::event_HUDBase_TA_OnChatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    
    hookWithCaller(Events::GFxData_Chat_TA_OnChatMessage,
        std::bind(&CustomQuickchat::event_GFxData_Chat_TA_OnChatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // ========================================================================================

    
    // other init
    initStuffOnLoad();

    LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload()
{
    writeBindingsToJson();      // just to make sure any unsaved changes are saved before exiting

#ifdef USE_SPEECH_TO_TEXT

    Websocket->StopClient();
    stop_websocket_server();

#endif // USE_SPEECH_TO_TEXT

}