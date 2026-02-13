#include "pch.h"
#include "CustomQuickchat.hpp"
#include "util/Logging.hpp"
#include "util/Macros.hpp"
#include "Events.hpp"
#include <ModUtils/util/Utils.hpp>
#include "components/LobbyInfo.hpp"

void CustomQuickchat::initHooks() {
	hookEventPost(Events::EngineShare_X_EventPreLoadMap, [this](...) {
		m_gamePaused  = false;
		m_matchEnded  = false;
		m_inGameEvent = false;
		m_chatboxOpen = false;

		m_bindingManager.resetState(true);
		LobbyInfo.clearCachedData();
	});

	hookEventPost(Events::GameEvent_Soccar_TA_EventMatchEnded, [this](...) { m_matchEnded = true; });

	// track the state of the chatbox UI
	auto setChatboxStateOpen = [this](std::string eventName) { m_chatboxOpen = true; };
	hookEventPost(Events::GFxData_Chat_TA_OpenChat, setChatboxStateOpen);
	hookEventPost(Events::GFxData_Chat_TA_OpenTeamChat, setChatboxStateOpen);
	hookEventPost(Events::GFxData_Chat_TA_OpenPartyChat, setChatboxStateOpen);
	hookEventPost(Events::GFxData_Chat_TA_ClearDistracted, [this](std::string eventName) { m_chatboxOpen = false; });

	hookEvent(Events::GFxData_Chat_TA_SendChatPresetMessage, [this](...) {
		m_bindingManager.setLastBindingActivation(std::chrono::steady_clock::now());
		m_bindingManager.resetState();
	});

	hookWithCallerPost(Events::GameViewportClient_TA_HandleKeyPress, [this](ActorWrapper Caller, void *Params, ...) {
		auto *keyPressData = reinterpret_cast<UGameViewportClient_TA_execHandleKeyPress_Params *>(Params);
		if (!keyPressData)
			return;

		std::string keyName      = keyPressData->Key.ToString();
		EInputEvent keyEventType = static_cast<EInputEvent>(keyPressData->EventType);

		m_bindingManager.updateKeyState(keyName, keyEventType);

		if (m_gamePaused || !m_inGameEvent || m_chatboxOpen)
			return;

		if (m_matchEnded && *m_disablePostMatchQuickchats)
			return;

		if (keyEventType == EInputEvent::IE_Pressed) {
			m_usingGamepad = keyPressData->bGamepad;
			DLOG("Using gamepad: {}", m_usingGamepad);

			auto triggeredBinding = m_bindingManager.processKeyPress({keyName, std::chrono::steady_clock::now()});
			if (!triggeredBinding)
				return;

			if (triggeredBinding->bConflictsWithDefaultQC)
				m_blockNextDefaultQuickchat.store(true);
			performBindingAction(*triggeredBinding);
		}
	});

	/*
	// this method doesn't work as of RL v2.59 update :(
	g_hookManager.hookEvent(Events::ApplyChatSpamFilter,
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

	hookWithCaller(Events::GFxHUD_TA_ChatPreset, [this](ActorWrapper Caller, void *Params, ...) {
		if (!*m_enabled)
			return;
		if (*m_blockDefaultQuickchats || (*m_overrideDefaultQuickchats && m_blockNextDefaultQuickchat.load())) {
			auto *params = reinterpret_cast<AGFxHUD_TA_execChatPreset_Params *>(Params);
			if (!params)
				return;
			params->Index = 420;
			m_blockNextDefaultQuickchat.store(false); // reset flag
			LOG("Blocked default quickchat in GFxHUD_TA_ChatPreset hook");
		}
	});

	hookEvent(Events::GFxHUD_TA_NotifyChatDisabled, [this](...) { m_gamePaused = false; });

	hookWithCallerPost(Events::GFxData_MenuStack_TA_PushMenu, [this](ActorWrapper Caller, void *Params, ...) {
		auto *params = reinterpret_cast<UGFxData_MenuStack_TA_execPushMenu_Params *>(Params);
		if (!params)
			return;

		if (params->MenuName == m_fnameCache.midgameMenuMovie.get(L"MidGameMenuMovie"))
			m_gamePaused = true;
	});

	hookWithCallerPost(Events::GFxData_MenuStack_TA_PopMenu, [this](ActorWrapper Caller, void *Params, ...) {
		auto *params = reinterpret_cast<UGFxData_MenuStack_TA_execPopMenu_Params *>(Params);
		if (!params)
			return;

		if (params->MenuName == m_fnameCache.midgameMenuMovie.get(L"MidGameMenuMovie"))
			m_gamePaused = false;
	});

	hookWithCallerPost(Events::PlayerController_EnterStartState, [this](ActorWrapper Caller, ...) {
		m_inGameEvent = true;

		auto *caller = reinterpret_cast<APlayerController *>(Caller.memory_address);
		if (!caller || !caller->myHUD || !caller->myHUD->IsA<AGFxHUD_TA>())
			return;
		auto *hud = static_cast<AGFxHUD_TA *>(caller->myHUD);

		setChatTimeoutMsg(getChatTimeoutMsg(), hud);
	});

	// happens after joining a match and after a binding has been changed in RL settings
	hookWithCallerPost(Events::GFxData_Controls_TA_InitUIBindings, [this](ActorWrapper Caller, ...) {
		auto *caller = reinterpret_cast<UGFxData_Controls_TA *>(Caller.memory_address);
		if (!caller)
			return;

		// wait 0.5s to allow all the UGFxData_Controls_TA::MapUIBinding(...) calls to finish
		gameWrapper->SetTimeout(
		    [this, caller](GameWrapper *gw) {
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
	hookWithCaller(Events::GFxData_Chat_TA_OnPressChatPreset, [this](ActorWrapper Caller, void *Params, ...) {
		if (!*m_enabled || !*m_overrideDefaultQuickchats)
			return;

		if (gameWrapper->IsInFreeplay())
			return;

		auto *caller = reinterpret_cast<UGFxData_Chat_TA *>(Caller.memory_address);
		if (!caller)
			return;

		auto *params = reinterpret_cast<UGFxData_Chat_TA_execOnPressChatPreset_Params *>(Params);
		if (!params)
			return;

		applyCustomQcLabelsToUi(caller, params);
	});

	/*
	// https://github.com/ThisIs0xBC/ChatUncensor/blob/83029565049415ac8ef618e28e6eb6c0149fd92a/ChatUncensor/ChatUncensor.cpp#L63
	// thx fam
	// ========================================= uncensored chats =========================================
	using ChatCensorParams = U__GFxData_Chat_TA__AddChatMessage_0x1_exec__GFxData_Chat_TA__AddChatMessage_0x1_Params;

	g_hookManager.hookEvent(Events::__GFxData_Chat_TA__AddChatMessage_0x1,
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

	g_hookManager.hookEvent(Events::__GFxData_Chat_TA__AddChatMessage_0x1,
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

	auto logChatData = [this](const UGFxData_Chat_TA_execOnChatMessage_Params &params) {
		{
			Helper::ScopedBannerLog log{"Chat Info"};
			LOG("{:<25} {}", "Team:", params.Team);
			LOG("{:<25} \"{}\"", "PlayerName:", params.PlayerName);
			LOG("{:<25} \"{}\"", "Message:", params.Message);
			LOG("{:<25} {}", "ChatChannel:", params.ChatChannel);
			LOG("{:<25} {}", "bLocalPlayer:", static_cast<bool>(params.bLocalPlayer));
			LOG("{:<25} {}", "MessageType:", params.MessageType);
			LOG("{:<25} \"{}\"", "TimeStamp:", params.TimeStamp);
		}
		{
			Helper::ScopedBannerLog log{"SenderId"};
			LOG("{:<25} {}", "Platform:", params.SenderId.Platform);
			LOG("{:<25} \"{}\"", "EpicAccountId:", params.SenderId.EpicAccountId);
			LOG("{:<25} {}", "Uid:", params.SenderId.Uid);
			LOG("{:<25} {}", "SplitscreenID:", params.SenderId.SplitscreenID);
		}
	};

	// when a chat is displayed
	// ... also fires on "X left the match." match notification chats
	hookWithCaller(Events::GFxData_Chat_TA_OnChatMessage, [this, logChatData](ActorWrapper Caller, void *Params, ...) {
		auto *params = reinterpret_cast<UGFxData_Chat_TA_execOnChatMessage_Params *>(Params);
		if (!params)
			return;

		// logChatData(*params); // for debug

		if (params->Team < 0 && params->PlayerName.empty())
			LOG("Team < 0 and PlayerName is empty. Assuming this is a match notification (wont be added to log)", params->Team);
		else
			LobbyInfo.handleChatMsg(*params);

		if (*m_removeTimestamps) {
			auto *timeStampFstr = reinterpret_cast<FStringBase *>(&params->TimeStamp);
			timeStampFstr->size = 0;
		}
	});
}
