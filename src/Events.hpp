#pragma once


namespace Events
{
    constexpr auto SendChatPresetMessage =               "Function TAGame.GFxData_Chat_TA.SendChatPresetMessage";
    constexpr auto GFxData_Chat_TA_OnChatMessage =       "Function TAGame.GFxData_Chat_TA.OnChatMessage";
    constexpr auto OnPressChatPreset =                   "Function TAGame.GFxData_Chat_TA.OnPressChatPreset";
    constexpr auto UpdateChatGroups =                    "Function TAGame.GFxData_Chat_TA.UpdateChatGroups";
    constexpr auto RefreshQuickChat =                    "Function TAGame.GFxData_Chat_TA.RefreshQuickChat";
    constexpr auto HandleStateChanged =                  "Function TAGame.GFxData_Chat_TA.HandleStateChanged";
    constexpr auto GFxData_Chat_TA_OnShellSet =          "Function TAGame.GFxData_Chat_TA.OnShellSet";
    constexpr auto GFxData_Chat_TA_OnRemoved =           "Function TAGame.GFxData_Chat_TA.OnRemoved";
    constexpr auto GFxData_Chat_TA_AddChatMessage =      "Function TAGame.GFxData_Chat_TA.AddChatMessage";
    constexpr auto GFxData_Chat_TA_AddPresetMessage =    "Function TAGame.GFxData_Chat_TA.AddPresetMessage";
    constexpr auto GFxData_Chat_TA_OpenChat =            "Function TAGame.GFxData_Chat_TA.OpenChat";
    constexpr auto GFxData_Chat_TA_OpenTeamChat =        "Function TAGame.GFxData_Chat_TA.OpenTeamChat";
    constexpr auto GFxData_Chat_TA_OpenPartyChat =       "Function TAGame.GFxData_Chat_TA.OpenPartyChat";
    constexpr auto GFxData_Chat_TA_ClearDistracted =     "Function TAGame.GFxData_Chat_TA.ClearDistracted";

    constexpr auto PopulateSettingsMenu =                "Function TAGame.GFxData_Controls_TA.PopulateSettingsMenu";
    constexpr auto InitUIBindings =                      "Function TAGame.GFxData_Controls_TA.InitUIBindings";

    constexpr auto GFxHUD_TA_NotifyChatDisabled =        "Function TAGame.GFxHUD_TA.NotifyChatDisabled";
    constexpr auto GFxHUD_TA_Destroyed =                 "Function TAGame.GFxHUD_TA.Destroyed";
    constexpr auto GFxHUD_TA_ChatPreset =                "Function TAGame.GFxHUD_TA.ChatPreset";

    constexpr auto KeyPressed =                          "Function TAGame.GameViewportClient_TA.HandleKeyPress";
    constexpr auto ApplyChatSpamFilter =                 "Function TAGame.PlayerController_TA.ApplyChatSpamFilter";

    constexpr auto PlayerController_EnterStartState =    "Function Engine.PlayerController.EnterStartState";
    constexpr auto MatchEnded =                          "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded";
    constexpr auto LoadingScreenStart =                  "Function ProjectX.EngineShare_X.EventPreLoadMap";
    constexpr auto PushMenu =                            "Function TAGame.GFxData_MenuStack_TA.PushMenu";
    constexpr auto PopMenu =                             "Function TAGame.GFxData_MenuStack_TA.PopMenu";

    constexpr auto OnActiveBindingsChanged =             "Function TAGame.PlayerInput_Game_TA.OnActiveBindingsChanged";

    // possibly useful, but not used atm
    // constexpr std::string_view SetGamePaused =         "Function ProjectX.GFxShell_X.SetGamePaused";
    // constexpr std::string_view PodiumBegin =           "Function GameEvent_Soccar_TA.PrePodiumSpotlight.BeginState";
    // constexpr std::string_view PodiumEnd =             "Function GameEvent_Soccar_TA.PodiumSpotlight.EndState";

    constexpr auto HUDBase_TA_OnChatMessage =            "Function TAGame.HUDBase_TA.OnChatMessage";
}
