#pragma once
#include "Cvars.hpp"
#include <ModUtils/wrappers/GFxWrapper.hpp>

template <typename Derived> class Component
{
protected:
	std::shared_ptr<GameWrapper> gameWrapper;

	template <typename... Args>
	static void LOG(std::string_view format_str, Args&&... args) // overload LOG function to add component name prefix
	{
		std::string strWithComponentName = std::format("[{}] {}", Derived::componentName, format_str);
		::LOG(std::vformat(strWithComponentName, std::make_format_args(args...)));
	}

	template <typename... Args>
	static void LOGERROR(std::string_view format_str, Args&&... args) // overload LOG function to add component name prefix
	{
		std::string strWithComponentName = std::format("[{}] ERROR: {}", Derived::componentName, format_str);
		::LOG(std::vformat(strWithComponentName, std::make_format_args(args...)));
	}

	// hooks
	void hookEvent(const char* funcName, std::function<void(std::string eventName)> callback)
	{
		gameWrapper->HookEvent(funcName, callback);
		LOG("Hooked function pre: \"{}\"", funcName);
	}

	void hookEventPost(const char* funcName, std::function<void(std::string eventName)> callback)
	{
		gameWrapper->HookEventPost(funcName, callback);
		LOG("Hooked function post: \"{}\"", funcName);
	}

	void hookWithCaller(const char* function_name, std::function<void(ActorWrapper Caller, void* Params, std::string eventName)> callback)
	{
		gameWrapper->HookEventWithCaller<ActorWrapper>(function_name, callback);
		LOG("Hooked function pre: \"{}\"", function_name);
	}

	void hookWithCallerPost(
	    const char* function_name, std::function<void(ActorWrapper Caller, void* Params, std::string eventName)> callback)
	{
		gameWrapper->HookEventWithCallerPost<ActorWrapper>(function_name, callback);
		LOG("Hooked function post: \"{}\"", function_name);
	}

	// cvars
	CVarWrapper getCvar(const CvarData& cvar) { return _globalCvarManager->getCvar(cvar.name); }

	CVarWrapper registerCvar_bool(const CvarData& cvar, bool startingValue, bool log = true)
	{
		std::string value = startingValue ? "1" : "0";

		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, value, cvar.description, true, true, 0, true, 1);
	}

	CVarWrapper registerCvar_string(const CvarData& cvar, const std::string& startingValue, bool log = true)
	{
		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
	}

	CVarWrapper registerCvar_number(
	    const CvarData& cvar, float startingValue, bool hasMinMax = false, float min = 0.0f, float max = 0.0f, bool log = true)
	{
		std::string numberStr = std::to_string(startingValue);

		if (log)
			LOG("Registered CVar: {}", cvar.name);
		if (hasMinMax)
		{
			return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description, true, true, min, true, max);
		}
		else
		{
			return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description);
		}
	}

	CVarWrapper registerCvar_color(const CvarData& cvar, const std::string& startingValue, bool log = true)
	{
		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
	}

	// commands
	void runCommand(const CvarData& command, float delaySeconds = 0.0f)
	{
		if (delaySeconds == 0)
		{
			_globalCvarManager->executeCommand(command.name);
		}
		else if (delaySeconds > 0)
		{
			gameWrapper->SetTimeout([this, command](GameWrapper* gw) { _globalCvarManager->executeCommand(command.name); }, delaySeconds);
		}
	}

	void autoRunCommand(const CvarData& autoRunBool, const CvarData& command, float delaySeconds = 0.0f)
	{
		auto autoRunBool_cvar = getCvar(autoRunBool);
		if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
			return;

		runCommand(command, delaySeconds);
	}

	void runCommandInterval(const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false)
	{
		if (!delayFirstCommand)
		{
			runCommand(command);
			numIntervals--;
		}

		for (int i = 1; i <= numIntervals; ++i)
			runCommand(command, delaySeconds * i);
	}

	void autoRunCommandInterval(
	    const CvarData& autoRunBool, const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand = false)
	{
		auto autoRunBool_cvar = getCvar(autoRunBool);
		if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
			return;

		runCommandInterval(command, numIntervals, delaySeconds, delayFirstCommand);
	}

	void registerCommand(const CvarData& cvar, std::function<void(std::vector<std::string>)> callback)
	{
		_globalCvarManager->registerNotifier(cvar.name, callback, cvar.description, PERMISSION_ALL);
	}

	bool isInMainMenu()
	{
		APlayerController* pc = Instances.getPlayerController();
		if (!pc)
		{
			LOGERROR("APlayerController* is null");
			return false;
		}

		if (!validUObject(pc->myHUD) || !pc->myHUD->IsA<AHUDBase_TA>())
		{
			LOGERROR("pc->myHUD is invalid or not a AHUDBase_TA");
			return false;
		}

		auto* hud = static_cast<AHUDBase_TA*>(pc->myHUD);
		if (!validUObject(hud->Shell) || !validUObject(hud->Shell->SystemData))
		{
			LOGERROR("hud->Shell or hud->Shell->SystemData is invalid");
			return false;
		}

		return hud->Shell->SystemData->UIState == L"MainMenu";
	}
};