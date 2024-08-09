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


	// ----------------- other init ------------------
	
	PreventGameFreeze();	// hacky solution, but seems to work

	InitKeyStates();

	// set global sequenceStoredButtonPresses to default value
	sequenceStoredButtonPresses["global"].buttonName = "poopfart";
	sequenceStoredButtonPresses["global"].pressedTime = std::chrono::steady_clock::now();

	// make sure JSON files are good to go, then read them to update data
	GetFilePaths();
	CheckJsonFiles();
	UpdateData();
	ClearTranscriptionJson();


	// ====================================== cvars ===========================================

	// bools
	auto enabled_cvar = cvarManager->registerCvar(CvarNames::enabled,
		"1", "Toggle custom quick chats on or off", true, true, 0, true, 1);
	
	auto enableSTTNotifications_cvar = cvarManager->registerCvar(CvarNames::enableSTTNotifications,
		"1", "Toggle speech-to-text notifications on or off", true, true, 0, true, 1);
	
	auto autoDetectInterpreterPath_cvar = cvarManager->registerCvar(CvarNames::autoDetectInterpreterPath,
		"1", "Automatically detect python interpreter filepath", true, true, 0, true, 1);

	auto overrideDefaultQuickchats_cvar = cvarManager->registerCvar(CvarNames::overrideDefaultQuickchats,
		"1", "override default quickchat with the custom one if they share the same binding", true, true, 0, true, 1);

	auto blockDefaultQuickchats_cvar = cvarManager->registerCvar(CvarNames::blockDefaultQuickchats,
		"0", "block default quickchats (without unbinding them)", true, true, 0, true, 1);

	// numbers
	cvarManager->registerCvar(CvarNames::sequenceTimeWindow,		"1.1", "Time window given for button sequence macros", true, true, 0, true, 10);
	cvarManager->registerCvar(CvarNames::beginSpeechTimeout,		"3", "timeout for starting speech", true, true, 1.5, true, 10);
	cvarManager->registerCvar(CvarNames::notificationDuration,		"3", "how long a popup notification will stay on the screen", true, true, 1.5, true, 10);
	cvarManager->registerCvar(CvarNames::speechProcessingTimeout,	"10", "timeout for processing speech", true, true, 3, true, 500);

	// strings
	cvarManager->registerCvar(CvarNames::pythonInterpreterPath, "", "filepath to python interpreter");


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

	cvarManager->registerNotifier(CvarNames::toggleEnabled,
		std::bind(&CustomQuickchat::cmd_toggleEnabled, this, std::placeholders::_1), "", PERMISSION_ALL);
	
	cvarManager->registerNotifier(CvarNames::showPathDirectories,
		std::bind(&CustomQuickchat::cmd_showPathDirectories, this, std::placeholders::_1), "", PERMISSION_ALL);

	cvarManager->registerNotifier(CvarNames::listBindings,
		std::bind(&CustomQuickchat::cmd_listBindings, this, std::placeholders::_1), "", PERMISSION_ALL);

	cvarManager->registerNotifier(CvarNames::test,
		std::bind(&CustomQuickchat::cmd_test, this, std::placeholders::_1), "", PERMISSION_ALL);
	

	// ======================================= hooks ==========================================

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::KeyPressed,
		std::bind(&CustomQuickchat::Event_KeyPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCaller<ActorWrapper>(Events::ChatPresetPressed,
		std::bind(&CustomQuickchat::Event_ChatPresetPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::ApplyChatSpamFilter,
		std::bind(&CustomQuickchat::Event_ApplyChatSpamFilter, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	

	// ========================================================================================

	
	// find & store filepath to pythonw.exe
	pyInterpreter = findPythonInterpreter();
			


	onLoadComplete = true;
	LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload()
{
	// just to make sure any unsaved changes are saved before exiting
	WriteBindingsToJson();

	// save all CVar values to .cfg file
	cvarManager->backupCfg(cfgPath.string());

	Files::FilterLinesInFile(cfgPath, CvarNames::prefix);
}