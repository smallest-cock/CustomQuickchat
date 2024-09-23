#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::cmd_toggleEnabled(std::vector<std::string> args)
{
	CVarWrapper enabledCvar = GetCvar(Cvars::enabled);
	if (!enabledCvar) return;

	bool enabled = enabledCvar.getBoolValue();
	enabledCvar.setValue(!enabled);
}


void CustomQuickchat::cmd_showPathDirectories(std::vector<std::string> args)
{
	auto paths = getPathsFromEnvironmentVariable();

	LOG("==================== PATH directories ====================");
	for (const auto& path : paths)
	{
		LOG(path);
	}
}


void CustomQuickchat::cmd_listBindings(std::vector<std::string> args)
{
	// list button bindings
	auto controls = Instances.GetInstanceOf<UGFxData_Controls_TA>();
	if (!controls) {
		LOG("UGFxData_Controls_TA* is null!");
		return;
	}

	auto gpBindings = controls->GamepadBindings;
	auto pcBindings = controls->PCBindings;

	LOG("================ PC bindings =================");
	for (const auto& binding : pcBindings)
	{
		LOG("{}: {}", binding.Action.ToString(), binding.Key.ToString());
	}

	LOG("============== Gamepad bindings ==============");
	for (const auto& binding : gpBindings)
	{
		LOG("{}: {}", binding.Action.ToString(), binding.Key.ToString());
	}
}


void CustomQuickchat::cmd_forfeit(std::vector<std::string> args)
{
	auto shell = Instances.GetInstanceOf<UGFxShell_TA>();
	if (!shell) return;

	shell->VoteToForfeit();

	LOG("voted to forfeit...");
}


void CustomQuickchat::cmd_test(std::vector<std::string> args)
{
	// ...
}