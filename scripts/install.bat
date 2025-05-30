@echo off
setlocal

rem ----------------------------------------------------------------------
echo Changing current working directory to be script's directory...
cd /d "%~dp0"

rem Specify the destination CustomQuickchat folder for installation
set "installationDestinationFolder=%APPDATA%\bakkesmod\bakkesmod\data\CustomQuickchat"

rem Specify SpeechToText subfolder path
set "speechToTextFolder=%installationDestinationFolder%\SpeechToText"

rem Specify the bakkesmod plugins folder
set "bmPluginsFolder=%APPDATA%\bakkesmod\bakkesmod\plugins"

rem set variables for folder/file paths
set "customQuickchatSourceFolder=%~dp0CustomQuickchat"

set "dllFile=%~dp0CustomQuickchat.dll"

set "cfgFolder=%APPDATA%\bakkesmod\bakkesmod\cfg"


rem ----------------------------------------------------------------------
echo.
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
    echo Plugin .dll successfully copied to "%bmPluginsFolder%"
)


rem ----------------------------------------------------------------------
if exist "%customQuickchatSourceFolder%" (
    rem Remove old SpeechToText folder if it exists
    if exist "%speechToTextFolder%" (
        echo.
        echo Yeeting old SpeechToText folder...
        rmdir /s /q "%speechToTextFolder%"
    )

    rem Perform the copy operation
    echo.
    echo Copying CustomQuickchat folder to bakkesmod data folder...
    xcopy "%customQuickchatSourceFolder%" "%installationDestinationFolder%" /E /I /Y

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
)


rem ----------------------------------------------------------------------
echo.
echo Adding line: "plugin load customquickchat" to plugins.cfg...

cd /d %cfgFolder%

set "newLine=plugin load customquickchat"

echo %newLine% | findstr /G:plugins.cfg > nul

IF %ERRORLEVEL% EQU 0 (
    echo Line already found in .cfg file
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