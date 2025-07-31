@echo off
git submodule init

echo Shallow cloning nlohmann/json (saves ~195MB)...
git submodule update --depth=1 external/json

echo Shallow cloning websocketpp...
git submodule update --depth=1 external/websocketpp

:: Standalone Asio must be downgraded to be compatible with the decade old WebSocket++ library.
:: WebSocket++ expects asio 1.8.0+, but breaks with newer asio versions like 1.34.2.
:: 1.18.2 is a known compatible version, so we use that.
echo Initializing and checking out asio-1-18-2...
git submodule update --init external/asio
cd external/asio
git fetch --tags
git checkout tags/asio-1-18-2
cd ../..

:: Configure branch-tracked submodules
call :setup_branch_submodule external/RLSDK main
call :setup_branch_submodule external/ModUtils main
call :setup_branch_submodule external/BMSDK master
call :setup_branch_submodule external/BakkesmodPluginTemplate master

git submodule status
exit /b

:setup_branch_submodule
echo Setting up submodule %1 to track branch %2...
git submodule set-branch --branch %2 %1
git submodule update --remote %1
cd %1
git checkout %2
git pull
cd ../..
exit /b