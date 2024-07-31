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
	
	// find & store filepath to pythonw.exe
	pyInterpreter = findPythonInterpreter();

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
	auto enabledCvar = cvarManager->registerCvar(CvarNames::enabled, "1", "Toggle custom quick chats on or off", true, true, 0, true, 1);
	auto enableSTTNotificationsCvar = cvarManager->registerCvar(CvarNames::enableSTTNotifications, "1", "Toggle speech-to-text notifications on or off", true, true, 0, true, 1);

	// numbers
	cvarManager->registerCvar(CvarNames::sequenceTimeWindow,		"1.1", "Time window given for button sequence macros", true, true, 0, true, 10);
	cvarManager->registerCvar(CvarNames::beginSpeechTimeout,		"3", "timeout for starting speech", true, true, 1.5, true, 10);
	cvarManager->registerCvar(CvarNames::notificationDuration,		"3", "how long a popup notification will stay on the screen", true, true, 1.5, true, 10);
	cvarManager->registerCvar(CvarNames::speechProcessingTimeout,	"10", "timeout for processing speech", true, true, 3, true, 500);


	// cvar change callbacks
	enabledCvar.addOnValueChanged(std::bind(&CustomQuickchat::enabled_Changed, this, std::placeholders::_1, std::placeholders::_2));
	enableSTTNotificationsCvar.addOnValueChanged(std::bind(&CustomQuickchat::enableSTTNotifications_Changed, this, std::placeholders::_1, std::placeholders::_2));


	// load previous saved cvar values from .cfg file
	cvarManager->loadCfg(cfgPath.string());		// cfgPath.stem().string() would just evaluate to 'customQuickchat'


	// ===================================== commands =========================================

	cvarManager->registerNotifier(CvarNames::toggleEnabled,		std::bind(&CustomQuickchat::toggleEnabled, this, std::placeholders::_1), "", PERMISSION_ALL);
	cvarManager->registerNotifier(CvarNames::test,				std::bind(&CustomQuickchat::test, this, std::placeholders::_1), "", PERMISSION_ALL);
	

	// ======================================= hooks ==========================================

	gameWrapper->HookEventWithCallerPost<ActorWrapper>(Events::KeyPressed,
		std::bind(&CustomQuickchat::Event_KeyPressed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


	// ========================================================================================



	LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload()
{
	// just to make sure any unsaved changes are saved before exiting
	WriteBindingsToJson();

	// save all CVar values to .cfg file
	cvarManager->backupCfg(cfgPath.string());

	Files::FilterLinesInFile(cfgPath, "customQuickchat_");
}