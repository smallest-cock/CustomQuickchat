#include "pch.h"
#include "CustomQuickchat.h"



void CustomQuickchat::cmd_toggleEnabled(std::vector<std::string> args)
{
	CVarWrapper enabledCvar = GetCvar(Cvars::enabled);
	if (!enabledCvar) return;

	bool enabled = enabledCvar.getBoolValue();
	enabledCvar.setValue(!enabled);
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


void CustomQuickchat::cmd_list_custom_chat_labels(std::vector<std::string> args)
{
	determine_quickchat_labels(nullptr, true);

	LOG("============= pc custom qc lablels ============");

	for (int i = 0; i < 4; i++)
	{
		const auto& chat_label_arr = pc_qc_labels[i];

		LOG("{}:", preset_group_names[i]);
		for (int j = 0; j < 4; j++)
		{
			LOG("[{}]\t{}", j, chat_label_arr.at(j).ToString());
		}
	}

	LOG("=========== gamepad custom qc lablels =========");
	for (int i = 0; i < 4; i++)
	{
		const auto& chat_label_arr = gp_qc_labels[i];

		LOG("{}:", preset_group_names[i]);
		for (int j = 0; j < 4; j++)
		{
			LOG("[{}]\t{}", j, chat_label_arr.at(j).ToString());
		}
	}
}


void CustomQuickchat::cmd_exitToMainMenu(std::vector<std::string> args)
{
	auto shell = Instances.GetInstanceOf<UGFxShell_X>();
	if (!shell) return;

	shell->ExitToMainMenu();

	LOG("exited to main menu");
}


void CustomQuickchat::cmd_forfeit(std::vector<std::string> args)
{
	auto shell = Instances.GetInstanceOf<UGFxShell_TA>();
	if (!shell) return;

	shell->VoteToForfeit();

	LOG("voted to forfeit...");
}



// ============================ testing ============================

void CustomQuickchat::cmd_test(std::vector<std::string> args)
{
#ifdef USE_SPEECH_TO_TEXT

	Websocket->SendEvent("test", { { "data", "test" } });

#endif // USE_SPEECH_TO_TEXT
	
	LOG("did the test");
}


void CustomQuickchat::cmd_test2(std::vector<std::string> args)
{
	// ...
}