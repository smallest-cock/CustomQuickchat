#include "pch.h"
#include "Cvars.hpp"
#include "CustomQuickchat.hpp"
#include "components/Instances.hpp"
#include "HookManager.hpp"
#ifdef USE_SPEECH_TO_TEXT
#include "components/SpeechToText.hpp"
#endif

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

#ifdef USE_SPEECH_TO_TEXT
	SpeechToText.init(singleton, gameWrapper); // needs access to singleton
#endif
	initStuffOnLoad(); // other init

	LOG("CustomQuickchat loaded! :)");
}

void CustomQuickchat::onUnload()
{
	writeBindingsToJson(); // just to make sure any unsaved changes are saved before exiting

#ifdef USE_SPEECH_TO_TEXT
	SpeechToText.onUnload();
#endif
	Hooks.unhookAllEvents(); // shouldn't be necessary but who cares about life
}

void CustomQuickchat::initCvars()
{
	// bools
	auto enabled_cvar                    = registerCvar_Bool(Cvars::enabled, true);
	auto overrideDefaultQuickchats_cvar  = registerCvar_Bool(Cvars::overrideDefaultQuickchats, true);
	auto blockDefaultQuickchats_cvar     = registerCvar_Bool(Cvars::blockDefaultQuickchats, false);
	auto disablePostMatchQuickchats_cvar = registerCvar_Bool(Cvars::disablePostMatchQuickchats, false);
	auto disableChatTimeout_cvar         = registerCvar_Bool(Cvars::disableChatTimeout, true);
	auto useCustomChatTimeoutMsg_cvar    = registerCvar_Bool(Cvars::useCustomChatTimeoutMsg, true);
	auto removeTimestamps_cvar           = registerCvar_Bool(Cvars::removeTimestamps, true);
	auto randomizeSarcasm_cvar           = registerCvar_Bool(Cvars::randomizeSarcasm, true);
	auto uncensorChats_cvar              = registerCvar_Bool(Cvars::uncensorChats, true);

	// numbers
	auto sequenceTimeWindow_cvar = registerCvar_Number(Cvars::sequenceTimeWindow, 2, true, 0, 10);
	auto minBindingDelay_cvar    = registerCvar_Number(Cvars::minBindingDelay, 0.05, true, 0, 1);

	// strings
	auto customChatTimeoutMsg_cvar = registerCvar_String(Cvars::customChatTimeoutMsg, "Wait [Time] seconds lil bro");

	// === bind to shared ptrs ===
	enabled_cvar.bindTo(m_enabled);
	overrideDefaultQuickchats_cvar.bindTo(m_overrideDefaultQuickchats);
	blockDefaultQuickchats_cvar.bindTo(m_blockDefaultQuickchats);
	disablePostMatchQuickchats_cvar.bindTo(m_disablePostMatchQuickchats);
	disableChatTimeout_cvar.bindTo(m_disableChatTimeout);
	useCustomChatTimeoutMsg_cvar.bindTo(m_useCustomChatTimeoutMsg);
	removeTimestamps_cvar.bindTo(m_removeTimestamps);
	randomizeSarcasm_cvar.bindTo(m_randomizeSarcasm);
	uncensorChats_cvar.bindTo(m_uncensorChats);

	sequenceTimeWindow_cvar.bindTo(m_sequenceTimeWindow);
	minBindingDelay_cvar.bindTo(m_minBindingDelay);

	customChatTimeoutMsg_cvar.bindTo(m_customChatTimeoutMsg);

	// === cvar change callbacks ===
	enabled_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enabled, this, std::placeholders::_1, std::placeholders::_2));

	overrideDefaultQuickchats_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_overrideDefaultQuickchats, this, std::placeholders::_1, std::placeholders::_2));

	blockDefaultQuickchats_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_blockDefaultQuickchats, this, std::placeholders::_1, std::placeholders::_2));

	useCustomChatTimeoutMsg_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_useCustomChatTimeoutMsg, this, std::placeholders::_1, std::placeholders::_2));

	customChatTimeoutMsg_cvar.addOnValueChanged(
	    std::bind(&CustomQuickchat::changed_customChatTimeoutMsg, this, std::placeholders::_1, std::placeholders::_2));
}
