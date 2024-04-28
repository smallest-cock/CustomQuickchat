#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);



FString StrToFString(const std::string& str);


// ---------------------------- RLSDK shit ---------------------------------------------------------------------------

static constexpr int32_t INSTANCES_INTERATE_OFFSET = 100;

bool CheckNotInName(UObject* obj, const std::string& str);

namespace plugin {
	namespace instances {
		// Get an object instance by it's name and class type. Example: UTexture2D* texture = FindObject<UTexture2D>("WhiteSquare");
		template<typename T> T* FindObject(const std::string& objectName, bool bStrictFind = false);

		// Get the instance of a class using an index for the GObjects array. Example: UEngine* engine = GetInstanceOf<UEngine>(420);
		template<typename T> T* GetInstanceFromIndex(int index);

		// Get all active instances of a class type. Example: std::vector<APawn*> pawns = GetAllInstancesOf<APawn>();
		template<typename T> std::vector<T*> GetAllInstancesOf();

		// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
		template<typename T> T* GetInstanceOf();
	}
	namespace memory {
		uintptr_t FindPattern(HMODULE module, const unsigned char* pattern, const char* mask);
	}
	namespace globals {
		uintptr_t fnGObjects();
		uintptr_t fnGNames();
		void Init();
	}
}

// -------------------------------------------------------------------------------------------------------------------



class CustomQuickchat : public BakkesMod::Plugin::BakkesModPlugin
	,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{

	//Boilerplate
	void onLoad() override;
	//void onUnload() override; // Uncomment and implement if you need a unload method

	void SendChat(const std::string& chat, const std::string& chatMode);

	bool AreGObjectsValid();
	bool AreGNamesValid();


public:
	void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
