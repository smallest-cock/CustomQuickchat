#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::changed_enabled(std::string cvarName, CVarWrapper updatedCvar)
{
	bool enabled = updatedCvar.getBoolValue();

	std::string msg = "Custom quickchats turned " + std::string(enabled ? "ON" : "OFF");

	gameWrapper->Execute([this, msg](GameWrapper* gw) {

			Instances.SpawnNotification("Custom Quickchat", msg, 3);

		});

}


void CustomQuickchat::changed_autoDetectInterpreterPath(std::string cvarName, CVarWrapper updatedCvar)
{
	//bool autoDetectInterpreterPath = updatedCvar.getBoolValue();

	gameWrapper->Execute([this](GameWrapper* gw)
		{
			pyInterpreter = findPythonInterpreter();
		});
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
		auto blockDefaultQuickchats_cvar = cvarManager->getCvar(CvarNames::blockDefaultQuickchats);
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
		auto overrideDefaultQuickchats_cvar = cvarManager->getCvar(CvarNames::overrideDefaultQuickchats);
		if (!overrideDefaultQuickchats_cvar) return;

		overrideDefaultQuickchats_cvar.setValue(false);
	}
}