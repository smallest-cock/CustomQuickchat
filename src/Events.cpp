#include "pch.h"
#include "HookManager.hpp"
#include "CustomQuickchat.hpp"
#include "Events.hpp"
#include "Macros.hpp"
#include "components/Instances.hpp"
#include <ModUtils/util/Utils.hpp>
#include "HookManager.hpp"
#include "components/LobbyInfo.hpp"

class HookManager Hooks{};

void CustomQuickchat::initHooks()
{
	Hooks.hookEvent(Events::LoadingScreenStart,
	    HookType::Post,
	    [this](std::string event)
	    {
		    m_gamePaused  = false;
		    m_matchEnded  = false;
		    m_inGameEvent = false;
		    m_chatboxOpen = false;

		    // reset all "pressed" buttons (to fix bug of bindings mistakenly firing bc a key's state is stuck in "pressed" mode upon
		    // joining a game/freeplay)
		    for (auto& [key, state] : m_keyStates)
			    state = false;

		    LobbyInfo.clearCachedData();
	    });

	Hooks.hookEvent(Events::MatchEnded, HookType::Post, [this](std::string eventName) { m_matchEnded = true; });

	// track the state of the chatbox UI
	auto set_chatbox_open = [this](std::string eventName) { m_chatboxOpen = true; };
	Hooks.hookEvent(Events::GFxData_Chat_TA_OpenChat, HookType::Post, set_chatbox_open);
	Hooks.hookEvent(Events::GFxData_Chat_TA_OpenTeamChat, HookType::Post, set_chatbox_open);
	Hooks.hookEvent(Events::GFxData_Chat_TA_OpenPartyChat, HookType::Post, set_chatbox_open);
	Hooks.hookEvent(Events::GFxData_Chat_TA_ClearDistracted, HookType::Post, [this](std::string eventName) { m_chatboxOpen = false; });

	Hooks.hookEvent(Events::SendChatPresetMessage,
	    HookType::Pre,
	    [this](std::string eventName)
	    {
		    // reset/update data for all bindings
		    lastBindingActivated = std::chrono::steady_clock::now();
		    ResetAllFirstButtonStates();
	    });

	// ========================================= hooks with caller =========================================

	Hooks.hookEvent(Events::KeyPressed,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    if (m_gamePaused || !m_inGameEvent || m_chatboxOpen)
			    return;

		    if (m_matchEnded)
		    {
			    auto disablePostMatchQuickchats_cvar = getCvar(Cvars::disablePostMatchQuickchats);
			    if (!disablePostMatchQuickchats_cvar || disablePostMatchQuickchats_cvar.getBoolValue())
				    return;
		    }

		    auto* keyPressData = reinterpret_cast<UGameViewportClient_TA_execHandleKeyPress_Params*>(Params);
		    if (!keyPressData)
			    return;

		    std::string keyName      = keyPressData->Key.ToString();
		    EInputEvent keyEventType = static_cast<EInputEvent>(keyPressData->EventType);

		    if (keyEventType == EInputEvent::IE_Pressed)
		    {
			    m_keyStates[keyName] = true; // update key state (for CheckCombination() to analyze a "snapshot" of all pressed buttons)

			    // update state for tracking whether user is using gamepad or pc inputs
			    using_gamepad = keyPressData->bGamepad;
			    // LOG("Using gamepad: {}", using_gamepad);      // can uncomment for testing purposes, otherwise it clutters up the console

			    ButtonPress buttonPressEvent{keyName, std::chrono::steady_clock::now()};

			    // get min binding delay
			    auto minBindingDelay_cvar = getCvar(Cvars::minBindingDelay);
			    if (!minBindingDelay_cvar)
				    return;
			    double minBindingDelay_raw = minBindingDelay_cvar.getFloatValue();
			    auto   minBindingDelay     = std::chrono::duration<double>(minBindingDelay_raw);

			    // get max sequence time window
			    auto sequenceTimeWindow_cvar = getCvar(Cvars::sequenceTimeWindow);
			    if (!sequenceTimeWindow_cvar)
				    return;
			    double sequenceTimeWindow_raw = sequenceTimeWindow_cvar.getFloatValue();
			    auto   sequenceTimeWindow     = std::chrono::duration<double>(sequenceTimeWindow_raw);

			    // check if any bindings triggered
			    for (Binding& binding : m_bindings)
			    {
				    if (binding.enabled &&
				        binding.shouldBeTriggered(
				            buttonPressEvent, m_keyStates, lastBindingActivated, epochTime, minBindingDelay, sequenceTimeWindow))
				    {
					    // reset/update data for all bindings
					    lastBindingActivated = std::chrono::steady_clock::now();
					    ResetAllFirstButtonStates();

					    // activate binding action
					    performBindingAction(binding);
					    return;
				    }
			    }
		    }
		    else if (keyEventType == EInputEvent::IE_Released)
			    m_keyStates[keyName] = false; // update key state (for CheckCombination() to analyze a "snapshot" of all pressed buttons)
	    });

	Hooks.hookEvent(Events::ApplyChatSpamFilter,
	    HookType::Post,
	    [this](ActorWrapper Caller, ...)
	    {
		    auto* pc = reinterpret_cast<APlayerController_TA*>(Caller.memory_address);
		    if (!pc)
			    return;

		    auto disableChatTimeout_cvar = getCvar(Cvars::disableChatTimeout);
		    if (!disableChatTimeout_cvar)
			    return;
		    bool disableChatTimeout = disableChatTimeout_cvar.getBoolValue();

		    // effectively disables chat timeout (in freeplay)
		    pc->ChatSpam.MaxValue   = disableChatTimeout ? 420 : 4; // default 4
		    pc->ChatSpam.DecayRate  = disableChatTimeout ? 69 : 1;  // default 1
		    pc->ChatSpam.RiseAmount = disableChatTimeout ? 1 : 1.2; // default 1.2
	    });

	Hooks.hookEvent(Events::GFxHUD_TA_ChatPreset,
	    HookType::Pre,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    auto* params = reinterpret_cast<AGFxHUD_TA_execChatPreset_Params*>(Params);
		    if (!params)
			    return;

		    // get cvars
		    auto enabled_cvar                   = getCvar(Cvars::enabled);
		    auto overrideDefaultQuickchats_cvar = getCvar(Cvars::overrideDefaultQuickchats);
		    auto blockDefaultQuickchats_cvar    = getCvar(Cvars::blockDefaultQuickchats);

		    if (!enabled_cvar || !enabled_cvar.getBoolValue())
			    return;

		    // block default quickchat if necessary
		    if (overrideDefaultQuickchats_cvar.getBoolValue())
		    {
			    auto currentTime          = std::chrono::steady_clock::now();
			    auto blockQuickchatWindow = std::chrono::duration<double>(BLOCK_DEFAULT_QUICKCHAT_WINDOW);

			    if (currentTime <= (lastBindingActivated + blockQuickchatWindow))
				    params->Index = 420; // effectively blocks default quickchat from propagating
		    }
		    else if (blockDefaultQuickchats_cvar.getBoolValue())
			    params->Index = 420;
	    });

	Hooks.hookEvent(Events::GFxHUD_TA_NotifyChatDisabled,
	    HookType::Pre,
	    [this](ActorWrapper Caller, ...)
	    {
		    m_gamePaused = false;

		    auto useCustomChatTimeoutMsg_cvar = getCvar(Cvars::useCustomChatTimeoutMsg);
		    if (!useCustomChatTimeoutMsg_cvar || !useCustomChatTimeoutMsg_cvar.getBoolValue())
			    return;

		    auto* hud = reinterpret_cast<AGFxHUD_TA*>(Caller.memory_address);
		    if (!hud)
			    return;

		    Instances.SetChatTimeoutMsg(chatTimeoutMsg, hud);
	    });

	Hooks.hookEvent(Events::PushMenu,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    auto* params = reinterpret_cast<UGFxData_MenuStack_TA_execPushMenu_Params*>(Params);
		    if (!params)
			    return;

		    if (params->MenuName.ToString() == "MidGameMenuMovie")
			    m_gamePaused = true;
	    });

	Hooks.hookEvent(Events::PopMenu,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    auto* params = reinterpret_cast<UGFxData_MenuStack_TA_execPopMenu_Params*>(Params);
		    if (!params)
			    return;

		    if (params->MenuName.ToString() == "MidGameMenuMovie")
			    m_gamePaused = false;
	    });

	Hooks.hookEvent(Events::PlayerController_EnterStartState,
	    HookType::Post,
	    [this](ActorWrapper Caller, ...)
	    {
		    m_inGameEvent = true;

		    // set chat timeout message
		    auto useCustomChatTimeoutMsg_cvar = getCvar(Cvars::useCustomChatTimeoutMsg);
		    if (!useCustomChatTimeoutMsg_cvar || !useCustomChatTimeoutMsg_cvar.getBoolValue())
			    return;

		    auto* caller = reinterpret_cast<APlayerController*>(Caller.memory_address);
		    if (!caller || !caller->myHUD || !caller->myHUD->IsA<AGFxHUD_TA>())
			    return;
		    auto* hud = static_cast<AGFxHUD_TA*>(caller->myHUD);

		    Instances.SetChatTimeoutMsg(chatTimeoutMsg, hud);
	    });

	// happens after joining a match and after a binding has been changed in RL settings
	Hooks.hookEvent(Events::InitUIBindings,
	    HookType::Post,
	    [this](ActorWrapper Caller, ...)
	    {
		    auto* caller = reinterpret_cast<UGFxData_Controls_TA*>(Caller.memory_address);
		    if (!caller)
			    return;

		    // wait 0.5s to allow all the UGFxData_Controls_TA::MapUIBinding(...) calls to finish
		    gameWrapper->SetTimeout(
		        [this, caller](GameWrapper* gw)
		        {
			        if (!caller)
				        return;
			        determineQuickchatLabels(caller);
		        },
		        0.5f);

		    DELAY(
		        0.5f,
		        {
			        if (!caller)
				        return;
			        determineQuickchatLabels(caller);
		        },
		        caller);
	    });

	// NOTE: Running this on every chat preset pressed (aka every time the quickchat ui shows up) ensures the correct group of custom
	// quickchat labels (pc vs gamepad) will be displayed. It may seem more efficient to apply chat labels to ui less often, but that
	// wouldn't account for user switching between pc & gamepad inputs
	Hooks.hookEvent(Events::OnPressChatPreset,
	    HookType::Pre,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    if (gameWrapper->IsInFreeplay())
			    return;

		    auto enabled_cvar                   = getCvar(Cvars::enabled);
		    auto overrideDefaultQuickchats_cvar = getCvar(Cvars::overrideDefaultQuickchats);
		    if (!enabled_cvar || !enabled_cvar.getBoolValue())
			    return;
		    if (!overrideDefaultQuickchats_cvar || !overrideDefaultQuickchats_cvar.getBoolValue())
			    return;

		    auto* caller = reinterpret_cast<UGFxData_Chat_TA*>(Caller.memory_address);
		    if (!caller)
			    return;

		    auto* params = reinterpret_cast<UGFxData_Chat_TA_execOnPressChatPreset_Params*>(Params);
		    if (!params)
			    return;

		    apply_custom_qc_labels_to_ui(caller, params);
	    });

	// when uncensored chat is recieved
	Hooks.hookEvent(Events::HUDBase_TA_OnChatMessage,
	    HookType::Pre,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    if (!*m_uncensorChats)
			    return;

		    auto* params = reinterpret_cast<AHUDBase_TA_execOnChatMessage_Params*>(Params);
		    if (!params)
			    return;

		    FChatMessage& msg = params->NewMsg;
		    if (msg.bPreset)
			    return;

		    auto* caller = reinterpret_cast<AHUDBase_TA*>(Caller.memory_address);
		    if (!caller)
			    return;

		    m_mostRecentUncensoredChat = msg;
	    });

	// when censored chat is displayed
	Hooks.hookEvent(Events::GFxData_Chat_TA_OnChatMessage,
	    HookType::Pre,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    auto* params = reinterpret_cast<UGFxData_Chat_TA_execOnChatMessage_Params*>(Params);
		    if (!params)
			    return;

		    if (*m_uncensorChats)
		    {
			    std::string gfxUid      = ChatMsgData::generateUid(params);
			    std::string censoredMsg = params->Message.ToString();
			    if (gfxUid == m_mostRecentUncensoredChat.uid && censoredMsg != m_mostRecentUncensoredChat.uncensoredMsg)
			    {
				    params->Message = FString::create(m_mostRecentUncensoredChat.uncensoredMsg);
				    LOG("Uncensored chat: \"{}\" --> \"{}\"",
				        Format::EscapeBraces(censoredMsg),
				        Format::EscapeBraces(m_mostRecentUncensoredChat.uncensoredMsg));
			    }
		    }

		    if (*m_removeTimestamps)
		    {
			    auto timeStampFstr  = reinterpret_cast<FStringBase*>(&params->TimeStamp);
			    timeStampFstr->size = 0;
		    }
	    });
}
