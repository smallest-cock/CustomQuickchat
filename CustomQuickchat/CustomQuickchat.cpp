#include "pch.h"
#include "CustomQuickchat.h"


BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_THREADED)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


void CustomQuickchat::onLoad()
{
	_globalCvarManager = cvarManager;


	// init globals
	Instances.InitGlobals();
	if (!Instances.CheckGlobals()) return;


	// other init
	InitStuffOnLoad();


	// ====================================== cvars ===========================================

	// bools
	auto enabled_cvar =						RegisterCvar_Bool(Cvars::enabled,					true);
	auto enableSTTNotifications_cvar =		RegisterCvar_Bool(Cvars::enableSTTNotifications,	true);
	auto overrideDefaultQuickchats_cvar =	RegisterCvar_Bool(Cvars::overrideDefaultQuickchats,	true);
	auto blockDefaultQuickchats_cvar =		RegisterCvar_Bool(Cvars::blockDefaultQuickchats,	false);
	auto searchForPyInterpreter_cvar =		RegisterCvar_Bool(Cvars::searchForPyInterpreter,	false);
	auto autoDetectInterpreterPath_cvar =	RegisterCvar_Bool(Cvars::autoDetectInterpreterPath,	true);


	// numbers
	RegisterCvar_Number(Cvars::sequenceTimeWindow,			1.1,	true, 0,	10);
	RegisterCvar_Number(Cvars::beginSpeechTimeout,			3,		true, 1.5,	10);
	RegisterCvar_Number(Cvars::notificationDuration,		3,		true, 1.5,	10);
	RegisterCvar_Number(Cvars::speechProcessingTimeout,		10,		true, 3,	500);


	// strings
	RegisterCvar_String(Cvars::pythonInterpreterPath,	"");


	
	// cvar change callbacks
	enabled_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enabled,
		this, std::placeholders::_1, std::placeholders::_2));
	
	enableSTTNotifications_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_enableSTTNotifications,
		this, std::placeholders::_1, std::placeholders::_2));
	
	autoDetectInterpreterPath_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_autoDetectInterpreterPath,
		this, std::placeholders::_1, std::placeholders::_2));

	overrideDefaultQuickchats_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_overrideDefaultQuickchats,
		this, std::placeholders::_1, std::placeholders::_2));

	blockDefaultQuickchats_cvar.addOnValueChanged(std::bind(&CustomQuickchat::changed_blockDefaultQuickchats,
		this, std::placeholders::_1, std::placeholders::_2));



	// load previous saved cvar values from .cfg file
	cvarManager->loadCfg(cfgPath.stem().string());		// cfgPath.stem().string() should just evaluate to 'customQuickchat'


	// ===================================== commands =========================================

	RegisterCommand(Cvars::toggleEnabled,		std::bind(&CustomQuickchat::cmd_toggleEnabled, this, std::placeholders::_1));
	RegisterCommand(Cvars::showPathDirectories, std::bind(&CustomQuickchat::cmd_showPathDirectories, this, std::placeholders::_1));
	RegisterCommand(Cvars::listBindings,		std::bind(&CustomQuickchat::cmd_listBindings, this, std::placeholders::_1));
	RegisterCommand(Cvars::test,				std::bind(&CustomQuickchat::cmd_test, this, std::placeholders::_1));


	// ======================================= hooks ==========================================

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::KeyPressed,
		std::bind(&CustomQuickchat::Event_KeyPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::ChatPresetPressed,
		std::bind(&CustomQuickchat::Event_ChatPresetPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::ApplyChatSpamFilter,
		std::bind(&CustomQuickchat::Event_ApplyChatSpamFilter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	

	// ========================================================================================

	
	DELAY(2.0f,
		auto searchForPyInterpreter_cvar = GetCvar(Cvars::searchForPyInterpreter);
		if (!searchForPyInterpreter_cvar || !searchForPyInterpreter_cvar.getBoolValue()) return;

		// find & store filepath to pythonw.exe
		pyInterpreter = findPythonInterpreter();
	);
			


	onLoadComplete = true;
	LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload()
{
	// just to make sure any unsaved changes are saved before exiting
	WriteBindingsToJson();

	// save all CVar values to .cfg file
	cvarManager->backupCfg(cfgPath.string());

	Files::FilterLinesInFile(cfgPath, Cvars::prefix);
}