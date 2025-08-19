#ifndef MACROS_H
#define MACROS_H

// convenient macros to avoid repetive typing  (should only be used within main plugin class)
// ... 'args' param comes last to support multiple variables in capture list

#define DELAY(delay, body, ...) gameWrapper->SetTimeout([ this, ##__VA_ARGS__ ](GameWrapper * gw) body, delay)

#define GAME_THREAD_EXECUTE(body, ...)                                                                                                     \
	do                                                                                                                                     \
	{                                                                                                                                      \
		gameWrapper->Execute([ this, ##__VA_ARGS__ ](GameWrapper * gw) body);                                                              \
	} while (0)

#define INTERVAL(delaySeconds, numIntervals, code)                                                                                         \
	for (int i = 0; i < numIntervals; i++)                                                                                                 \
	{                                                                                                                                      \
		gameWrapper->SetTimeout([this](GameWrapper* gw) { code }, delaySeconds * i);                                                       \
	}

#define INTERVAL_CAPTURE(delaySeconds, numIntervals, code, ...)                                                                            \
	for (int i = 0; i < numIntervals; i++)                                                                                                 \
	{                                                                                                                                      \
		gameWrapper->SetTimeout([this, __VA_ARGS__](GameWrapper* gw) { code }, delaySeconds * i);                                          \
	}

#define SEPARATE_THREAD(code)                                                                                                              \
	do                                                                                                                                     \
	{                                                                                                                                      \
		std::thread([this]() { code }).detach();                                                                                           \
	} while (0)

#define SEPARATE_THREAD_CAPTURE(code, ...)                                                                                                 \
	do                                                                                                                                     \
	{                                                                                                                                      \
		std::thread([this, __VA_ARGS__]() { code }).detach();                                                                              \
	} while (0)

// custom bm macro
#define BM_PLUGIN(classType, pluginName, pluginVersion, pluginType)                                                                        \
	static std::shared_ptr<classType> singleton;                                                                                           \
	extern "C"                                                                                                                             \
	{                                                                                                                                      \
		BAKKESMOD_PLUGIN_EXPORT uintptr_t getPlugin()                                                                                      \
		{                                                                                                                                  \
                                                                                                                                           \
			if (!singleton)                                                                                                                \
			{                                                                                                                              \
				singleton = std::shared_ptr<classType>(new classType());                                                                   \
			}                                                                                                                              \
			return reinterpret_cast<std::uintptr_t>(&singleton);                                                                           \
		}                                                                                                                                  \
		BAKKESMOD_PLUGIN_EXPORT void deleteMe()                                                                                            \
		{                                                                                                                                  \
			if (singleton)                                                                                                                 \
				singleton = nullptr;                                                                                                       \
		}                                                                                                                                  \
		BAKKESMOD_PLUGIN_EXPORT BakkesMod::Plugin::PluginInfo exports = {BAKKESMOD_PLUGIN_API_VERSION,                                     \
		    "C:\\Users\\edgar\\" #classType ".cpp",                                                                                        \
		    #classType,                                                                                                                    \
		    pluginName,                                                                                                                    \
		    pluginVersion,                                                                                                                 \
		    pluginType,                                                                                                                    \
		    getPlugin,                                                                                                                     \
		    deleteMe};                                                                                                                     \
	}

#endif
