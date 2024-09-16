#pragma once



namespace Events
{
	constexpr const char* KeyPressed =				"Function TAGame.GameViewportClient_TA.HandleKeyPress";
	constexpr const char* ChatPresetPressed =		"Function TAGame.GFxHUD_TA.ChatPreset";
	constexpr const char* ApplyChatSpamFilter =		"Function TAGame.PlayerController_TA.ApplyChatSpamFilter";
	
	constexpr const char* EnterStartState =			"Function Engine.PlayerController.EnterStartState";
	constexpr const char* NotifyChatDisabled =		"Function TAGame.GFxHUD_TA.NotifyChatDisabled";
	constexpr const char* HUDDestroyed =			"Function TAGame.GFxHUD_TA.Destroyed";
	constexpr const char* OnChatMessage =			"Function TAGame.GFxData_Chat_TA.OnChatMessage";
	constexpr const char* MatchEnded =				"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded";
	constexpr const char* LoadingScreenStart =		"Function ProjectX.EngineShare_X.EventPreLoadMap";
	constexpr const char* SendChatPresetMessage =	"Function TAGame.GFxData_Chat_TA.SendChatPresetMessage";
	constexpr const char* PushMenu =				"Function TAGame.GFxData_MenuStack_TA.PushMenu";
	constexpr const char* PopMenu =					"Function TAGame.GFxData_MenuStack_TA.PopMenu";

	// possibly useful, but not used atm
	//constexpr const char* SetGamePaused =			"Function ProjectX.GFxShell_X.SetGamePaused";
	//constexpr const char* PodiumBegin =				"Function GameEvent_Soccar_TA.PrePodiumSpotlight.BeginState";
	//constexpr const char* PodiumEnd =				"Function GameEvent_Soccar_TA.PodiumSpotlight.EndState";
}