#include "pch.h"
#include "PluginHelperBase.hpp"
#include "util/HookManager.hpp"
#include "util/Logging.hpp"

// cvars
CVarWrapper PluginHelperBase::registerCvar_Bool(const CvarData &cvar, bool startingValue) {
	std::string value = startingValue ? "1" : "0";

	return _globalCvarManager->registerCvar(cvar.name, value, cvar.description, true, true, 0, true, 1);
}

CVarWrapper PluginHelperBase::registerCvar_String(const CvarData &cvar, const std::string &startingValue) {
	return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
}

CVarWrapper PluginHelperBase::registerCvar_Number(const CvarData &cvar, float startingValue, bool hasMinMax, float min, float max) {
	std::string numberStr = std::to_string(startingValue);

	if (hasMinMax) {
		return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description, true, true, min, true, max);
	} else {
		return _globalCvarManager->registerCvar(cvar.name, numberStr, cvar.description);
	}
}

CVarWrapper PluginHelperBase::registerCvar_Color(const CvarData &cvar, const std::string &startingValue) {
	return _globalCvarManager->registerCvar(cvar.name, startingValue, cvar.description);
}

void PluginHelperBase::registerCommand(const CvarData &cvar, std::function<void(std::vector<std::string>)> callback) {
	_globalCvarManager->registerNotifier(cvar.name, callback, cvar.description, PERMISSION_ALL);
}

CVarWrapper PluginHelperBase::getCvar(const CvarData &cvar) { return _globalCvarManager->getCvar(cvar.name); }

// commands
void PluginHelperBase::runCommand(const CvarData &command, float delaySeconds) {
	if (delaySeconds == 0) {
		_globalCvarManager->executeCommand(command.name);
	} else if (delaySeconds > 0) {
		getPlugin()->gameWrapper->SetTimeout(
		    [this, command](GameWrapper *gw) { _globalCvarManager->executeCommand(command.name); }, delaySeconds);
	}
}

void PluginHelperBase::runCommandInterval(const CvarData &command, int numIntervals, float delaySeconds, bool delayFirstCommand) {
	if (!delayFirstCommand) {
		runCommand(command);
		numIntervals--;
	}

	for (int i = 1; i <= numIntervals; i++) {
		runCommand(command, delaySeconds * i);
	}
}

void PluginHelperBase::autoRunCommand(const CvarData &autoRunBool, const CvarData &command, float delaySeconds) {
	auto autoRunBool_cvar = getCvar(autoRunBool);
	if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
		return;

	runCommand(command, delaySeconds);
}

void PluginHelperBase::autoRunCommandInterval(
    const CvarData &autoRunBool, const CvarData &command, int numIntervals, float delaySeconds, bool delayFirstCommand) {
	auto autoRunBool_cvar = getCvar(autoRunBool);
	if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
		return;

	runCommandInterval(command, numIntervals, delaySeconds, delayFirstCommand);
}

// hooks
void PluginHelperBase::hookEvent(const char *funcName, std::function<void(std::string)> callback) {
	g_hookManager.registerHookPre(funcName, std::move(callback));
}

void PluginHelperBase::hookEventPost(const char *funcName, std::function<void(std::string)> callback) {
	g_hookManager.registerHookPost(funcName, std::move(callback));
}

void PluginHelperBase::hookWithCaller(const char *funcName, std::function<void(ActorWrapper, void *, std::string)> callback) {
	g_hookManager.registerHookPre(funcName, std::move(callback));
}

void PluginHelperBase::hookWithCallerPost(const char *funcName, std::function<void(ActorWrapper, void *, std::string)> callback) {
	g_hookManager.registerHookPost(funcName, std::move(callback));
}
