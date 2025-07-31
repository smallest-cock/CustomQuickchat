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

## 🔨 Building
> [!NOTE]  
> Building this plugin requires **64-bit Windows** and the **MSVC** toolchain
> - Due to reliance on the Windows SDK and the need for ABI compatibility with Rocket League

### 1. Initialize Submodules
Run `./scripts/init-submodules.bat` after cloning the repo to initialize the submodules optimally.

<details> <summary>🔍 Why this instead of <code>git submodule update --init</code> ?</summary>
<li>Uses a specific version of the <strong>asio</strong> library (1.18.2) required for compatibility with <strong>websocketpp</strong></li>
<li>Avoids downloading 200MB of history for the <strong>nlohmann/json</strong> library</li>
<li>Ensures Git can detect updates for the other submodules</li>
</details>

### 2. Build with CMake
1. Install [CMake](https://cmake.org/download) and [Ninja](https://github.com/ninja-build/ninja/releases) (or another build system if you prefer)
2. Run `cmake --preset windows-x64-msvc` (or create your own preset in a `CMakeUserPresets.json`) to configure
3. Run `cmake --build build`
   - The built binaries will be in `./plugins`

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
