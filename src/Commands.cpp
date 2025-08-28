#include "pch.h"
#include "logging.h"
#include "CustomQuickchat.hpp"
#include "Cvars.hpp"
#include "components/Instances.hpp"

void CustomQuickchat::initCommands()
{
	registerCommand(Commands::toggleEnabled,
	    [this](std::vector<std::string> args)
	    {
		    CVarWrapper enabledCvar = getCvar(Cvars::enabled);
		    if (!enabledCvar)
			    return;

		    bool enabled = enabledCvar.getBoolValue();
		    enabledCvar.setValue(!enabled);
	    });

	registerCommand(Commands::listBindings,
	    [this](std::vector<std::string> args)
	    {
		    auto* controls = Instances.GetInstanceOf<UGFxData_Controls_TA>();
		    if (!controls)
		    {
			    LOGERROR("UGFxData_Controls_TA* is null!");
			    return;
		    }

		    auto gpBindings = controls->GamepadBindings;
		    auto pcBindings = controls->PCBindings;

		    LOG("================ PC bindings =================");
		    for (const auto& binding : pcBindings)
			    LOG("{}: {}", binding.Action.ToString(), binding.Key.ToString());

		    LOG("============== Gamepad bindings ==============");
		    for (const auto& binding : gpBindings)
			    LOG("{}: {}", binding.Action.ToString(), binding.Key.ToString());
	    });

	registerCommand(Commands::list_custom_chat_labels,
	    [this](std::vector<std::string> args)
	    {
		    determineQuickchatLabels(nullptr, true);

		    LOG("============= pc custom qc lablels ============");

		    for (int i = 0; i < 4; ++i)
		    {
			    const auto& chat_label_arr = pc_qc_labels[i];

			    LOG("{}:", PRESET_GROUP_NAMES[i]);
			    for (int j = 0; j < 4; ++j)
				    LOG("[{}]\t{}", j, chat_label_arr.at(j).ToString());
		    }

		    LOG("=========== gamepad custom qc lablels =========");
		    for (int i = 0; i < 4; ++i)
		    {
			    const auto& chat_label_arr = gp_qc_labels[i];

			    LOG("{}:", PRESET_GROUP_NAMES[i]);
			    for (int j = 0; j < 4; ++j)
				    LOG("[{}]\t{}", j, chat_label_arr.at(j).ToString());
		    }
	    });

	registerCommand(Commands::list_playlist_info,
	    [this](std::vector<std::string> args)
	    {
		    auto* playlists = Instances.GetInstanceOf<UOnlineGamePlaylists_X>();
		    if (!playlists)
			    return;

		    LOG("DownloadedPlaylists size: {}", playlists->DownloadedPlaylists.size());

		    LOG("--------------------------------------");
		    LOG("ID --> Internal name --> Display name");
		    LOG("--------------------------------------");
		    for (const auto& p : playlists->DownloadedPlaylists)
		    {
			    if (!p)
				    continue;

			    LOG("{} --> {} --> {}", p->PlaylistId, playlists->IdToName(p->PlaylistId).ToString(), p->GetLocalizedName().ToString());
		    }
	    });

	registerCommand(Commands::exitToMainMenu,
	    [this](std::vector<std::string> args)
	    {
		    if (m_chatboxOpen)
			    return;

		    auto* shell = Instances.GetInstanceOf<UGFxShell_X>();
		    if (!shell)
			    return;

		    shell->ExitToMainMenu();

		    LOG("exited to main menu");
	    });

	registerCommand(Commands::forfeit,
	    [this](std::vector<std::string> args)
	    {
		    if (m_chatboxOpen)
			    return;

		    auto* shell = Instances.GetInstanceOf<UGFxShell_TA>();
		    if (!shell)
			    return;

		    shell->VoteToForfeit();

		    LOG("voted to forfeit...");
	    });

	// ================================= testing =================================
	registerCommand(Commands::test,
	    [this](std::vector<std::string> args)
	    {
#ifdef USE_SPEECH_TO_TEXT
		    Websocket->SendEvent("test", {{"data", "test"}});
#endif

		    LOG("did the test");
	    });

	registerCommand(Commands::test2,
	    [this](std::vector<std::string> args)
	    {
		    // ...
	    });
}
