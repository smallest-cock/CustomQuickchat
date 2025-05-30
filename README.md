# Custom Quickchat (BakkesMod Plugin)
Send custom quickchats (and more) using Rocket League’s native chat system.

✔️ No hacky keypress macros  
✔️ No blocked inputs  
✔️ No editing code

🎥 Video showcase: https://youtu.be/P4UZTl09oYo

<img src="./docs/images/cover_pic.png" alt="Plugin Overview" width="600"/>

## 🔧 Installation
Download the latest release from the [Releases page](https://github.com/smallest-cock/CustomQuickchat/releases)

>[!WARNING]
> This plugin is currently **incompatible with [BetterChat](https://bakkesplugins.com/plugins/view/416)**. If both plugins are installed, your game **may crash**!

## 📖 Usage
See the [Settings documentation](./docs/Settings.md) for:
- Custom keywords & effects
- Troubleshooting tips
- Additional info

## 💻 Console Commands
You can use the following commands in the BakkesMod console (`F6`) or bind them to keys:

| Command | Description | Best Used With |
|--------|--------------|:--------------:|
| `cqc_toggle_enabled` | Toggle custom quickchats on or off | Key bind |
| `cqc_forfeit` | Instantly forfeit the match | Key bind |
| `cqc_exit_to_main_menu` | Instantly return to the main menu | Key bind |
| `cqc_list_bindings` | Lists your current quickchat bindings | Console |
| `cqc_list_custom_chat_labels` | Shows all custom quickchat labels visible in-game | Console |

<br>

## 🛠️ Building the Plugin
To build the project, follow these steps:

### 1. Initialize Submodules

Run `scripts\init-submodules.bat` after cloning the repo to initialize the submodules in an optimal way.

<details> <summary>🔍 Why this instead of <code>git submodule update --init</code> ?</summary>
<li>Avoids downloading 200MB of history for the <strong>nlohmann/json</strong> library</li>
<li>Allows Git to detect updates for the other submodules</li>
</details>

---

### 2. Install Dependencies via vcpkg

This project uses [websocketpp](https://github.com/zaphoyd/websocketpp), which was removed from vcpkg's official packages on March 3rd 2025. So you'll need to use an older vcpkg version that still contains it.

**Quick Setup**:
```bash
# Clone vcpkg at a last known good commit (minimal download)
git clone https://github.com/microsoft/vcpkg.git --depth 1
cd vcpkg
git fetch --depth=1 origin efb1e7436979a30c4d3e5ab2375fd8e2e461d541
git checkout efb1e7436979a30c4d3e5ab2375fd8e2e461d541

# Initialize submodules (minimal download)
git submodule update --init --recursive --depth 1

# Bootstrap vcpkg and enable MSBuild integration
./bootstrap-vcpkg.bat
./vcpkg integrate install
```

➡️ Now when you build the project for the first time, vcpkg will build/install the dependencies listed in `vcpkg.json`.

More info: [vcpkg manifest mode](https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode?tabs=msbuild%2Cbuild-MSBuild#2---integrate-vcpkg-with-your-build-system)

---

### 3. Bundle the Python Script

This project includes `speech-to-text-server.pyw` which is a Python server that must be converted into an executable.

**Instructions:**

- Use [PyInstaller](https://github.com/pyinstaller/pyinstaller) (or the GUI wrapper [auto-py-to-exe](https://github.com/brentvollebregt/auto-py-to-exe)) with these command-line arguments:
  ```
  pyinstaller ./speech_to_text_server/speech-to-text-server.pyw --noconfirm --onedir --console --name "SpeechToText"
  ```
- Place the resulting `SpeechToText` folder here:
  ```
  bakkesmod/data/CustomQuickchat/SpeechToText
  ```
  (You can create the `CustomQuickchat` folder if it doesn't already exist)

<br>

## 👀 Credits
Inspired by Blaku’s [CustomBindingPlugin](https://github.com/blaku-rl/CustomBindingPlugin)

## ❤️ Support
If you found this plugin helpful and want to support future development:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/sslowdev)
