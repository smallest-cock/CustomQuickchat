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