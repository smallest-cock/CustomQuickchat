#include "pch.h"
#include "CustomQuickchat.h"


BAKKESMOD_PLUGIN(CustomQuickchat, "Custom Quickchat", plugin_version, PLUGINTYPE_THREADED)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;


std::unordered_map<std::string, bool> CustomQuickchat::keyStates;
std::unordered_map<std::string, ButtonPress> CustomQuickchat::sequenceStoredButtonPresses;

std::vector<Binding> CustomQuickchat::Bindings;
std::vector<VariationList> CustomQuickchat::Variations;

std::filesystem::path CustomQuickchat::customQuickchatFolder;
std::filesystem::path CustomQuickchat::bindingsFilePath;
std::filesystem::path CustomQuickchat::variationsFilePath;
std::filesystem::path CustomQuickchat::lobbyInfoFolder;
std::filesystem::path CustomQuickchat::lobbyInfoChatsFilePath;
std::filesystem::path CustomQuickchat::lobbyInfoRanksFilePath;

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


	// make sure JSON files are good to go, then read them to update data
	GetFilePaths();
	CheckJsonFiles();
	UpdateData();

	PreventGameFreeze();	// somewhat hacky solution, but seems to work


	// register CVars
	cvarManager->registerCvar("customQuickchat_chatsOn", "1", "Toggle custom quick chats on or off", true, true, 0, true, 1);
	cvarManager->registerCvar("customQuickchat_macroTimeWindow", "1.1", "Time window given for button sequence macros", true, true, 0, true, 10);

	
	// command to toggle custom quickchats on/off
	cvarManager->registerNotifier("customQuickchat_toggle", [&](std::vector<std::string> args) {

		CVarWrapper chatsOnCvar = cvarManager->getCvar("customQuickchat_chatsOn");
		if (!chatsOnCvar) { return; }

		bool chatsOn = chatsOnCvar.getBoolValue();
		chatsOnCvar.setValue(!chatsOn);
		chatsOn = chatsOnCvar.getBoolValue();

		LOG(chatsOn ? "<<\tcustom quickchats turned ON\t>>" : "<<\tcustom quickchats turned OFF\t>>");

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
			for (Binding binding: Bindings) {

				if (possibleBindingTypes[binding.typeNameIndex] == "button combination") {

					// skip if no buttons for the binding
					if (binding.buttonNameIndexes.empty()) { continue; }

					std::vector<std::string> args;
					for (int buttonIndex : binding.buttonNameIndexes) {
						args.push_back(possibleKeyNames[buttonIndex]);
					}

					if (Combine(args)) {
						SendChat(binding.chat, possibleChatModes[binding.chatMode]);
						return;
					}
				}
				else if (possibleBindingTypes[binding.typeNameIndex] == "button sequence") {
					
					// skip if less than 2 buttons for the binding
					if (binding.buttonNameIndexes.size() < 2) { continue; }
					
					if (Sequence(possibleKeyNames[binding.buttonNameIndexes[0]], possibleKeyNames[binding.buttonNameIndexes[1]])) {
						SendChat(binding.chat, possibleChatModes[binding.chatMode]);
						return;
					}
				}
			}

		}
		else if (keyEventType == InputEvent::Released) {
			keyStates[keyName] = false;
		}
	});



	LOG("CustomQuickchat loaded! :)");
}


void CustomQuickchat::onUnload() {
	// just to make sure any unsaved changes are saved before exiting
	WriteBindingsToJson();
}