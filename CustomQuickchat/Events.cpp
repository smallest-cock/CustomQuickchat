#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::Event_KeyPressed(ActorWrapper caller, void* params, std::string eventName)
{
	if (gamePaused || !inGameEvent) return;

	if (matchEnded)
	{
		auto disablePostMatchQuickchats_cvar = GetCvar(Cvars::disablePostMatchQuickchats);
		if (!disablePostMatchQuickchats_cvar || disablePostMatchQuickchats_cvar.getBoolValue()) return;
	}

	UGameViewportClient_TA_execHandleKeyPress_Params* keyPressData = reinterpret_cast<UGameViewportClient_TA_execHandleKeyPress_Params*>(params);
	if (!keyPressData) return;

	std::string keyName = keyPressData->Key.ToString();
	EInputEvent keyEventType = static_cast<EInputEvent>(keyPressData->EventType);

	if (keyEventType == EInputEvent::IE_Pressed)
	{
		keyStates[keyName] = true;		// update key state (for CheckCombination() to analyze a "snapshot" of all pressed buttons)

		ButtonPress buttonPressEvent{ keyName, std::chrono::steady_clock::now() };

		// get min binding delay
		auto minBindingDelay_cvar = GetCvar(Cvars::minBindingDelay);
		if (!minBindingDelay_cvar) return;
		double minBindingDelay_raw = minBindingDelay_cvar.getFloatValue();
		auto minBindingDelay = std::chrono::duration<double>(minBindingDelay_raw);

		// get max sequence time window
		auto sequenceTimeWindow_cvar = GetCvar(Cvars::sequenceTimeWindow);
		if (!sequenceTimeWindow_cvar) return;
		double sequenceTimeWindow_raw = sequenceTimeWindow_cvar.getFloatValue();
		auto sequenceTimeWindow = std::chrono::duration<double>(sequenceTimeWindow_raw);


		// check if any bindings triggered
		for (Binding& binding : Bindings)
		{
			if (binding.ShouldBeTriggered(buttonPressEvent, keyStates, lastBindingActivated, epochTime, minBindingDelay, sequenceTimeWindow))
			{
				// reset/update data for all bindings
				lastBindingActivated = std::chrono::steady_clock::now();
				ResetAllFirstButtonStates();

				// activate binding action
				PerformBindingAction(binding);
				return;
			}
		}
	}
	else if (keyEventType == EInputEvent::IE_Released)
		keyStates[keyName] = false;		// update key state (for CheckCombination() to analyze a "snapshot" of all pressed buttons)
}


void CustomQuickchat::Event_ChatPresetPressed(ActorWrapper caller, void* params, std::string eventName)
{
	AGFxHUD_TA_execChatPreset_Params* Params = reinterpret_cast<AGFxHUD_TA_execChatPreset_Params*>(params);
	if (!Params) return;

	// get cvars
	auto enabled_cvar =						GetCvar(Cvars::enabled);
	auto overrideDefaultQuickchats_cvar =	GetCvar(Cvars::overrideDefaultQuickchats);
	auto blockDefaultQuickchats_cvar =		GetCvar(Cvars::blockDefaultQuickchats);

	if (!enabled_cvar || !overrideDefaultQuickchats_cvar || !blockDefaultQuickchats_cvar) return;	// prolly unnecessary, idk
	if (!enabled_cvar.getBoolValue()) return;

	// block default quickchat if necessary
	if (overrideDefaultQuickchats_cvar.getBoolValue())
	{
		auto currentTime = std::chrono::steady_clock::now();

		auto blockQuickchatWindow = std::chrono::duration<double>(BLOCK_DEFAULT_QUICKCHAT_WINDOW);

		if (currentTime <= (lastBindingActivated + blockQuickchatWindow))
		{
			Params->Index = 420;	// effectively blocks default quickchat from propagating
		}
	}
	else if (blockDefaultQuickchats_cvar.getBoolValue())
	{
		Params->Index = 420;	// effectively blocks default quickchat from propagating
	}
}


void CustomQuickchat::Event_ApplyChatSpamFilter(ActorWrapper caller, void* params, std::string eventName)
{
	APlayerController_TA* pc = reinterpret_cast<APlayerController_TA*>(caller.memory_address);
	if (!pc) return;

	auto disableChatTimeout_cvar = GetCvar(Cvars::disableChatTimeout);
	if (!disableChatTimeout_cvar) return;
	bool disableChatTimeout = disableChatTimeout_cvar.getBoolValue();

	// effectively disables chat timeout (in freeplay)
	pc->ChatSpam.MaxValue =	disableChatTimeout ? 420 : 4;		// default 4
	pc->ChatSpam.DecayRate = disableChatTimeout ? 69 : 1;		// default 1
	pc->ChatSpam.RiseAmount = disableChatTimeout ? 1 : 1.2;		// default 1.2
}


void CustomQuickchat::Event_NotifyChatDisabled(ActorWrapper caller, void* params, std::string eventName)
{
	gamePaused = false;

	AGFxHUD_TA* hud = reinterpret_cast<AGFxHUD_TA*>(caller.memory_address);
	if (!hud) return;

	Instances.SetChatTimeoutMsg(chatTimeoutMsg, hud, caller.memory_address);
}


// remove chat timestamps
void CustomQuickchat::Event_OnChatMessage(ActorWrapper caller, void* params, std::string eventName)
{
	auto removeTimestamps_cvar = GetCvar(Cvars::removeTimestamps);
	if (!removeTimestamps_cvar || !removeTimestamps_cvar.getBoolValue()) return;

	FGFxChatMessage* Params = reinterpret_cast<FGFxChatMessage*>(params);
	if (!Params) return;
	Params->TimeStamp = "";
}


void CustomQuickchat::Event_PushMenu(ActorWrapper caller, void* params, std::string eventName)
{
	UGFxData_MenuStack_TA_execPushMenu_Params* Params = reinterpret_cast<UGFxData_MenuStack_TA_execPushMenu_Params*>(params);
	if (!Params) return;

	if (Params->MenuName.ToString() == "MidGameMenuMovie")
		gamePaused = true;
}


void CustomQuickchat::Event_PopMenu(ActorWrapper caller, void* params, std::string eventName)
{
	UGFxData_MenuStack_TA_execPopMenu_Params* Params = reinterpret_cast<UGFxData_MenuStack_TA_execPopMenu_Params*>(params);
	if (!Params) return;

	if (Params->MenuName.ToString() == "MidGameMenuMovie")
		gamePaused = false;
}


void CustomQuickchat::Event_LoadingScreenStart(std::string eventName)
{
	gamePaused = false;
	matchEnded = false;
	inGameEvent = false;

	// reset all "pressed" buttons (to fix bug of bindings mistakenly firing bc a key's state is stuck in "pressed" mode upon joining a game/freeplay)
	for (auto& [key, state] : keyStates)
	{
		state = false;
	}
}
