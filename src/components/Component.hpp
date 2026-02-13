#pragma once
#include "util/Instances.hpp"
#include "util/Logging.hpp"
#include "util/HookManager.hpp"
#include "Cvars.hpp"

template <typename Derived>
class Component {
protected:
	std::shared_ptr<GameWrapper> gameWrapper;

	// logging
	template <typename... Args>
	static void LOG(std::string_view format_str, Args &&...args) // overload LOG function to add component name prefix
	{
		std::string strWithComponentName = std::format("[{}] {}", Derived::componentName, format_str);
		::LOG(strWithComponentName, std::forward<Args>(args)...);
	}
	template <typename... Args>
	static void DLOG(std::string_view format_str, Args &&...args) // overload LOG function to add component name prefix
	{
		if constexpr (DEBUG_LOG) {
			std::string strWithComponentName = std::format("[{}] {}", Derived::componentName, format_str);
			::LOG(strWithComponentName, std::forward<Args>(args)...);
		}
	}

	template <typename... Args>
	static void LOGWARNING(std::string_view format_str, Args &&...args) {
		std::string strWithComponentName = std::format(
		    "[{}] " ANSI_CODE_YELLOW "WARNING: {}" ANSI_CODE_SUFFIX, Derived::componentName, format_str);
		::LOG(strWithComponentName, std::forward<Args>(args)...);
	}
	template <typename... Args>
	static void DLOGWARNING(std::string_view format_str, Args &&...args) {
		if constexpr (DEBUG_LOG) {
			std::string strWithComponentName = std::format(
			    "[{}] " ANSI_CODE_YELLOW "WARNING: {}" ANSI_CODE_SUFFIX, Derived::componentName, format_str);
			::LOG(strWithComponentName, std::forward<Args>(args)...);
		}
	}

	template <typename... Args>
	static void LOGERROR(std::string_view format_str, Args &&...args) {
		std::string strWithComponentName = std::format(
		    "[{}] " ANSI_CODE_RED "ERROR: {}" ANSI_CODE_SUFFIX, Derived::componentName, format_str);
		::LOG(strWithComponentName, std::forward<Args>(args)...);
	}
	template <typename... Args>
	static void DLOGERROR(std::string_view format_str, Args &&...args) // overload LOG function to add component name prefix
	{
		if constexpr (DEBUG_LOG) {
			std::string strWithComponentName = std::format(
			    "[{}] " ANSI_CODE_RED "ERROR: {}" ANSI_CODE_SUFFIX, Derived::componentName, format_str);
			::LOG(strWithComponentName, std::forward<Args>(args)...);
		}
	}

	// hooks
	void hookEvent(const char *funcName, std::function<void(std::string eventName)> callback) {
		g_hookManager.registerHookPre(funcName, std::move(callback));
	}

	void hookEventPost(const char *funcName, std::function<void(std::string eventName)> callback) {
		g_hookManager.registerHookPost(funcName, std::move(callback));
	}

	void hookWithCaller(const char *funcName, std::function<void(ActorWrapper Caller, void *Params, std::string eventName)> callback) {
		g_hookManager.registerHookPre(funcName, std::move(callback));
	}

	void hookWithCallerPost(const char *funcName, std::function<void(ActorWrapper Caller, void *Params, std::string eventName)> callback) {
		g_hookManager.registerHookPost(funcName, std::move(callback));
	}

	// cvars
	CVarWrapper getCvar(const CvarData &cvar) { return _globalCvarManager->getCvar(cvar.name); }

	CVarWrapper registerCvar_bool(const CvarData &cvar, bool startingValue, bool log = true) {
		std::string value = startingValue ? "1" : "0";

		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, value, cvar.description, true, true, 0, true, 1);
	}

	CVarWrapper registerCvar_string(const CvarData &cvar, const std::string &startingValue, bool log = true) {
		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
	}

	CVarWrapper registerCvar_number(
	    const CvarData &cvar, float startingValue, bool hasMinMax = false, float min = 0.0f, float max = 0.0f, bool log = true) {
		std::string numberStr = std::to_string(startingValue);

		if (log)
			LOG("Registered CVar: {}", cvar.name);
		if (hasMinMax) {
			return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description, true, true, min, true, max);
		} else {
			return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description);
		}
	}

	CVarWrapper registerCvar_color(const CvarData &cvar, const std::string &startingValue, bool log = true) {
		if (log)
			LOG("Registered CVar: {}", cvar.name);
		return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
	}

	// commands
	void registerCommand(const CvarData &cvar, std::function<void(std::vector<std::string>)> callback) {
		_globalCvarManager->registerNotifier(cvar.name, std::move(callback), cvar.description, PERMISSION_ALL);
	}

	bool isInMainMenu() {
		APlayerController *pc = Instances.getPlayerController();
		if (!pc) {
			LOGERROR("APlayerController* is null");
			return false;
		}

		if (!validUObject(pc->myHUD) || !pc->myHUD->IsA<AHUDBase_TA>()) {
			LOGERROR("pc->myHUD is invalid or not a AHUDBase_TA");
			return false;
		}

		auto *hud = static_cast<AHUDBase_TA *>(pc->myHUD);
		if (!validUObject(hud->Shell) || !validUObject(hud->Shell->SystemData)) {
			LOGERROR("hud->Shell or hud->Shell->SystemData is invalid");
			return false;
		}

		return hud->Shell->SystemData->UIState == L"MainMenu";
	}
};
