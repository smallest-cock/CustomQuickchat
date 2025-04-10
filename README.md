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

Check out the [Settings](./docs/Settings.md) page for info about special keywords, troubleshooting, and more
   
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
1. Initialize the repo's submodules
2. Install the remaining dependencies with vcpkg
3. Bundle `speech-to-text-server.pyw` into an executable named `SpeechToText.exe`

Contributions are welcome 🤗

### Initialize submodules
Assuming you already have git installed, run this command after cloning the repo to initialize the submodules:

```bash
git submodule update --init --recursive
```


### Install other dependencies with vcpkg
You'll need to install the libraries found in `vcpkg.json`

Brief steps:
1. Install vcpkg if you don't already have it ([follow step 1 here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-vs?pivots=shell-powershell#1---set-up-vcpkg))
2. `cd` to the project folder (where `vcpkg.json` is) and install the project's dependencies with
   ```vcpkg integrate install```
   - [more info here](https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode?tabs=msbuild%2Cbuild-MSBuild#2---integrate-vcpkg-with-your-build-system)


### Bundle Python script
To bundle `speech-to-text-server.pyw` into an executable, I use [auto-py-to-exe](https://github.com/brentvollebregt/auto-py-to-exe) (which uses [pyinstaller](https://github.com/pyinstaller/pyinstaller) internally) with the "One Directory" option (`--onedir` in pyinstaller)

For the plugin to recognize the python program, the executable needs to be named `SpeechToText.exe` and should be located in the `bakkesmod\data\CustomQuickchat\SpeechToText` folder. (You can create the folder if it doesn't already exist)


## 👀 Credits

Inspired by Blaku's [CustomBindingPlugin](https://github.com/blaku-rl/CustomBindingPlugin)

## ❤️ Support

<br>

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/sslowdev)
