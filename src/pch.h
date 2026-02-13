#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <bakkesmod/plugin/bakkesmodplugin.h>

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include <BakkesmodPluginTemplate/IMGUI/imgui.h>
#include <BakkesmodPluginTemplate/IMGUI/imgui_internal.h>
#include <BakkesmodPluginTemplate/IMGUI/imgui_stdlib.h>
#include <BakkesmodPluginTemplate/IMGUI/imgui_searchablecombo.h>
#include <BakkesmodPluginTemplate/IMGUI/imgui_rangeslider.h>

#include <limits.h>
#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <fstream>
#include <shellapi.h>

#pragma comment(lib, "Shlwapi.lib")

#include <json/single_include/nlohmann/json.hpp>

#define USE_GMALLOC
#include <RLSDK/RLSDK_w_pch_includes/SdkHeaders.hpp>

#include "util/Logging.hpp"

namespace fs = std::filesystem;
using json   = nlohmann::json;
