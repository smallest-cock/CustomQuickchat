#include "pch.h"
#include "CustomQuickchat.h"


BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


std::unordered_map<std::string, bool> CustomQuickchat::keyStates;
std::unordered_map<std::string, ButtonPress> CustomQuickchat::sequenceStoredButtonPresses;

std::vector<CombinationMacro> CustomQuickchat::ComboMacros;
std::vector<SequenceMacro> CustomQuickchat::SequenceMacros;


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



	// initialize key states map
	InitKeyStates();

	// init global sequenceStoredButtonPresses to default value
	sequenceStoredButtonPresses["global"].buttonName = "poopfart";
	sequenceStoredButtonPresses["global"].pressedTime = std::chrono::steady_clock::now();


	// register CVars
	cvarManager->registerCvar("customQuickchat_chatsOn", "1", "Toggle custom quick chats on or off", true, true, 0, true, 1);

	cvarManager->registerCvar("customQuickchat_macroTimeWindow", "1.1", "Time window given for button sequence macros", true, true, 0, true, 10);




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
	
	// command to create a button sequence binding
	cvarManager->registerNotifier("customQuickchat_createSequenceMacro", [&](std::vector<std::string> args) {

		if (args.size() > 2) {
			SequenceMacro sequence;
			sequence.chat = args[1];
			for (int i = 2; i < args.size(); i++) {
				sequence.buttons.push_back(args[i]);
			}

			SequenceMacros.push_back(sequence);
		}

	}, "", 0);
	
	// command to create a button combination binding
	cvarManager->registerNotifier("customQuickchat_createComboMacro", [&](std::vector<std::string> args) {

		if (args.size() > 2) {
			CombinationMacro combo;
			combo.chat = args[1];
			for (int i = 2; i < args.size(); i++) {
				combo.buttons.push_back(args[i]);
			}

			ComboMacros.push_back(combo);
		}

	}, "", 0);




	// hooks

	// on every input event
	gameWrapper->HookEventWithCallerPost<ActorWrapper>(KeyPressedEvent, [this](ActorWrapper caller, void* params, std::string eventName) {
	
		UGameViewportClient_TA_execHandleKeyPress_Params* keyPressData = (UGameViewportClient_TA_execHandleKeyPress_Params*)params;
		if (!keyPressData) { return; }

		std::string keyName = keyPressData->Key.ToString();
		uint8_t keyEventType = keyPressData->EventType;

		// update key state
		if (keyEventType == InputEvent::Pressed) {
			keyStates[keyName] = true;

			// check if any bindings triggered
			for (CombinationMacro combo : ComboMacros) {
				std::vector<std::string> args;
				for (std::string button : combo.buttons) {
					args.push_back(button);
				}

				if (Combine(args)) {
					SendChat(combo.chat, "lobby");
					return;
				}
			}

			for (SequenceMacro sequence : SequenceMacros) {

				if (Sequence(sequence.buttons[0], sequence.buttons[1])) {
					SendChat(sequence.chat, "lobby");
					return;
				}
			}


		}
		else if (keyEventType == InputEvent::Released) {
			keyStates[keyName] = false;
		}
	});



	LOG("CustomQuickchat loaded! :)");
}
