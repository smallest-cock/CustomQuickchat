#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::Event_KeyPressed(ActorWrapper caller, void* params, std::string eventName)
{
    if (gamePaused || !inGameEvent || chatbox_open)
        return;

    if (matchEnded)
    {
        auto disablePostMatchQuickchats_cvar = GetCvar(Cvars::disablePostMatchQuickchats);
        if (!disablePostMatchQuickchats_cvar || disablePostMatchQuickchats_cvar.getBoolValue()) 
            return;
    }

    auto keyPressData = reinterpret_cast<UGameViewportClient_TA_execHandleKeyPress_Params*>(params);
    if (!keyPressData)
        return;

    std::string keyName = keyPressData->Key.ToString();
    EInputEvent keyEventType = static_cast<EInputEvent>(keyPressData->EventType);

    if (keyEventType == EInputEvent::IE_Pressed)
    {
        keyStates[keyName] = true;      // update key state (for CheckCombination() to analyze a "snapshot" of all pressed buttons)

        // update state for tracking whether user is using gamepad or pc inputs
        using_gamepad = keyPressData->bGamepad;
        //LOG("Using gamepad: {}", using_gamepad);      // can uncomment for testing purposes, otherwise it clutters up the console

        ButtonPress buttonPressEvent{ keyName, std::chrono::steady_clock::now() };

        // get min binding delay
        auto minBindingDelay_cvar = GetCvar(Cvars::minBindingDelay);
        if (!minBindingDelay_cvar)
            return;
        double minBindingDelay_raw = minBindingDelay_cvar.getFloatValue();
        auto minBindingDelay = std::chrono::duration<double>(minBindingDelay_raw);

        // get max sequence time window
        auto sequenceTimeWindow_cvar = GetCvar(Cvars::sequenceTimeWindow);
        if (!sequenceTimeWindow_cvar)
            return;
        double sequenceTimeWindow_raw = sequenceTimeWindow_cvar.getFloatValue();
        auto sequenceTimeWindow = std::chrono::duration<double>(sequenceTimeWindow_raw);


        // check if any bindings triggered
        for (Binding& binding : Bindings)
        {
            if (binding.enabled && binding.ShouldBeTriggered(buttonPressEvent, keyStates, lastBindingActivated, epochTime, minBindingDelay, sequenceTimeWindow))
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
        keyStates[keyName] = false;     // update key state (for CheckCombination() to analyze a "snapshot" of all pressed buttons)
}


void CustomQuickchat::Event_GFxHUD_TA_ChatPreset(ActorWrapper caller, void* params, std::string eventName)
{
    AGFxHUD_TA_execChatPreset_Params* Params = reinterpret_cast<AGFxHUD_TA_execChatPreset_Params*>(params);
    if (!Params)
        return;

    // get cvars
    auto enabled_cvar =                     GetCvar(Cvars::enabled);
    auto overrideDefaultQuickchats_cvar =   GetCvar(Cvars::overrideDefaultQuickchats);
    auto blockDefaultQuickchats_cvar =      GetCvar(Cvars::blockDefaultQuickchats);

    if (!enabled_cvar || !enabled_cvar.getBoolValue())
        return;

    // block default quickchat if necessary
    if (overrideDefaultQuickchats_cvar.getBoolValue())
    {
        auto currentTime = std::chrono::steady_clock::now();

        auto blockQuickchatWindow = std::chrono::duration<double>(BLOCK_DEFAULT_QUICKCHAT_WINDOW);

        if (currentTime <= (lastBindingActivated + blockQuickchatWindow))
        {
            Params->Index = 420;    // effectively blocks default quickchat from propagating
        }
    }
    else if (blockDefaultQuickchats_cvar.getBoolValue())
    {
        Params->Index = 420;    // effectively blocks default quickchat from propagating
    }
}


// happens after joining a match and after a binding has been changed in RL settings
void CustomQuickchat::Event_InitUIBindings(ActorWrapper Caller, void* Params, std::string eventName)
{
    auto caller = reinterpret_cast<UGFxData_Controls_TA*>(Caller.memory_address);
    if (!caller)
        return;

    // wait 0.5s to allow all the UGFxData_Controls_TA::MapUIBinding(...) calls to finish
    DELAY_CAPTURE(0.5f,
        if (!caller)
            return;
        determine_quickchat_labels(caller);
    , caller);
}


// NOTE: Running this on every chat preset pressed (aka every time the quickchat ui shows up) ensures the correct group of custom
// quickchat labels (pc vs gamepad) will be displayed. It may seem more efficient to apply chat labels to ui less often, but that wouldn't
// account for user switching between pc & gamepad inputs
void CustomQuickchat::Event_OnPressChatPreset(ActorWrapper Caller, void* Params, std::string eventName)
{
    if (gameWrapper->IsInFreeplay())
        return;

    auto enabled_cvar = GetCvar(Cvars::enabled);
    auto overrideDefaultQuickchats_cvar = GetCvar(Cvars::overrideDefaultQuickchats);
    if (!enabled_cvar || !enabled_cvar.getBoolValue())
        return;
    if (!overrideDefaultQuickchats_cvar || !overrideDefaultQuickchats_cvar.getBoolValue())
        return;

    auto caller = reinterpret_cast<UGFxData_Chat_TA*>(Caller.memory_address);
    if (!caller)
        return;

    auto params = reinterpret_cast<UGFxData_Chat_TA_execOnPressChatPreset_Params*>(Params);
    if (!params)
        return;

    apply_custom_qc_labels_to_ui(caller, params);
}


void CustomQuickchat::Event_ApplyChatSpamFilter(ActorWrapper caller, void* params, std::string eventName)
{
    auto pc = reinterpret_cast<APlayerController_TA*>(caller.memory_address);
    if (!pc)
        return;

    auto disableChatTimeout_cvar = GetCvar(Cvars::disableChatTimeout);
    if (!disableChatTimeout_cvar)
        return;
    bool disableChatTimeout = disableChatTimeout_cvar.getBoolValue();

    // effectively disables chat timeout (in freeplay)
    pc->ChatSpam.MaxValue = disableChatTimeout ? 420 : 4;       // default 4
    pc->ChatSpam.DecayRate = disableChatTimeout ? 69 : 1;       // default 1
    pc->ChatSpam.RiseAmount = disableChatTimeout ? 1 : 1.2;     // default 1.2
}


void CustomQuickchat::Event_GFxHUD_TA_NotifyChatDisabled(ActorWrapper caller, void* params, std::string eventName)
{
    gamePaused = false;

    auto useCustomChatTimeoutMsg_cvar = GetCvar(Cvars::useCustomChatTimeoutMsg);
    if (!useCustomChatTimeoutMsg_cvar || !useCustomChatTimeoutMsg_cvar.getBoolValue())
        return;

    auto hud = reinterpret_cast<AGFxHUD_TA*>(caller.memory_address);
    if (!hud)
        return;

    Instances.SetChatTimeoutMsg(chatTimeoutMsg, hud);
}


void CustomQuickchat::Event_PlayerController_EnterStartState(ActorWrapper Caller, void* Params, std::string eventName)
{
    inGameEvent = true;

    // set chat timeout message
    auto useCustomChatTimeoutMsg_cvar = GetCvar(Cvars::useCustomChatTimeoutMsg);
    if (!useCustomChatTimeoutMsg_cvar || !useCustomChatTimeoutMsg_cvar.getBoolValue())
        return;

    auto caller = reinterpret_cast<APlayerController*>(Caller.memory_address);
    if (!caller || !caller->myHUD || !caller->myHUD->IsA<AGFxHUD_TA>())
        return;
    auto hud = static_cast<AGFxHUD_TA*>(caller->myHUD);

    Instances.SetChatTimeoutMsg(chatTimeoutMsg, hud);
}


// when uncensored chat is recieved
void CustomQuickchat::event_HUDBase_TA_OnChatMessage(ActorWrapper Caller, void* Params, std::string eventName)
{
    if (!*m_uncensorChats)
        return;

    auto params = reinterpret_cast<AHUDBase_TA_execOnChatMessage_Params*>(Params);
    if (!params)
        return;

    FChatMessage& msg = params->NewMsg;
    if (msg.bPreset)
        return;

    auto caller = reinterpret_cast<AHUDBase_TA*>(Caller.memory_address);
    if (!caller)
        return;

    m_mostRecentUncensoredChat = msg;
}

// when censored chat is displayed
void CustomQuickchat::event_GFxData_Chat_TA_OnChatMessage(ActorWrapper Caller, void* Params, std::string eventName)
{
    auto params = reinterpret_cast<UGFxData_Chat_TA_execOnChatMessage_Params*>(Params);
    if (!params)
        return;

    if (*m_uncensorChats)
    {
        std::string gfxUid = ChatMsgData::generateUid(params);
        std::string censoredMsg = params->Message.ToString();
        if (gfxUid == m_mostRecentUncensoredChat.uid && censoredMsg != m_mostRecentUncensoredChat.uncensoredMsg)
        {
			params->Message = StringUtils::newFString(m_mostRecentUncensoredChat.uncensoredMsg);
            LOG("Uncensored chat: \"{}\" --> \"{}\"",
                Format::EscapeBraces(censoredMsg), Format::EscapeBraces(m_mostRecentUncensoredChat.uncensoredMsg));
        }
    }

    if (*m_removeTimestamps)
    {
		auto timeStampFstr = reinterpret_cast<FStringBase*>(&params->TimeStamp);
		timeStampFstr->size = 0;
    }
}


void CustomQuickchat::Event_PushMenu(ActorWrapper Caller, void* Params, std::string eventName)
{
    auto params = reinterpret_cast<UGFxData_MenuStack_TA_execPushMenu_Params*>(Params);
    if (!params)
        return;

    if (params->MenuName.ToString() == "MidGameMenuMovie")
        gamePaused = true;
}


void CustomQuickchat::Event_PopMenu(ActorWrapper Caller, void* Params, std::string eventName)
{
    auto params = reinterpret_cast<UGFxData_MenuStack_TA_execPopMenu_Params*>(Params);
    if (!params)
        return;

    if (params->MenuName.ToString() == "MidGameMenuMovie")
        gamePaused = false;
}


void CustomQuickchat::Event_LoadingScreenStart(std::string eventName)
{
    gamePaused = false;
    matchEnded = false;
    inGameEvent = false;
    chatbox_open = false;

    // reset all "pressed" buttons (to fix bug of bindings mistakenly firing bc a key's state is stuck in "pressed" mode upon joining a game/freeplay)
    for (auto& [key, state] : keyStates)
        state = false;
}
