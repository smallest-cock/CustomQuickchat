#include "pch.h"
#include "CustomQuickchat.h"


BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_THREADED)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


void CustomQuickchat::onLoad()
{
	_globalCvarManager = cvarManager;

	// init RLSDK globals
	plugin::globals::Init();
	CheckGlobals();

	LOG("[Troubleshooting] 1");

	// init key states map
	InitKeyStates();

	LOG("[Troubleshooting] 2");

	pyInterpreter = findPythonInterpreter();	// find & store filepath to pythonw.exe

	LOG("[Troubleshooting] 3");

	// set global sequenceStoredButtonPresses to default value
	sequenceStoredButtonPresses["global"].buttonName = "poopfart";
	sequenceStoredButtonPresses["global"].pressedTime = std::chrono::steady_clock::now();

	LOG("[Troubleshooting] 4");

	// make sure JSON files are good to go, then read them to update data
	GetFilePaths();
	LOG("[Troubleshooting] 5");
	CheckJsonFiles();
	LOG("[Troubleshooting] 6");
	UpdateData();
	LOG("[Troubleshooting] 7");
	ClearTranscriptionJson();
	LOG("[Troubleshooting] 8");


	PreventGameFreeze();	// hacky solution, but seems to work
	LOG("[Troubleshooting] 9");


	// execute this stuff in the main thread
	gameWrapper->Execute([this](GameWrapper* gw) {

		// register CVars
		cvarManager->registerCvar("customQuickchat_chatsOn", "1", "Toggle custom quick chats on or off", true, true, 0, true, 1);
		cvarManager->registerCvar("customQuickchat_speechToTextNotificationsOn", "1", "Toggle speech-to-text notifications on or off", true, true, 0, true, 1);
		cvarManager->registerCvar("customQuickchat_macroTimeWindow", "1.1", "Time window given for button sequence macros", true, true, 0, true, 10);
		cvarManager->registerCvar("customQuickchat_waitForSpeechTimeout", "3", "timeout for starting speech", true, true, 1.5, true, 10);
		cvarManager->registerCvar("customQuickchat_popupNotificationDuration", "3", "how long a popup notification will stay on the screen", true, true, 1.5, true, 10);
		cvarManager->registerCvar("customQuickchat_processSpeechTimeout", "10", "timeout for processing speech", true, true, 3, true, 500);

		LOG("[Troubleshooting] 10");


		// load previous saved CVar values from .cfg file
		std::string cfgPathStr = cfgPath.string();
		LOG("cfgPath: {}", cfgPathStr);
		cvarManager->loadCfg(cfgPathStr);

		LOG("[Troubleshooting] 11");


		// do a background speech-to-text test run after onLoad, to help prevent error on first real attempt
		gameWrapper->SetTimeout([this](GameWrapper* gw) {

			LOG("[Troubleshooting] finna do the background speech-to-text test run after onLoad, to help prevent error ...");

			//StartSpeechToText("lobby", "", true);	// do a dummy test run of speech-to-text
			StartSpeechToText("lobby", "", true, true);  // calibrate mic energy threshold

			LOG("[Troubleshooting] did the background speech-to-text test run after onLoad, to help prevent error");

		}, 5);	// wait 5s to give threaded onLoad some time to finish		<---- maybe causing crash for some ppl? Maybe sumn to do with using invalid filepaths?

		LOG("[Troubleshooting] 12");

	});
	
	LOG("[Troubleshooting] 13");

	// command to toggle custom quickchats on/off
	cvarManager->registerNotifier("customQuickchat_toggle", [&](std::vector<std::string> args) {

		CVarWrapper chatsOnCvar = cvarManager->getCvar("customQuickchat_chatsOn");
		if (!chatsOnCvar) { return; }

		bool chatsOn = chatsOnCvar.getBoolValue();
		chatsOnCvar.setValue(!chatsOn);
		chatsOn = chatsOnCvar.getBoolValue();

		LOG(chatsOn ? "<<\tcustom quickchats turned ON\t>>" : "<<\tcustom quickchats turned OFF\t>>");

	}, "", 0);
	
	
	// test stuff... for new features?
	cvarManager->registerNotifier("customQuickchat_testShit", [&](std::vector<std::string> args) {
		TestShit();
	}, "", 0);
	


	// ------------------------------------------------ hooks -------------------------------------------------

	// on every keypress
	gameWrapper->HookEventWithCallerPost<ActorWrapper>(KeyPressedEvent,
		std::bind(&CustomQuickchat::HandleKeyPress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));



	LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload() {
	// just to make sure any unsaved changes are saved before exiting
	WriteBindingsToJson();

	// save all CVar values to .cfg file
	cvarManager->backupCfg(cfgPath.string());

	FilterLinesInFile(cfgPath, "customQuickchat_");
}