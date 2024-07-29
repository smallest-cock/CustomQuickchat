#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::toggleEnabled(std::vector<std::string> args)
{
	CVarWrapper enabledCvar = cvarManager->getCvar(CvarNames::enabled);
	if (!enabledCvar) return;

	bool enabled = enabledCvar.getBoolValue();
	enabledCvar.setValue(!enabled);
}


void CustomQuickchat::test(std::vector<std::string> args)
{
	// ...

	PreventGameFreeze();

	//Instances.SpawnNotification("test", "hope this doesn't crash", 3);
}