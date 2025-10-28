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

		    m_bindingManager.resetState(true);
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
		    m_bindingManager.setLastBindingActivation(std::chrono::steady_clock::now());
		    m_bindingManager.resetState();
	    });

	// ========================================= hooks with caller =========================================

	Hooks.hookEvent(Events::KeyPressed,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    auto* keyPressData = reinterpret_cast<UGameViewportClient_TA_execHandleKeyPress_Params*>(Params);
		    if (!keyPressData)
			    return;

		    std::string keyName      = keyPressData->Key.ToString();
		    EInputEvent keyEventType = static_cast<EInputEvent>(keyPressData->EventType);

		    m_bindingManager.updateKeyState(keyName, keyEventType);

		    if (m_gamePaused || !m_inGameEvent || m_chatboxOpen)
			    return;

		    if (m_matchEnded && *m_disablePostMatchQuickchats)
			    return;

		    if (keyEventType == EInputEvent::IE_Pressed)
		    {
			    m_usingGamepad = keyPressData->bGamepad;
			    // LOG("Using gamepad: {}", m_usingGamepad); // can uncomment for testing purposes, otherwise it clutters up the console

			    auto triggeredBinding = m_bindingManager.processKeyPress({keyName, std::chrono::steady_clock::now()});
			    if (!triggeredBinding)
				    return;

			    performBindingAction(*triggeredBinding);
		    }
	    });

	/*
	// this method doesn't work as of RL v2.59 update :(
	Hooks.hookEvent(Events::ApplyChatSpamFilter,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
	        auto* pc = reinterpret_cast<APlayerController_TA*>(Caller.memory_address);
	        if (!pc)
	            return;

	        auto* params = reinterpret_cast<APlayerController_TA_execApplyChatSpamFilter_Params*>(Params);
	        if (!params)
	            return;

	        // effectively disables chat timeout (in freeplay)
	        pc->ChatSpam.MaxValue   = *m_disableChatTimeout ? 420 : 4; // default 4
	        pc->ChatSpam.DecayRate  = *m_disableChatTimeout ? 69 : 1;  // default 1
	        pc->ChatSpam.RiseAmount = *m_disableChatTimeout ? 1 : 1.2; // default 1.2
	    });
	*/

	Hooks.hookEvent(Events::GFxHUD_TA_ChatPreset,
	    HookType::Pre,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
		    if (!*m_enabled)
			    return;

		    auto* params = reinterpret_cast<AGFxHUD_TA_execChatPreset_Params*>(Params);
		    if (!params)
			    return;

		    // block default quickchat if necessary
		    if (*m_overrideDefaultQuickchats)
		    {
			    auto currentTime          = std::chrono::steady_clock::now();
			    auto blockQuickchatWindow = std::chrono::duration<double>(BLOCK_DEFAULT_QUICKCHAT_WINDOW);

			    if (currentTime <= (m_bindingManager.getLastBindingActivation() + blockQuickchatWindow))
				    params->Index = 420; // effectively blocks default quickchat from propagating
		    }
		    else if (*m_blockDefaultQuickchats)
			    params->Index = 420;
	    });

	Hooks.hookEvent(Events::GFxHUD_TA_NotifyChatDisabled, HookType::Pre, [this](ActorWrapper Caller, ...) { m_gamePaused = false; });

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

		    auto* caller = reinterpret_cast<APlayerController*>(Caller.memory_address);
		    if (!caller || !caller->myHUD || !caller->myHUD->IsA<AGFxHUD_TA>())
			    return;
		    auto* hud = static_cast<AGFxHUD_TA*>(caller->myHUD);

		    Instances.SetChatTimeoutMsg(getChatTimeoutMsg(), hud);
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
		    if (!*m_enabled || !*m_overrideDefaultQuickchats)
			    return;

		    if (gameWrapper->IsInFreeplay())
			    return;

		    auto* caller = reinterpret_cast<UGFxData_Chat_TA*>(Caller.memory_address);
		    if (!caller)
			    return;

		    auto* params = reinterpret_cast<UGFxData_Chat_TA_execOnPressChatPreset_Params*>(Params);
		    if (!params)
			    return;

		    apply_custom_qc_labels_to_ui(caller, params);
	    });

	/*
	// https://github.com/ThisIs0xBC/ChatUncensor/blob/83029565049415ac8ef618e28e6eb6c0149fd92a/ChatUncensor/ChatUncensor.cpp#L63
	// thx fam
	// ========================================= uncensored chats =========================================
	using ChatCensorParams = U__GFxData_Chat_TA__AddChatMessage_0x1_exec__GFxData_Chat_TA__AddChatMessage_0x1_Params;

	Hooks.hookEvent(Events::__GFxData_Chat_TA__AddChatMessage_0x1,
	    HookType::Pre,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
	        if (!*m_uncensorChats)
	            return;

	        auto* params = reinterpret_cast<ChatCensorParams*>(Params);
	        if (!params)
	            return;

	        m_censoredChatSave = params->Sanitized; // save censored FString (so we can restore it in post hook)
	        params->Sanitized  = params->instance;  // overwrite censored FString with uncensored FString, aka uncensor it

	        if (m_censoredChatSave != params->instance)
	            LOG("Uncensored chat: \"{}\" --> \"{}\"", m_censoredChatSave.ToString(), params->instance.ToString());
	    });

	Hooks.hookEvent(Events::__GFxData_Chat_TA__AddChatMessage_0x1,
	    HookType::Post,
	    [this](ActorWrapper Caller, void* Params, ...)
	    {
	        if (!*m_uncensorChats)
	            return;

	        auto* params = reinterpret_cast<ChatCensorParams*>(Params);
	        if (!params)
	            return;

	        params->Sanitized = m_censoredChatSave; // restore censored FString (prevents crashes)

	        if (m_censoredChatSave.isValid())
	            memset(&m_censoredChatSave, 0, sizeof(FString)); // clear saved censored FString
	    });
	*/

	// ====================================================================================================

	auto logChatData = [this](const UGFxData_Chat_TA_execOnChatMessage_Params& params)
	{
		{
			Helper::ScopedBannerLog log{"Chat Info"};
			LOG("{:<25} {}", "Team:", params.Team);
			LOG("{:<25} \"{}\"", "PlayerName:", params.PlayerName.ToString());
			LOG("{:<25} \"{}\"", "Message:", params.Message.ToString());
			LOG("{:<25} {}", "ChatChannel:", params.ChatChannel);
			LOG("{:<25} {}", "bLocalPlayer:", static_cast<bool>(params.bLocalPlayer));
			LOG("{:<25} {}", "MessageType:", params.MessageType);
			LOG("{:<25} \"{}\"", "TimeStamp:", params.TimeStamp.ToString());
		}
		{
			Helper::ScopedBannerLog log{"SenderId"};
			LOG("{:<25} {}", "Platform:", params.SenderId.Platform);
			LOG("{:<25} \"{}\"", "EpicAccountId:", params.SenderId.EpicAccountId.ToString());
			LOG("{:<25} {}", "Uid:", params.SenderId.Uid);
			LOG("{:<25} {}", "SplitscreenID:", params.SenderId.SplitscreenID);
		}
	};

	// when a chat is displayed
	// ... also fires on "X left the match." match notification chats
	Hooks.hookEvent(Events::GFxData_Chat_TA_OnChatMessage,
	    HookType::Pre,
	    [this, logChatData](ActorWrapper Caller, void* Params, ...)
	    {
		    auto* params = reinterpret_cast<UGFxData_Chat_TA_execOnChatMessage_Params*>(Params);
		    if (!params)
			    return;

		    // logChatData(*params); // for debug

		    if (params->Team < 0 && params->PlayerName.empty())
			    LOG("Team < 0 and PlayerName is empty. Assuming this is a match notification (wont be added to log)", params->Team);
		    else
			    LobbyInfo.handleChatMsg(*params);

		    if (*m_removeTimestamps)
		    {
			    auto* timeStampFstr = reinterpret_cast<FStringBase*>(&params->TimeStamp);
			    timeStampFstr->size = 0;
		    }
	    });
}
