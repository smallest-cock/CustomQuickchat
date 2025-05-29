@echo off
git submodule init

:: Shallow clone for nlohmann/json (saves ~195MB)
git submodule update --depth=1 external/json

:: Configure branch-tracked submodules
call :setup_branch_submodule external/RLSDK main
call :setup_branch_submodule external/ModUtils main
call :setup_branch_submodule external/BMSDK master
call :setup_branch_submodule external/BakkesmodPluginTemplate master

git submodule status
exit /b

:setup_branch_submodule
git submodule set-branch --branch %2 %1
git submodule update --remote %1
cd %1
git checkout %2
git pull
cd ../..
exit /b