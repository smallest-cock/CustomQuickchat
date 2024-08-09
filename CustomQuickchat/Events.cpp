#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::Event_KeyPressed(ActorWrapper caller, void* params, std::string eventName)
{
	UGameViewportClient_TA_execHandleKeyPress_Params* keyPressData = reinterpret_cast<UGameViewportClient_TA_execHandleKeyPress_Params*>(params);
	if (!keyPressData) return;

	std::string keyName = keyPressData->Key.ToString();
	uint8_t keyEventType = keyPressData->EventType;

	if (keyEventType == InputEvent::Pressed)
	{
		keyStates[keyName] = true;		// update key state

		// check if any bindings triggered
		for (Binding binding : Bindings)
		{
			if (possibleBindingTypes[binding.typeNameIndex] == "button combination")
			{
				// skip if no buttons for the binding
				if (binding.buttonNameIndexes.empty()) { continue; }

				std::vector<std::string> args;
				for (int buttonIndex : binding.buttonNameIndexes) {
					args.push_back(possibleKeyNames[buttonIndex]);
				}

				if (Combine(args)) {
					PerformBindingAction(binding);
					return;
				}
			}
			else if (possibleBindingTypes[binding.typeNameIndex] == "button sequence")
			{
				// skip if less than 2 buttons for the binding
				if (binding.buttonNameIndexes.size() < 2) { continue; }

				if (Sequence(possibleKeyNames[binding.buttonNameIndexes[0]], possibleKeyNames[binding.buttonNameIndexes[1]])) {
					PerformBindingAction(binding);
					return;
				}
			}
		}
	}
	else if (keyEventType == InputEvent::Released)
	{
		keyStates[keyName] = false;		// update key state
	}
}


void CustomQuickchat::Event_ChatPresetPressed(ActorWrapper caller, void* params, std::string eventName)
{
	AGFxHUD_TA_execChatPreset_Params* Params = reinterpret_cast<AGFxHUD_TA_execChatPreset_Params*>(params);
	if (!Params) return;

	// get cvars
	auto enabled_cvar =						cvarManager->getCvar(CvarNames::enabled);
	auto overrideDefaultQuickchats_cvar =	cvarManager->getCvar(CvarNames::overrideDefaultQuickchats);
	auto blockDefaultQuickchats_cvar =		cvarManager->getCvar(CvarNames::blockDefaultQuickchats);

	if (!enabled_cvar || !overrideDefaultQuickchats_cvar || !blockDefaultQuickchats_cvar) return;	// prolly unnecessary, idk
	if (!enabled_cvar.getBoolValue()) return;

	// block default quickchat if necessary
	if (overrideDefaultQuickchats_cvar.getBoolValue())
	{
		auto currentTime = std::chrono::steady_clock::now();

		auto blockQuickchatWindow = std::chrono::duration<double>(BLOCK_DEFAULT_QUICKCHAT_WINDOW);

		if (currentTime <= (lastCustomChatSent + blockQuickchatWindow))
		{
			Params->Index = 420;
		}
	}
	else if (blockDefaultQuickchats_cvar.getBoolValue())
	{
		Params->Index = 420;	// effectively blocks quickchat from propagating
	}
}


void CustomQuickchat::Event_ApplyChatSpamFilter(ActorWrapper caller, void* params, std::string eventName)
{
	APlayerController_TA* Caller = reinterpret_cast<APlayerController_TA*>(caller.memory_address);
	if (!Caller) return;

	// effectively disables chat timeout (in freeplay)
	Caller->ChatSpam.MaxValue = 420;
	Caller->ChatSpam.DecayRate = 69;
	Caller->ChatSpam.RiseAmount = 1;
}