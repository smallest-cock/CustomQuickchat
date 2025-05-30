@echo off
setlocal

rem ----------------------------------------------------------------------
echo Changing current working directory to be script's directory...
cd /d "%~dp0"

rem Specify the destination CustomQuickchat folder for installation
set "customQuickchatInstallationFolder=%APPDATA%\bakkesmod\bakkesmod\data\CustomQuickchat"

rem Specify SpeechToText subfolder path
set "speechToTextFolder=%customQuickchatInstallationFolder%\SpeechToText"

rem Specify the bakkesmod plugins folder
set "bmPluginsFolder=%APPDATA%\bakkesmod\bakkesmod\plugins"

rem set variables for folder/file paths
set "sourceFolder=%~dp0CustomQuickchat"

set "dllFile=%~dp0CustomQuickchat.dll"

set "cfgFolder=%APPDATA%\bakkesmod\bakkesmod\cfg"


rem ----------------------------------------------------------------------
echo Copying .dll to bakkesmod plugins folder...

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
    echo Error occurred while copying .dll file to bakkesmod plugins folder.
    echo.
    echo.
    pause
    exit /b 1
) else (
    echo PLugin .dll file successfully copied to "%bmPluginsFolder%"
)


rem ----------------------------------------------------------------------
echo Copying CustomQuickchat folder to bakkesmod data folder...

rem Check if the source folder exists before attempting to copy
if not exist "%sourceFolder%" (
    echo Error: Source folder does not exist: "%sourceFolder%"
    echo.
    echo.
    pause
    exit /b 1
)

rem Remove old SpeechToText folder if it exists
if exist "%speechToTextFolder%" (
    rmdir /s /q "%speechToTextFolder%"
)

rem Perform the copy operation
xcopy "%sourceFolder%" "%customQuickchatInstallationFolder%" /E /I /Y

rem Check the exit code of xcopy and handle any errors if needed
if errorlevel 1 (
    echo Error occurred while copying the CustomQuickchat folder to bakkesmod data folder.
    echo.
    echo.
    pause
    exit /b 1
) else (
    echo Successfully copied CustomQuickchat folder to bakkesmod data folder
)


rem ----------------------------------------------------------------------
echo Adding line: "plugin load customquickchat" to plugins.cfg...

cd /d %cfgFolder%

set "newLine=plugin load customquickchat"

echo %newLine% | findstr /G:plugins.cfg > nul

IF %ERRORLEVEL% EQU 0 (
    echo Line already found in .cfg file...
) ELSE ( 
    echo %newLine% >> "plugins.cfg"
    echo Successfully added line to plugins.cfg
)



rem success message
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