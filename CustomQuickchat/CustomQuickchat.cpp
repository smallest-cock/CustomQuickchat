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
	auto enabled_cvar =						RegisterCvar_Bool(Cvars::enabled,						true);
	auto enableSTTNotifications_cvar =		RegisterCvar_Bool(Cvars::enableSTTNotifications,		true);
	auto overrideDefaultQuickchats_cvar =	RegisterCvar_Bool(Cvars::overrideDefaultQuickchats,		true);
	auto blockDefaultQuickchats_cvar =		RegisterCvar_Bool(Cvars::blockDefaultQuickchats,		false);
	auto searchForPyInterpreter_cvar =		RegisterCvar_Bool(Cvars::searchForPyInterpreter,		false);
	auto autoDetectInterpreterPath_cvar =	RegisterCvar_Bool(Cvars::autoDetectInterpreterPath,		true);
	auto disablePostMatchQuickchats_cvar =	RegisterCvar_Bool(Cvars::disablePostMatchQuickchats,	false);
	auto disableChatTimeout_cvar =			RegisterCvar_Bool(Cvars::disableChatTimeout,			true);
	auto useCustomChatTimeoutMsg_cvar =		RegisterCvar_Bool(Cvars::useCustomChatTimeoutMsg,		false);
	auto removeTimestamps_cvar =			RegisterCvar_Bool(Cvars::removeTimestamps,				true);

	// numbers
	auto sequenceTimeWindow_cvar =			RegisterCvar_Number(Cvars::sequenceTimeWindow,		1.1,	true, 0,	10);
	auto minBindingDelay_cvar =				RegisterCvar_Number(Cvars::minBindingDelay,			0.05,	true, 0,	1);
	auto beginSpeechTimeout_cvar =			RegisterCvar_Number(Cvars::beginSpeechTimeout,		3,		true, 1.5,	10);
	auto notificationDuration_cvar =		RegisterCvar_Number(Cvars::notificationDuration,	3,		true, 1.5,	10);
	auto speechProcessingTimeout_cvar =		RegisterCvar_Number(Cvars::speechProcessingTimeout,	10,		true, 3,	500);

	// strings
	auto pythonInterpreterPath_cvar =		RegisterCvar_String(Cvars::pythonInterpreterPath,	"");
	auto customChatTimeoutMsg_cvar =		RegisterCvar_String(Cvars::customChatTimeoutMsg,	"Wait [Time] seconds lil bro");


	// cvar change callbacks
	enabled_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enabled,
		this, std::placeholders::_1, std::placeholders::_2));
	
	enableSTTNotifications_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enableSTTNotifications,
		this, std::placeholders::_1, std::placeholders::_2));
	
	searchForPyInterpreter_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_searchForPyInterpreter,
		this, std::placeholders::_1, std::placeholders::_2));

	autoDetectInterpreterPath_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_autoDetectInterpreterPath,
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

	RegisterCommand(Cvars::toggleEnabled,		std::bind(&CustomQuickchat::cmd_toggleEnabled, this, std::placeholders::_1));
	RegisterCommand(Cvars::showPathDirectories, std::bind(&CustomQuickchat::cmd_showPathDirectories, this, std::placeholders::_1));
	RegisterCommand(Cvars::listBindings,		std::bind(&CustomQuickchat::cmd_listBindings, this, std::placeholders::_1));
	RegisterCommand(Cvars::exitToMainMenu,		std::bind(&CustomQuickchat::cmd_exitToMainMenu, this, std::placeholders::_1));
	RegisterCommand(Cvars::forfeit,				std::bind(&CustomQuickchat::cmd_forfeit, this, std::placeholders::_1));
	RegisterCommand(Cvars::test,				std::bind(&CustomQuickchat::cmd_test, this, std::placeholders::_1));


	// ======================================= hooks ==========================================

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::KeyPressed,
		std::bind(&CustomQuickchat::Event_KeyPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::ChatPresetPressed,
		std::bind(&CustomQuickchat::Event_ChatPresetPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::ApplyChatSpamFilter,
		std::bind(&CustomQuickchat::Event_ApplyChatSpamFilter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::NotifyChatDisabled,
		std::bind(&CustomQuickchat::Event_NotifyChatDisabled, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::OnChatMessage,
		std::bind(&CustomQuickchat::Event_OnChatMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::PushMenu,
		std::bind(&CustomQuickchat::Event_PushMenu, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::PopMenu,
		std::bind(&CustomQuickchat::Event_PopMenu, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventPost(Events::LoadingScreenStart, std::bind(&CustomQuickchat::Event_LoadingScreenStart, this, std::placeholders::_1));

	gameWrapper->HookEventPost(Events::MatchEnded,			[this](std::string eventName) { matchEnded = true; });
	gameWrapper->HookEventPost(Events::EnterStartState,		[this](std::string eventName) { inGameEvent = true; });

	gameWrapper->HookEvent(Events::SendChatPresetMessage, [this](std::string eventName) {

		// reset/update data for all bindings
		lastBindingActivated = std::chrono::steady_clock::now();
		ResetAllFirstButtonStates();
		
		});


	// ========================================================================================

	
	// other init
	InitStuffOnLoad();


	// search for python interpreter path (if neccessary)
	DELAY(2.0f,
		onLoadComplete = true;

		auto searchForPyInterpreter_cvar = GetCvar(Cvars::searchForPyInterpreter);
		if (!searchForPyInterpreter_cvar) return;
		bool searchForPyInterpreter = searchForPyInterpreter_cvar.getBoolValue();

		LOG("[onLoad] searchForPyInterpreter: {}", searchForPyInterpreter);

		// find & store filepath to pythonw.exe
		if (searchForPyInterpreter)
		{
			auto autoDetectInterpreterPath_cvar = GetCvar(Cvars::autoDetectInterpreterPath);
			if (!autoDetectInterpreterPath_cvar) return;

			if (autoDetectInterpreterPath_cvar.getBoolValue())
			{
				GetOutputOfWherePythonw();

				DELAY(2.0f,
					pyInterpreter = findPythonInterpreter();
				);
			}
			else
			{
				pyInterpreter = findPythonInterpreter();
			}
		}
	);


	LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload()
{
	// just to make sure any unsaved changes are saved before exiting
	WriteBindingsToJson();
}