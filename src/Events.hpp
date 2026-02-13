#pragma once

namespace Events {
	constexpr auto GFxData_Chat_TA_SendChatPresetMessage = "Function TAGame.GFxData_Chat_TA.SendChatPresetMessage";
	constexpr auto GFxData_Chat_TA_OnChatMessage         = "Function TAGame.GFxData_Chat_TA.OnChatMessage";
	constexpr auto GFxData_Chat_TA_OnPressChatPreset     = "Function TAGame.GFxData_Chat_TA.OnPressChatPreset";
	constexpr auto GFxData_Chat_TA_UpdateChatGroups      = "Function TAGame.GFxData_Chat_TA.UpdateChatGroups";
	constexpr auto GFxData_Chat_TA_RefreshQuickChat      = "Function TAGame.GFxData_Chat_TA.RefreshQuickChat";
	constexpr auto GFxData_Chat_TA_HandleStateChanged    = "Function TAGame.GFxData_Chat_TA.HandleStateChanged";
	constexpr auto GFxData_Chat_TA_OnShellSet            = "Function TAGame.GFxData_Chat_TA.OnShellSet";
	constexpr auto GFxData_Chat_TA_OnRemoved             = "Function TAGame.GFxData_Chat_TA.OnRemoved";
	constexpr auto GFxData_Chat_TA_AddChatMessage        = "Function TAGame.GFxData_Chat_TA.AddChatMessage";
	constexpr auto GFxData_Chat_TA_AddPresetMessage      = "Function TAGame.GFxData_Chat_TA.AddPresetMessage";
	constexpr auto GFxData_Chat_TA_OpenChat              = "Function TAGame.GFxData_Chat_TA.OpenChat";
	constexpr auto GFxData_Chat_TA_OpenTeamChat          = "Function TAGame.GFxData_Chat_TA.OpenTeamChat";
	constexpr auto GFxData_Chat_TA_OpenPartyChat         = "Function TAGame.GFxData_Chat_TA.OpenPartyChat";
	constexpr auto GFxData_Chat_TA_ClearDistracted       = "Function TAGame.GFxData_Chat_TA.ClearDistracted";
	constexpr auto __GFxData_Chat_TA__AddChatMessage_0x1 =
	    "Function TAGame.__GFxData_Chat_TA__AddChatMessage_0x1.__GFxData_Chat_TA__AddChatMessage_0x1";

	constexpr auto HUDBase_TA_OnChatMessage     = "Function TAGame.HUDBase_TA.OnChatMessage";
	constexpr auto GFxHUD_TA_NotifyChatDisabled = "Function TAGame.GFxHUD_TA.NotifyChatDisabled";
	constexpr auto GFxHUD_TA_Destroyed          = "Function TAGame.GFxHUD_TA.Destroyed";
	constexpr auto GFxHUD_TA_ChatPreset         = "Function TAGame.GFxHUD_TA.ChatPreset";

	constexpr auto GFxData_MenuStack_TA_PushMenu = "Function TAGame.GFxData_MenuStack_TA.PushMenu";
	constexpr auto GFxData_MenuStack_TA_PopMenu  = "Function TAGame.GFxData_MenuStack_TA.PopMenu";

	constexpr auto GFxData_Controls_TA_PopulateSettingsMenu = "Function TAGame.GFxData_Controls_TA.PopulateSettingsMenu";
	constexpr auto GFxData_Controls_TA_InitUIBindings       = "Function TAGame.GFxData_Controls_TA.InitUIBindings";

	constexpr auto PlayerInput_Game_TA_OnActiveBindingsChanged = "Function TAGame.PlayerInput_Game_TA.OnActiveBindingsChanged";
	constexpr auto GameViewportClient_TA_HandleKeyPress        = "Function TAGame.GameViewportClient_TA.HandleKeyPress";

	constexpr auto PlayerController_EnterStartState        = "Function Engine.PlayerController.EnterStartState";
	constexpr auto PlayerControllerBase_TA_GetSaveObject   = "Function TAGame.PlayerControllerBase_TA.GetSaveObject";
	constexpr auto PlayerController_TA_ApplyChatSpamFilter = "Function TAGame.PlayerController_TA.ApplyChatSpamFilter";

	constexpr auto GameEvent_Soccar_TA_EventMatchEnded = "Function TAGame.GameEvent_Soccar_TA.EventMatchEnded";
	constexpr auto EngineShare_X_EventPreLoadMap       = "Function ProjectX.EngineShare_X.EventPreLoadMap";
}
