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

## 🛠️ Building
To build the project you'll need to install the dependencies with vcpkg, and have an SDK for Rocket League (RLSDK)

Contributions are welcome :)

### vcpkg
You'll need to install the libraries found in `vcpkg.json`

Brief steps:
1. Install vcpkg ([follow step 1 here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-vs?pivots=shell-powershell#1---set-up-vcpkg))
2. `cd` to the project folder (where `vcpkg.json` is) and install the project's dependencies with this command
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

The resulting SDK wont be perfect, but it should work for the purposes of this plugin

>[!NOTE]
>To generate a more perfect SDK, you can use [CodeRed-Generator](https://github.com/CodeRedModding/CodeRed-Generator/) and reverse the game classes in `GameDefines.hpp` yourself

## 👀 Credits

Inspired by Blaku's [CustomBindingPlugin](https://github.com/blaku-rl/CustomBindingPlugin)

## ☕ Support

If you enjoy this plugin, consider supporting:

<a href="https://www.buymeacoffee.com/sslowdev" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 200px !important;" ></a>
