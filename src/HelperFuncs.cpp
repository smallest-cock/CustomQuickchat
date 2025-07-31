#include "pch.h"
#include "CustomQuickchat.hpp"


// cvars
CVarWrapper CustomQuickchat::registerCvar_Bool(const CvarData& cvar, bool startingValue)
{
	std::string value = startingValue ? "1" : "0";

	return cvarManager->registerCvar(cvar.name, value, cvar.description, true, true, 0, true, 1);
}

CVarWrapper CustomQuickchat::registerCvar_String(const CvarData& cvar, const std::string& startingValue)
{
	return cvarManager->registerCvar(cvar.name, startingValue, cvar.description);
}

CVarWrapper CustomQuickchat::registerCvar_Number(const CvarData& cvar, float startingValue, bool hasMinMax, float min, float max)
{
	std::string numberStr = std::to_string(startingValue);

	if (hasMinMax)
	{
		return cvarManager->registerCvar(cvar.name, numberStr, cvar.description, true, true, min, true, max);
	}
	else
	{
		return cvarManager->registerCvar(cvar.name, numberStr, cvar.description);
	}
}

CVarWrapper CustomQuickchat::registerCvar_Color(const CvarData& cvar, const std::string& startingValue)
{
	return cvarManager->registerCvar(cvar.name, startingValue, cvar.description);
}

void CustomQuickchat::registerCommand(const CvarData& cvar, std::function<void(std::vector<std::string>)> callback)
{
	cvarManager->registerNotifier(cvar.name, callback, cvar.description, PERMISSION_ALL);
}

CVarWrapper CustomQuickchat::getCvar(const CvarData& cvar)
{
	return cvarManager->getCvar(cvar.name);
}


// commands
void CustomQuickchat::runCommand(const CvarData& command, float delaySeconds)
{
	if (delaySeconds == 0)
	{
		cvarManager->executeCommand(command.name);
	}
	else if (delaySeconds > 0)
	{
		gameWrapper->SetTimeout([this, command](GameWrapper* gw) { cvarManager->executeCommand(command.name); }, delaySeconds);
	}
}

void CustomQuickchat::runCommandInterval(const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand)
{
	if (!delayFirstCommand)
	{
		runCommand(command);
		numIntervals--;
	}

	for (int i = 1; i <= numIntervals; i++)
	{
		runCommand(command, delaySeconds * i);
	}
}

void CustomQuickchat::autoRunCommand(const CvarData& autoRunBool, const CvarData& command, float delaySeconds)
{
	auto autoRunBool_cvar = getCvar(autoRunBool);
	if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
		return;

	runCommand(command, delaySeconds);
}

void CustomQuickchat::autoRunCommandInterval(
	const CvarData& autoRunBool, const CvarData& command, int numIntervals, float delaySeconds, bool delayFirstCommand)
{
	auto autoRunBool_cvar = getCvar(autoRunBool);
	if (!autoRunBool_cvar || !autoRunBool_cvar.getBoolValue())
		return;

	runCommandInterval(command, numIntervals, delaySeconds, delayFirstCommand);
}


// hooks
void CustomQuickchat::hookEvent(const char* funcName, std::function<void(std::string)> callback)
{
	gameWrapper->HookEvent(funcName, callback);
	LOG("Hooked function pre: \"{}\"", funcName);
}

void CustomQuickchat::hookEventPost(const char* funcName, std::function<void(std::string)> callback)
{
	gameWrapper->HookEventPost(funcName, callback);
	LOG("Hooked function post: \"{}\"", funcName);
}

void CustomQuickchat::hookWithCaller(const char* funcName, std::function<void(ActorWrapper, void*, std::string)> callback)
{
	gameWrapper->HookEventWithCaller<ActorWrapper>(funcName, callback);
	LOG("Hooked function pre: \"{}\"", funcName);
}

void CustomQuickchat::hookWithCallerPost(const char* funcName, std::function<void(ActorWrapper, void*, std::string)> callback)
{
	gameWrapper->HookEventWithCallerPost<ActorWrapper>(funcName, callback);
	LOG("Hooked function post: \"{}\"", funcName);
}