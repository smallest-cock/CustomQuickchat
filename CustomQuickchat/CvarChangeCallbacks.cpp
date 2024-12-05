#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::changed_enabled(std::string cvarName, CVarWrapper updatedCvar)
{
	bool enabled = updatedCvar.getBoolValue();

	std::string msg = "Custom quickchats turned " + std::string(enabled ? "ON" : "OFF");

	GAME_THREAD_EXECUTE_CAPTURE(
		Instances.SpawnNotification("Custom Quickchat", msg, 3);
	, msg);
}


void CustomQuickchat::changed_enableSTTNotifications(std::string cvarName, CVarWrapper updatedCvar)
{
	bool enableSTTNotifications = updatedCvar.getBoolValue();

	std::string msg = "Speech-To-Text notifications turned " + std::string(enableSTTNotifications ? "ON" : "OFF");

	LOG(msg);
}


void CustomQuickchat::changed_overrideDefaultQuickchats(std::string cvarName, CVarWrapper updatedCvar)
{
	bool overrideDefaultQuickchats = updatedCvar.getBoolValue();

	// there can be only one...
	if (overrideDefaultQuickchats)
	{
		auto blockDefaultQuickchats_cvar = GetCvar(Cvars::blockDefaultQuickchats);
		if (!blockDefaultQuickchats_cvar) return;

		blockDefaultQuickchats_cvar.setValue(false);
	}
}


void CustomQuickchat::changed_blockDefaultQuickchats(std::string cvarName, CVarWrapper updatedCvar)
{
	bool blockDefaultQuickchats = updatedCvar.getBoolValue();

	// there can be only one...
	if (blockDefaultQuickchats)
	{
		auto overrideDefaultQuickchats_cvar = GetCvar(Cvars::overrideDefaultQuickchats);
		if (!overrideDefaultQuickchats_cvar) return;

		overrideDefaultQuickchats_cvar.setValue(false);
	}
}


void CustomQuickchat::changed_useCustomChatTimeoutMsg(std::string cvarName, CVarWrapper updatedCvar)
{
	bool useCustomChatTimeoutMsg = updatedCvar.getBoolValue();

	if (useCustomChatTimeoutMsg)
	{
		auto customChatTimeoutMsg_cvar = GetCvar(Cvars::customChatTimeoutMsg);
		if (!customChatTimeoutMsg_cvar) return;
	
		chatTimeoutMsg = customChatTimeoutMsg_cvar.getStringValue();
	}
	else
	{
		ResetChatTimeoutMsg();
	}
	
	GAME_THREAD_EXECUTE(
		Instances.SetChatTimeoutMsg(chatTimeoutMsg);
	);

}


void CustomQuickchat::changed_customChatTimeoutMsg(std::string cvarName, CVarWrapper updatedCvar)
{
	auto useCustomChatTimeoutMsg_cvar = GetCvar(Cvars::useCustomChatTimeoutMsg);
	if (!useCustomChatTimeoutMsg_cvar) return;

	if (useCustomChatTimeoutMsg_cvar.getBoolValue())
	{
		chatTimeoutMsg = updatedCvar.getStringValue();

		GAME_THREAD_EXECUTE(
			Instances.SetChatTimeoutMsg(chatTimeoutMsg);
		);
	}
}
