#include "pch.h"
#include "CustomQuickchat.hpp"
#include "bakkesmod/wrappers/cvarwrapper.h"
#include "Cvars.hpp"
#include "Macros.hpp"
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
	sequenceTimeWindow_cvar.addOnValueChanged(
	    [this](std::string oldVal, CVarWrapper updatedCvar) { m_bindingManager.setSequenceMaxTimeWindow(updatedCvar.getFloatValue()); });

	enabled_cvar.addOnValueChanged(
	    [this](std::string oldVal, CVarWrapper updatedCvar)
	    {
		    std::string msg = std::format("Custom quickchats turned {}", updatedCvar.getBoolValue() ? "ON" : "OFF");
		    GAME_THREAD_EXECUTE({ Instances.SpawnNotification("Custom Quickchat", msg, 3); }, msg);
	    });

	overrideDefaultQuickchats_cvar.addOnValueChanged(
	    [this](std::string oldVal, CVarWrapper updatedCvar)
	    {
		    bool overrideDefaultQuickchats = updatedCvar.getBoolValue();

		    // there can be only one...
		    if (overrideDefaultQuickchats)
		    {
			    auto blockDefaultQuickchats_cvar = getCvar(Cvars::blockDefaultQuickchats);
			    if (!blockDefaultQuickchats_cvar)
				    return;

			    blockDefaultQuickchats_cvar.setValue(false);
		    }
	    });

	blockDefaultQuickchats_cvar.addOnValueChanged(
	    [this](std::string oldVal, CVarWrapper updatedCvar)
	    {
		    bool blockDefaultQuickchats = updatedCvar.getBoolValue();

		    // there can be only one...
		    if (blockDefaultQuickchats)
		    {
			    auto overrideDefaultQuickchats_cvar = getCvar(Cvars::overrideDefaultQuickchats);
			    if (!overrideDefaultQuickchats_cvar)
				    return;

			    overrideDefaultQuickchats_cvar.setValue(false);
		    }
	    });

	useCustomChatTimeoutMsg_cvar.addOnValueChanged([this](std::string oldVal, CVarWrapper updatedCvar)
	    { GAME_THREAD_EXECUTE({ Instances.SetChatTimeoutMsg(getChatTimeoutMsg()); }); });

	customChatTimeoutMsg_cvar.addOnValueChanged(
	    [this](std::string oldVal, CVarWrapper updatedCvar)
	    {
		    if (!*m_useCustomChatTimeoutMsg)
			    return;

		    std::string newMsg = updatedCvar.getStringValue();
		    GAME_THREAD_EXECUTE({ Instances.SetChatTimeoutMsg(newMsg); }, newMsg);
	    });
}
