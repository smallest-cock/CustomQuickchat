#include "pch.h"
#include "CustomQuickchat.h"


BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


void CustomQuickchat::onLoad()
{
	_globalCvarManager = cvarManager;
	

	// init globals
	Instances.InitGlobals();
	if (!Instances.CheckGlobals()) return;


	// ====================================== cvars ===========================================

	// bools
	auto enabled_cvar =							RegisterCvar_Bool(Cvars::enabled,						true);
	auto overrideDefaultQuickchats_cvar =		RegisterCvar_Bool(Cvars::overrideDefaultQuickchats,		true);
	auto blockDefaultQuickchats_cvar =			RegisterCvar_Bool(Cvars::blockDefaultQuickchats,		false);
	auto disablePostMatchQuickchats_cvar =		RegisterCvar_Bool(Cvars::disablePostMatchQuickchats,	false);
	auto disableChatTimeout_cvar =				RegisterCvar_Bool(Cvars::disableChatTimeout,			true);
	auto useCustomChatTimeoutMsg_cvar =			RegisterCvar_Bool(Cvars::useCustomChatTimeoutMsg,		true);
	auto removeTimestamps_cvar =				RegisterCvar_Bool(Cvars::removeTimestamps,				true);
	auto enableSTTNotifications_cvar =			RegisterCvar_Bool(Cvars::enableSTTNotifications,		true);
	auto autoCalibrateMic_cvar =				RegisterCvar_Bool(Cvars::autoCalibrateMic,				true);
	auto user_chats_in_last_chat_cvar =			RegisterCvar_Bool(Cvars::user_chats_in_last_chat,		false);
	auto teammate_chats_in_last_chat_cvar =		RegisterCvar_Bool(Cvars::teammate_chats_in_last_chat,	true);
	auto quickchats_in_last_chat_cvar =			RegisterCvar_Bool(Cvars::quickchats_in_last_chat,		true);
	auto party_chats_in_last_chat_cvar =		RegisterCvar_Bool(Cvars::party_chats_in_last_chat,		true);
	auto team_chats_in_last_chat_cvar =			RegisterCvar_Bool(Cvars::team_chats_in_last_chat,		true);
	auto randomize_sarcasm_cvar =				RegisterCvar_Bool(Cvars::randomize_sarcasm,				true);

	// numbers
	auto micEnergyThreshold_cvar =				RegisterCvar_Number(Cvars::micEnergyThreshold,			420);
	auto sequenceTimeWindow_cvar =				RegisterCvar_Number(Cvars::sequenceTimeWindow,			2,		true, 0,	10);
	auto minBindingDelay_cvar =					RegisterCvar_Number(Cvars::minBindingDelay,				0.05,	true, 0,	1);
	auto notificationDuration_cvar =			RegisterCvar_Number(Cvars::notificationDuration,		3,		true, 1.5,	10);
	auto beginSpeechTimeout_cvar =				RegisterCvar_Number(Cvars::beginSpeechTimeout,			3,		true, 1.5,	10);
	auto speechProcessingTimeout_cvar =			RegisterCvar_Number(Cvars::speechProcessingTimeout,		10,		true, 3,	500);
	auto micCalibrationTimeout_cvar =			RegisterCvar_Number(Cvars::micCalibrationTimeout,		10,		true, 3,	20);
	auto websocket_port_cvar =					RegisterCvar_Number(Cvars::websocket_port,				8003,	true, 0,	65535);

	// strings
	auto customChatTimeoutMsg_cvar =			RegisterCvar_String(Cvars::customChatTimeoutMsg, "Wait [Time] seconds lil bro");


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

	RegisterCommand(Commands::toggleEnabled,			std::bind(&CustomQuickchat::cmd_toggleEnabled, this, std::placeholders::_1));
	RegisterCommand(Commands::listBindings,				std::bind(&CustomQuickchat::cmd_listBindings, this, std::placeholders::_1));
	RegisterCommand(Commands::list_custom_chat_labels,	std::bind(&CustomQuickchat::cmd_list_custom_chat_labels, this, std::placeholders::_1));
	RegisterCommand(Commands::list_playlist_info,		std::bind(&CustomQuickchat::cmd_list_playlist_info, this, std::placeholders::_1));
	RegisterCommand(Commands::exitToMainMenu,			std::bind(&CustomQuickchat::cmd_exitToMainMenu, this, std::placeholders::_1));
	RegisterCommand(Commands::forfeit,					std::bind(&CustomQuickchat::cmd_forfeit, this, std::placeholders::_1));
	RegisterCommand(Commands::test,						std::bind(&CustomQuickchat::cmd_test, this, std::placeholders::_1));
	RegisterCommand(Commands::test2,					std::bind(&CustomQuickchat::cmd_test2, this, std::placeholders::_1));


	// ======================================= hooks ==========================================

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::KeyPressed,
		std::bind(&CustomQuickchat::Event_KeyPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::GFxHUD_TA_ChatPreset,
		std::bind(&CustomQuickchat::Event_GFxHUD_TA_ChatPreset, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::ApplyChatSpamFilter,
		std::bind(&CustomQuickchat::Event_ApplyChatSpamFilter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::GFxHUD_TA_NotifyChatDisabled,
		std::bind(&CustomQuickchat::Event_NotifyChatDisabled, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::OnChatMessage,
		std::bind(&CustomQuickchat::Event_OnChatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::PushMenu,
		std::bind(&CustomQuickchat::Event_PushMenu, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::PopMenu,
		std::bind(&CustomQuickchat::Event_PopMenu, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventPost(Events::LoadingScreenStart, std::bind(&CustomQuickchat::Event_LoadingScreenStart, this, std::placeholders::_1));

	gameWrapper->HookEventPost(Events::MatchEnded, [this](std::string eventName) { matchEnded = true; });
	gameWrapper->HookEventPost(Events::EnterStartState, [this](std::string eventName) { inGameEvent = true; });

	// track the state of the chatbox UI
	auto set_chatbox_open = [this](std::string eventName) { chatbox_open = true; };
	gameWrapper->HookEventPost(Events::GFxData_Chat_TA_OpenChat,		set_chatbox_open);
	gameWrapper->HookEventPost(Events::GFxData_Chat_TA_OpenTeamChat,	set_chatbox_open);
	gameWrapper->HookEventPost(Events::GFxData_Chat_TA_OpenPartyChat,	set_chatbox_open);
	gameWrapper->HookEventPost(Events::GFxData_Chat_TA_ClearDistracted, [this](std::string eventName) { chatbox_open = false; });


	gameWrapper->HookEvent(Events::SendChatPresetMessage, [this](std::string eventName)
	{
		// reset/update data for all bindings
		lastBindingActivated = std::chrono::steady_clock::now();
		ResetAllFirstButtonStates();
	});

	// determine custom chat labels based on user's bindings
	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::InitUIBindings,
		std::bind(&CustomQuickchat::Event_InitUIBindings, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	// apply custom chat labels to ui
	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::OnPressChatPreset,
		std::bind(&CustomQuickchat::Event_OnPressChatPreset, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	// ========================================================================================


	// other init
	InitStuffOnLoad();


	LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload()
{
	WriteBindingsToJson();		// just to make sure any unsaved changes are saved before exiting

#ifdef USE_SPEECH_TO_TEXT

	Websocket->StopClient();
	stop_websocket_server();

#endif // USE_SPEECH_TO_TEXT

}