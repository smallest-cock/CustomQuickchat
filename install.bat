@echo off
setlocal

rem Change the current working directory to the script's directory
cd /d "%~dp0"

rem Specify the destination CustomQuickchat folder for installation
set "customQuickchatInstallationFolder=%APPDATA%\bakkesmod\bakkesmod\data\CustomQuickchat"

rem Specify the bakkesmod plugins folder
set "bmPluginsFolder=%APPDATA%\bakkesmod\bakkesmod\plugins"

rem set variables for folder/file paths
set "sourceFolder=%~dp0CustomQuickchat"

set "dllFile=%~dp0CustomQuickchat.dll"

set "cfgFolder=%APPDATA%\bakkesmod\bakkesmod\cfg"


rem ----------- Copy .dll file to bakkesmod plugins folder -----------

rem Check if the source folder exists before attempting to copy
if not exist "%bmPluginsFolder%" (
    echo.
    echo.
    echo Error: Destination folder does not exist: "%bmPluginsFolder%"
    echo.
    echo.
    pause
    exit /b 1
)

rem Use xcopy to copy the file to the destination folder
xcopy "%dllFile%" "%bmPluginsFolder%" /Y

rem Check the exit code of xcopy and handle any errors if needed
if errorlevel 1 (
    echo.
    echo.
    echo Error occurred while copying .dll file to bakkesmod plugins folder.
    echo.
    echo .... maybe try running as Administrator?
    echo.
    echo.
    pause
    exit /b 1
) else (
    echo PLugin .dll file successfully copied to "%bmPluginsFolder%"
)


rem ----------- Copy CustomQuickchat folder to bakkesmod data folder -----------

rem Check if the source folder exists before attempting to copy
if not exist "%sourceFolder%" (
    echo.
    echo.
    echo Error: Source folder does not exist: "%sourceFolder%"
    echo.
    echo .... maybe try running as Administrator?
    echo.
    echo.
    pause
    exit /b 1
)

rem Perform the copy operation
xcopy "%sourceFolder%" "%customQuickchatInstallationFolder%" /E /I /Y

rem Check the exit code of xcopy and handle any errors if needed
if errorlevel 1 (
    echo.
    echo.
    echo Error occurred while copying the CustomQuickchat folder to bakkesmod data folder.
    echo.
    echo .... maybe try running as Administrator?
    echo.
    echo.
    pause
    exit /b 1
) else (
    echo Successfully copied CustomQuickchat folder to bakkesmod data folder
)


rem ----------- Add line: plugin load customquickchat to plugins.cfg -------------

cd /d %cfgFolder%

set "newLine=plugin load customquickchat"

echo %newLine% | findstr /G:plugins.cfg > nul

IF %ERRORLEVEL% EQU 0 (
    echo Line already found in .cfg file...
) ELSE ( 
    echo %newLine% >> "plugins.cfg"
    echo Successfully added line to plugins.cfg
)



rem -------------------------- download plugin assets --------------------------

rem Define the folder for assets
set "ASSET_DIR=%APPDATA%\bakkesmod\bakkesmod\data\sslow_plugin_assets"

rem Create the folder if it doesn't exist
if not exist "%ASSET_DIR%" (
    mkdir "%ASSET_DIR%"
    echo Created folder: %ASSET_DIR%
)

rem Define image URLs and local file paths
set "URL1=https://raw.githubusercontent.com/smallest-cock/plugin-assets/main/images/settings-footer/discord.png"
set "URL2=https://raw.githubusercontent.com/smallest-cock/plugin-assets/main/images/settings-footer/github.png"
set "URL3=https://raw.githubusercontent.com/smallest-cock/plugin-assets/main/images/settings-footer/youtube.png"

set "FILE1=%ASSET_DIR%\discord.png"
set "FILE2=%ASSET_DIR%\github.png"
set "FILE3=%ASSET_DIR%\youtube.png"

rem Download the images
echo.
echo.
echo Downloading assets...
echo.
echo.

curl -o "%FILE1%" "%URL1%" || echo Failed to download discord.png
curl -o "%FILE2%" "%URL2%" || echo Failed to download github.png
curl -o "%FILE3%" "%URL3%" || echo Failed to download youtube.png


rem ------------------------------ success message -------------------------------
echo.
echo.
echo.
echo.
echo.
echo Setup successfully completed.
echo.
echo.
pause

endlocal
