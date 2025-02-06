# Custom Quickchat (BakkesMod plugin)

Enables custom quickchats in Rocket League

<img src='./docs/images/cover_pic.png' alt="overview" width="600"/>


<br>

Chats are sent through the native Rocket League chat system ✔️
  - Better than using a script + macros to simulate keypresses, which is error prone and hacky (temporarily blocks gameplay inputs while chats are typed out)

>[!WARNING]
>This plugin seems to be incompatible with the latest version of [BetterChat](https://bakkesplugins.com/plugins/view/416). If you have both installed your game may crash!

## 🎥 Video Tutorial

https://youtu.be/P4UZTl09oYo

<a href='https://youtu.be/P4UZTl09oYo'>
  <img src='./docs/images/YT_screenshot.png' alt="overview" width="400"/>
</a>

## 🔧 How To Install

Find the latest version in [Releases](https://github.com/smallest-cock/CustomQuickchat/releases)

## 📖 Usage

Check out the [Settings guide](./docs/Settings.md) for info about troubleshooting & special features
   
## 💻 Commands
You can run these commands in the bakkesmod console window (F6), or bind them to a key

| Command | Description | Preferred usage |
|---|---|:---:|
`cqc_toggle_enabled` | toggle custom quickchats on/off | key bind
`cqc_forfeit` | forfeit the current match | key bind
`cqc_exit_to_main_menu` | exit to the main menu | key bind
`cqc_list_bindings` | list all your current RL bindings | console
`cqc_list_custom_chat_labels` | list all custom chat labels that would show up in quickchat UI | console

## 🛠️ Building
To build the project you'll need to do the following:
- install the project's dependencies with vcpkg
- have an SDK for Rocket League (RLSDK)
- compile `speech-to-text-server.pyw` into an executable named `SpeechToText.exe`

Contributions are welcome 🤗

### vcpkg
You'll need to install the libraries found in `vcpkg.json`

Brief steps:
1. Install vcpkg ([follow step 1 here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-vs?pivots=shell-powershell#1---set-up-vcpkg))
2. `cd` to the project folder (where `vcpkg.json` is) and install the project's dependencies with
   ```vcpkg integrate install```
   - [more info here](https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode?tabs=msbuild%2Cbuild-MSBuild#2---integrate-vcpkg-with-your-build-system)

### SDK
You can generate one quickly using this config from matix2: https://github.com/matix2/RLSDK-Generator

Brief steps:
1. Add the folder `Engine/RocketLeague` to the VS project (for some reason it's missing)
2. Update the offsets for `GNames`/`GObjects` and the output folder path (in `Engine/RocketLeague/Configuration.cpp`)
    - You can get the offsets for `GNames` and `GObjects` using [RocketDumper](https://github.com/lchmagKekse/RocketDumper)
3. Build
4. Inject the .dll into Rocket League, and your SDK should begin generating

>[!NOTE]
>The resulting SDK wont be perfect but it should work for the purposes of this plugin


### python script
To compile `speech-to-text-server.pyw` into an executable, I use [auto-py-to-exe](https://github.com/brentvollebregt/auto-py-to-exe) (which uses [pyinstaller](https://github.com/pyinstaller/pyinstaller) internally) with the "One Directory" option (`--onedir` in pyinstaller)

For the plugin to recognize the python program, the executable needs to be named `SpeechToText.exe` and should be located in the `bakkesmod\CustomQuickchat\SpeechToText` folder. (You can create the folder if it doesn't already exist)


## 👀 Credits

Inspired by Blaku's [CustomBindingPlugin](https://github.com/blaku-rl/CustomBindingPlugin)

## ❤️ Support

<br>

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/sslowdev)
