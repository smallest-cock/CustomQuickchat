#include "pch.h"
#include "CustomQuickchat.hpp"
#include "components/Instances.hpp"
#include "HookManager.hpp"

BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void CustomQuickchat::onLoad()
{
	_globalCvarManager = cvarManager;

	Hooks.init(gameWrapper);
	if (!Instances.InitGlobals())
		return;

	initCvars();
	initCommands();
	initHooks();
	initStuffOnLoad(); // other init

	LOG("CustomQuickchat loaded! :)");
}

void CustomQuickchat::onUnload()
{
	writeBindingsToJson(); // just to make sure any unsaved changes are saved before exiting

#ifdef USE_SPEECH_TO_TEXT
	Websocket->StopClient();
	stop_websocket_server();
#endif
}

void CustomQuickchat::initCvars()
{
	// bools
	auto enabled_cvar                     = registerCvar_Bool(Cvars::enabled, true);
	auto overrideDefaultQuickchats_cvar   = registerCvar_Bool(Cvars::overrideDefaultQuickchats, true);
	auto blockDefaultQuickchats_cvar      = registerCvar_Bool(Cvars::blockDefaultQuickchats, false);
	auto disablePostMatchQuickchats_cvar  = registerCvar_Bool(Cvars::disablePostMatchQuickchats, false);
	auto disableChatTimeout_cvar          = registerCvar_Bool(Cvars::disableChatTimeout, true);
	auto useCustomChatTimeoutMsg_cvar     = registerCvar_Bool(Cvars::useCustomChatTimeoutMsg, true);
	auto removeTimestamps_cvar            = registerCvar_Bool(Cvars::removeTimestamps, true);
	auto enableSTTNotifications_cvar      = registerCvar_Bool(Cvars::enableSTTNotifications, true);
	auto autoCalibrateMic_cvar            = registerCvar_Bool(Cvars::autoCalibrateMic, true);
	auto user_chats_in_last_chat_cvar     = registerCvar_Bool(Cvars::user_chats_in_last_chat, false);
	auto teammate_chats_in_last_chat_cvar = registerCvar_Bool(Cvars::teammate_chats_in_last_chat, true);
	auto quickchats_in_last_chat_cvar     = registerCvar_Bool(Cvars::quickchats_in_last_chat, true);
	auto party_chats_in_last_chat_cvar    = registerCvar_Bool(Cvars::party_chats_in_last_chat, true);
	auto team_chats_in_last_chat_cvar     = registerCvar_Bool(Cvars::team_chats_in_last_chat, true);
	auto randomize_sarcasm_cvar           = registerCvar_Bool(Cvars::randomize_sarcasm, true);
	auto uncensorChats_cvar               = registerCvar_Bool(Cvars::uncensorChats, true);

	removeTimestamps_cvar.bindTo(m_removeTimestamps);
	uncensorChats_cvar.bindTo(m_uncensorChats);

	// numbers
	auto micEnergyThreshold_cvar      = registerCvar_Number(Cvars::micEnergyThreshold, 420);
	auto sequenceTimeWindow_cvar      = registerCvar_Number(Cvars::sequenceTimeWindow, 2, true, 0, 10);
	auto minBindingDelay_cvar         = registerCvar_Number(Cvars::minBindingDelay, 0.05, true, 0, 1);
	auto notificationDuration_cvar    = registerCvar_Number(Cvars::notificationDuration, 3, true, 1.5, 10);
	auto beginSpeechTimeout_cvar      = registerCvar_Number(Cvars::beginSpeechTimeout, 3, true, 1.5, 10);
	auto speechProcessingTimeout_cvar = registerCvar_Number(Cvars::speechProcessingTimeout, 10, true, 3, 500);
	auto micCalibrationTimeout_cvar   = registerCvar_Number(Cvars::micCalibrationTimeout, 10, true, 3, 20);
	auto websocket_port_cvar          = registerCvar_Number(Cvars::websocket_port, 8003, true, 0, 65535);

	// strings
	auto customChatTimeoutMsg_cvar = registerCvar_String(Cvars::customChatTimeoutMsg, "Wait [Time] seconds lil bro");

	// cvar change callbacks
	enabled_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enabled, this, std::placeholders::_1, std::placeholders::_2));

	enableSTTNotifications_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_enableSTTNotifications, this, std::placeholders::_1, std::placeholders::_2));

	overrideDefaultQuickchats_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_overrideDefaultQuickchats, this, std::placeholders::_1, std::placeholders::_2));

	blockDefaultQuickchats_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_blockDefaultQuickchats, this, std::placeholders::_1, std::placeholders::_2));

	useCustomChatTimeoutMsg_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_useCustomChatTimeoutMsg, this, std::placeholders::_1, std::placeholders::_2));

	customChatTimeoutMsg_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_customChatTimeoutMsg, this, std::placeholders::_1, std::placeholders::_2));
}
