@echo off

:: nlohmann/json (Shallow clone - saves ~195MB)
git submodule update --init --depth=1 external/json

:: Branch-tracked submodules
git submodule update --init --remote --branch main external/RLSDK
git submodule update --init --remote --branch main external/ModUtils
git submodule update --init --remote --branch master external/BMSDK
git submodule update --init --remote --branch master external/BakkesmodPluginTemplate

:: Verify
git submodule status