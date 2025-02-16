#pragma once

#include <string_view>

namespace Events
{
	constexpr const char* SendChatPresetMessage =				"Function TAGame.GFxData_Chat_TA.SendChatPresetMessage";
	constexpr const char* OnChatMessage =						"Function TAGame.GFxData_Chat_TA.OnChatMessage";
	constexpr const char* OnPressChatPreset =					"Function TAGame.GFxData_Chat_TA.OnPressChatPreset";
	constexpr const char* UpdateChatGroups =					"Function TAGame.GFxData_Chat_TA.UpdateChatGroups";
	constexpr const char* RefreshQuickChat =					"Function TAGame.GFxData_Chat_TA.RefreshQuickChat";
	constexpr const char* HandleStateChanged =					"Function TAGame.GFxData_Chat_TA.HandleStateChanged";
	constexpr const char* GFxData_Chat_TA_OnShellSet =			"Function TAGame.GFxData_Chat_TA.OnShellSet";
	constexpr const char* GFxData_Chat_TA_OnRemoved =			"Function TAGame.GFxData_Chat_TA.OnRemoved";
	constexpr const char* GFxData_Chat_TA_AddChatMessage =		"Function TAGame.GFxData_Chat_TA.AddChatMessage";
	constexpr const char* GFxData_Chat_TA_AddPresetMessage =	"Function TAGame.GFxData_Chat_TA.AddPresetMessage";

	constexpr const char* PopulateSettingsMenu =				"Function TAGame.GFxData_Controls_TA.PopulateSettingsMenu";
	constexpr const char* InitUIBindings =						"Function TAGame.GFxData_Controls_TA.InitUIBindings";

	constexpr const char* GFxHUD_TA_NotifyChatDisabled =		"Function TAGame.GFxHUD_TA.NotifyChatDisabled";
	constexpr const char* GFxHUD_TA_Destroyed =					"Function TAGame.GFxHUD_TA.Destroyed";
	constexpr const char* GFxHUD_TA_ChatPreset =				"Function TAGame.GFxHUD_TA.ChatPreset";

	constexpr const char* KeyPressed =							"Function TAGame.GameViewportClient_TA.HandleKeyPress";
	constexpr const char* ApplyChatSpamFilter =					"Function TAGame.PlayerController_TA.ApplyChatSpamFilter";

	constexpr const char* EnterStartState =						"Function Engine.PlayerController.EnterStartState";
	constexpr const char* MatchEnded =							"Function TAGame.GameEvent_Soccar_TA.EventMatchEnded";
	constexpr const char* LoadingScreenStart =					"Function ProjectX.EngineShare_X.EventPreLoadMap";
	constexpr const char* PushMenu =							"Function TAGame.GFxData_MenuStack_TA.PushMenu";
	constexpr const char* PopMenu =								"Function TAGame.GFxData_MenuStack_TA.PopMenu";

	constexpr const char* OnActiveBindingsChanged =				"Function TAGame.PlayerInput_Game_TA.OnActiveBindingsChanged";

	// possibly useful, but not used atm
	// constexpr std::string_view SetGamePaused =         "Function ProjectX.GFxShell_X.SetGamePaused";
	// constexpr std::string_view PodiumBegin =           "Function GameEvent_Soccar_TA.PrePodiumSpotlight.BeginState";
	// constexpr std::string_view PodiumEnd =             "Function GameEvent_Soccar_TA.PodiumSpotlight.EndState";
}
