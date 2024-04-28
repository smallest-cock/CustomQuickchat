#include "pch.h"
#include "CustomQuickchat.h"


BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void CustomQuickchat::onLoad()
{
	_globalCvarManager = cvarManager;

	plugin::globals::Init();

	if (!GObjects || !GNames || !AreGObjectsValid() || !AreGNamesValid()) {
		LOG("(onLoad) Error: RLSDK classes are wrong... need to fix them :(");
		LOG(std::to_string(!GObjects) + ", " + std::to_string(!GNames));
		return;
	}
	else {
		LOG("Globals Initailized");
	}


	// register CVars
	cvarManager->registerCvar("customQuickchat_chatsOn", "1", "Toggle custom quick chats on or off", true, true, 0, true, 1);


	// command to send a chat
	cvarManager->registerNotifier("customQuickchat", [&](std::vector<std::string> args) {

		int numArgs = args.size();

		if (numArgs == 1) {
			LOG("no chat provided ....");
		}
		else if (numArgs == 2) {
			SendChat(args[1], "lobby");		// default chat mode is lobby
		}
		else if (numArgs == 3) {
			SendChat(args[1], args[2]);
		}


	}, "", 0);
	
	// command to toggle custom quickchats on/off
	cvarManager->registerNotifier("customQuickchat_toggle", [&](std::vector<std::string> args) {

		CVarWrapper chatsOnCvar = cvarManager->getCvar("customQuickchat_chatsOn");
		if (!chatsOnCvar) { return; }

		bool chatsOn = chatsOnCvar.getBoolValue();
		chatsOnCvar.setValue(!chatsOn);
		chatsOn = chatsOnCvar.getBoolValue();

		LOG(chatsOn ? "<<\tcustom quickchats turned ON\t>>" : "<<\tcustom quickchats turned OFF\t>>");

	}, "", 0);



	LOG("CustomQuickchat loaded! :)");
}
