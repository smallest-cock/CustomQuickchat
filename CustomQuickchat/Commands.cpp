#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::toggleEnabled_cmd(std::vector<std::string> args)
{
	CVarWrapper enabledCvar = cvarManager->getCvar(CvarNames::enabled);
	if (!enabledCvar) return;

	bool enabled = enabledCvar.getBoolValue();
	enabledCvar.setValue(!enabled);
}


void CustomQuickchat::showPathDirectories_cmd(std::vector<std::string> args)
{
	auto paths = getPathsFromEnvironmentVariable();

	LOG("==================== PATH directories ====================");
	for (const auto& path : paths)
	{
		LOG(path);
	}
}


void CustomQuickchat::test_cmd(std::vector<std::string> args)
{
	// ...
}