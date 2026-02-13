# Changelog

## v1.10.5
- Improved method of blocking default quickchats when a custom binding is triggered (maybe)
- Minor UI improvements
- Updated internal SDK to match latest version of RL (v2.64)

## v1.10.4
- Fixed `[[lastChat]]` keywords not working due to RL update v2.59 [#19](https://github.com/smallest-cock/CustomQuickchat/issues/19)

## v1.10.3-hotfix.2
- Added support for version suffixes in plugin update system

## v1.10.3-Hotfix
- Fixed broken plugin init for Steam users

## v1.10.3
- Removed some features that no longer work due to RL's recent chat system update:
    - Disabling chat timeout in freeplay
    - Chat uncensoring
        - RIP you will be miss 🪦🙏🥀
- Updated internal SDK to match latest version of RL (v2.60)

## v1.10.2
- Added ability to programatically send chats via console commands
    |           Command     | Description |
    |:----------------------|:-----------:|
    | `cqc_send_chat_match "some chat"` | Send a match chat |
    | `cqc_send_chat_team "some chat"` | Send a team chat |
    | `cqc_send_chat_party "some chat"` | Send a party chat |
    - Can be used as a simple way for other plugins to send chats ✅
- Updated internal SDK to match latest version of RL (v2.59)

## v1.10.1
- Fixed bug in applying custom chat timeout message
- Minor improvements to binding detection system
- Minor UI improvements

## v1.10.0
- Added support for arbitrary length button sequence bindings
  - Button sequences can now be longer than 2 buttons! 🥳 e.g. `6` → `7` → `Ctrl` → `8`
- Updated internal SDK to match latest version of RL (v2.58)

## v1.9.7
- Fixed crashes if user is missing the `SpeechToText.exe` file
- Added a button to test the speech-to-text websocket connection
- Improved internal websockets functionality
- Updated internal SDK to match latest version of RL (v2.56)
    - Should fix in-game crashes #14 

## v1.9.6
- Improved chat uncensoring implementation 👀

## v1.9.5
- Added new `[[rumbleItem]]` keyword to get the name of your rumble item #7
- Updated internal SDK to match latest version of RL (v2.55)
- Added an update button in settings to easily update the plugin when there's a new version available

## v1.9.4
- Fixed bug causing disabled quickchats to remain in the RL quickchat UI
- Added new keywords to get name of the closest player #4
    |           keyword     | description |
    |:---------------------:|:-----------:|
    `[[closestPlayer]]`     | returns the name of the closest player
    `[[closestOpponent]]`   | returns the name of the closest opponent
    `[[closestTeammate]]`   | returns the name of the closest teammate 
    - **Example:** [[closestOpponent]] is an outstanding opponent

## v1.9.3
- Updated internal SDK to match latest RL update (v2.54)

## v1.9.2
- Updated internal SDK to match latest RL update (v2.53)

## v1.9.1
- Made randomization of word variations optional #6
  - You can now send chats in a specific order from a list 😄
- Slightly improved the randomization of variations

## v1.9.0
- Added option to uncensor chats in settings 🔥
- Improved speech-to-text server and install script
- Updated internal SDK to match latest RL update (v2.52)

## v1.8.4
- Improved menu UI
- Fixed issue where certain commands can be inadvertently triggered when typing in chat #11
- Fixed custom chat timeout message setting not being checked before applying the message
- Updated internal SDK to match latest RL update (v2.51)

## v1.8.3
- Updated internal SDK to match latest RL update (v2.50)
- Added a notification in settings for when the plugin needs an update

## v1.8.2
- Added an option in settings to toggle randomization for the sarcasm text effect
- Fixed a bug that allowed bindings to be triggered while typing in the chat box #11

## v1.8.1
- Added an experimental fix for mid-game crashes due to the latest bakkesmod / RL update
  - It's possible the crashes were intentionally caused by bakkesmod

## v1.8.0
- Integrated the features from deprecated Lobby Info plugin
  - You can now use the `[[lastChat]]` and `[[blast ...]]` features without needing to install another plugin 👌
  - Added support for the new quickchats released in RL update v2.48 (which was causing Lobby Info to crash the game)
- Added checkbox in bindings menu to enable/disable individual bindings #6

## v1.7.1
- Improved websocket functionality (thanks to @ChasonDeshotel )
  - Better logging, error handling and reliability
- Fixed a bug causing websocket server and client to start on different ports when the plugin first loads

## v1.7.0
- Added custom quickchat labels to the in-game UI
- Added support for nested word variations (up to 10 levels deep)
- Added ability to change the port number used for speech-to-text websocket connection
- Improved sarcasm text effect (more randomized capitalization)
- Fixed bug causing websocket server to remain running in background even after RL is closed
- Removed the problem of installation overwriting existing `Bindings.json` and `Variations.json` files
  - Updating the plugin is now easier: Just download the latest release and run `install.bat`
- Increased default button sequence time window to 2 seconds (which is the default time window for RL quickchats)
- Updated internal SDK to match latest RL update (v2.48)

## v1.6.6
- improved speech-to-text functionality
  - no longer requires Python to be installed
  - uses WebSockets for quicker and more reliable inter-process communication
- improved bindings menu UI slightly
- updated internal SDK to match latest RL update (v2.46)
- fixed bug causing potential crash when switching tabs with the bindings menu open

## v1.6.5
- added error logging for speech-to-text python script
  - located at `bakkesmod\data\CustomQuickchat\ErrorLog.txt`
- improved code for sending/modifying chats
- updated sdk to match latest RL update

## v1.6.4
- fixed crash on startup for some users (hopefully)
  - the crash was caused by searching for the python interpreter on every startup (which is known to cause crashes for some)
  - it will now only search for python interpreter if user has explicitly enabled the option in settings

## v1.6.3
- added a button in settings to send a test chat
- improved the logic for triggering bindings
- added another method to search for python interpreter (for those experiencing speech-to-text issues)

## v1.6.2
- added `[[menu]]` keyword to instantly exit to main menu
- added `[[forfeit]]` keyword to instantly forfeit the current match
- updated SDK to match latest RL update
- improved logic for processing bindings with keywords and word variations

## v1.6.1
- added a forfeit command
  - bind cqc_forfeit to forfeit a match with one press of a button 👌

## v1.6.0
- fixed bug allowing chats to be sent while in pause menu (e.g. while trying to forfeit)
- added option to remove chat timestamps
- added option to customize the chat timeout message
- added option to toggle chat timeouts in freeplay
- added option to disable custom chats in post-match screen
  - useful if you want to use default post-match quickchats like "gg" or "Well played."
  - also prevents bindings from accidentally triggering while navigating the post-match menu

## v1.5.4
- fixed bug causing crash on plugin load (for some users) due to unnecessarily searching for python interpreter

## v1.5.3
- improved stability when plugin loads to reduce the likelihood of a crash (hopefully)
- improved/simplified the process of saving settings values across RL restarts

## v1.5.2
- added an additional option to get speech-to-text working

## v1.5.1
- added support for international characters like űüäőö
  - some characters may show up as "?" in the settings window, but they should appear normally in-game

## v1.5.0
- added option to override default RL quickchats with custom ones (to prevent both chats from being sent at once)
- added option to "block" all default RL quickchats from activating (without needing to unbind them)
- disabled chat timeouts in freeplay
- added the `cqc_list_bindings` command to list all current RL bindings (for both PC and gamepad buttons)

## v1.4.10
- added option in settings to manually enter filepath of pythonw.exe
- improved function to automatically find filepath of python interpreter (hopefully)
- added `cbo_show_path_directories` command to help troubleshoot issues finding python interpreter

## v1.4.9
- removed some unnecessary things in plugin initialization code to hopefully prevent crashes on startup (some users were experiencing them)
- mic calibration for speech-to-text is now only an optional thing found in plugin settings (as opposed to being automatically activated when the plugin loads)

## v1.4.8
- slightly improved settings UI
- updated plugin for latest game update
- refactored code to be more modular and organized

## v1.4.7
- added microphone calibration on startup (for speech-to-text)
  - also added button in settings to manually start calibration
- added plugin version number in settings

## v1.4.6
- improved code to find the filepath of `pythonw.exe` for speech-to-text
  - now uses `SearchPathW()` from the Windows API (hopefully more reliable)
- added microphone device name in error notification when speech isn't detected (should help troubleshooting)

## v1.4.5
- added extra logging to help troubleshoot speech-to-text bug
- slightly improved the code which handles filepaths

## v1.4.4
- tried to fix a bug causing an error on the first speech-to-text attempt ..... maybe unsuccessfully

## v1.4.3
- made speech-to-text notifications fancier and less obtrusive (no more chat clutter)
- added settings for notification behavior
  - slider to control the duration
  - button to perform a test notification

## v1.4.2
- fixed speech-to-text bug (for real)
- added option to view speech-to-text notifications in chat
- added slider in settings to control timeout duration for starting speech-to-text

## v1.4.1
- fixed speech-to-text bug (hopefully)

## v1.4.0
- added speech-to-text functionality

## v1.3.2
- fixed bug causing CVar values not to persist across RL restarts

## v1.3.1
- fixed bug in button sequence bindings causing a sequence using the same 1st and 2nd button to trigger prematurely

## v1.3.0
- added ability to blast people's ranks in chat
- added ability to repeat or use other people's chats in your chat

## v1.2.0
- added support for word variations
  - use `that is [[variation list name]]!` syntax to include word variations in your chats
  - when creating a word variation list in settings, each word/phrase should be separated by a new line (hence the tall text input)

## v1.1.1
- fixed long game freeze when a chat is sent for the first time after starting RL
  - patch is kinda hacky, but works 🥴

## v1.1.0
- added bindings menu to easily create custom bindings
  - supports button combination bindings (Circle + Up) and button sequence bindings (Left → Down)
- unintentionally added a bug when a chat is sent for the first time 🙃

## v1.0.0
- basic chat functionality
