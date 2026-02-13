#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "Cvars.hpp"

class PluginHelperBase {
protected:
	// Same functions as above, but non-static
	CVarWrapper registerCvar_Bool(const CvarData &cvar, bool startingValue);
	CVarWrapper registerCvar_String(const CvarData &cvar, const std::string &startingValue);
	CVarWrapper registerCvar_Number(const CvarData &cvar, float startingValue, bool hasMinMax = false, float min = 0, float max = 0);
	CVarWrapper registerCvar_Color(const CvarData &cvar, const std::string &startingValue);
	void        registerCommand(const CvarData &cvar, std::function<void(std::vector<std::string>)> callback);
	CVarWrapper getCvar(const CvarData &cvar);

	void hookEvent(const char *funcName, std::function<void(std::string)> callback);
	void hookEventPost(const char *funcName, std::function<void(std::string)> callback);
	void hookWithCaller(const char *funcName, std::function<void(ActorWrapper, void *, std::string)> callback);
	void hookWithCallerPost(const char *funcName, std::function<void(ActorWrapper, void *, std::string)> callback);

	void runCommand(const CvarData &command, float delaySeconds = 0.0f);
	void autoRunCommand(const CvarData &autoRunBool, const CvarData &command, float delaySeconds = 0.0f);
	void runCommandInterval(const CvarData &command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);
	void autoRunCommandInterval(
	    const CvarData &autoRunBool, const CvarData &command, int numIntervals, float delaySeconds, bool delayFirstCommand = false);

	virtual BakkesMod::Plugin::BakkesModPlugin *getPlugin() = 0;
};
