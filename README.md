# sw-LinkSaver

[![Platform](https://img.shields.io/badge/Platform-Nintendo%20Switch-E60012?style=flat-square&logo=nintendo-switch)](https://github.com/switchbrew/libnx)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=flat-square&logo=c%2B%2B)](https://isocpp.org/)
[![UI](https://img.shields.io/badge/UI-Borealis-22c55e?style=flat-square)](#)
[![Format](https://img.shields.io/badge/Format-.NRO%20Homebrew-blue?style=flat-square)](#)

Native Nintendo Switch client application (.nro) for the LinkSaver multiplatform game save synchronization ecosystem. Built with C++17, libnx, and Borealis UI, it provides a native Horizon OS look-and-feel with Joy-Con and touch screen navigation, on-screen keyboard support, SD card folder explorer, and atomic save synchronization.

---

## Features

- **Horizon OS Native Interface**: Rendered at 60 FPS using Borealis, emulating the official Switch system style with Joy-Con controller navigation, focus animations, and touch controls.
- **On-Screen Keyboard Integration**: Leverages Horizon OS `SwkbdConfig` applet for entering server URLs, authentication tokens, and tag names.
- **SD Card Folder Explorer (`SdExplorerView`)**: Interactive browser for `sdmc:/` allowing users to navigate and bind emulator save directories using the (Y) controller button.
- **Smart URL Normalization**: Automatically resolves local LAN IP addresses (e.g., `192.168.1.50:8080`) and remote HTTPS domains (e.g., Cloudflare Tunnels).
- **Asynchronous Sync Engine**: Non-blocking background worker (`brls::async`) ensuring the UI stays completely fluid during network requests and file transfers.
- **Download Resumption & Garbage Collection**: Supports HTTP Range headers for resuming partial downloads and automatically purges orphaned `.linksaver_part_*` temporary files upon failure.
- **Bilingual Support (i18n)**: Full English (en-US) and Spanish (es-ES) localization loaded from `romfs`.

---

## Directory Structure

```
sw-LinkSaver/
├── CMakeLists.txt              # Cross-compilation configuration
├── include/
│   ├── api/
│   │   ├── api_client.hpp      # libcurl REST client with Range resume
│   │   └── url_normalizer.hpp  # Smart address parser (LAN IP vs HTTPS)
│   ├── config/
│   │   └── config_manager.hpp  # sdmc:/switch/LinkSaver/config.json manager
│   ├── crypto/
│   │   └── sha256.hpp          # FIPS 180-4 streaming hash calculator
│   ├── platform/
│   │   └── software_keyboard.hpp # Horizon swkbd applet wrapper
│   ├── sync/
│   │   └── sync_engine.hpp     # Bidirectional manifest and upload coordinator
│   └── views/
│       ├── config_view.hpp     # Server URL, Token, and async Ping form
│       ├── directory_edit_view.hpp # Directory tag editor with extension filter
│       ├── directory_manager_view.hpp # List of mapped folders
│       ├── main_tab_view.hpp   # Root Borealis TabFrame
│       ├── sd_explorer_view.hpp # SDMC folder picker with (Y) button selection
│       └── sync_view.hpp       # One-click sync button and live status logs
├── romfs/
│   ├── icon.jpg                # 256x256 official homebrew icon
│   ├── icon.png                # Transparent app icon
│   └── i18n/                   # Translation strings (en-US, es-ES)
└── src/
    ├── main.cpp                # App entrypoint, Horizon sockets, and Borealis loop
    ├── api/
    ├── config/
    ├── crypto/
    ├── platform/
    ├── sync/
    └── views/
```

---

## Configuration File

The app stores its settings in `sdmc:/switch/LinkSaver/config.json`:

```json
{
  "server_url": "https://linksaver.yourdomain.com",
  "auth_token": "",
  "timeout_seconds": 10,
  "directories": [
    {
      "tag": "retroarch-cores",
      "display_name": "RetroArch Cores",
      "local_path": "/retroarch/cores/savefiles",
      "extensions": [".srm", ".sav", ".state"]
    },
    {
      "tag": "azahar-saves",
      "display_name": "Azahar 3DS",
      "local_path": "/switch/azahar/saves",
      "extensions": [".sav", ".dsv"]
    }
  ]
}
```

---

## Building for Nintendo Switch

### Prerequisites

- [devkitPro](https://devkitpro.org/) with the `switch-dev` package:
  ```bash
  sudo dkp-pacman -S switch-dev switch-curl switch-mbedtls switch-zlib
  ```
- CMake 3.16+
- Make / Ninja

### Compilation Steps

```bash
cd sw-LinkSaver
mkdir build && cd build

# Configure using devkitPro toolchain
cmake -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake ..

# Compile .elf and package .nro
make -j$(nproc)
```

The build produces `sw-LinkSaver.nro`.

### Installation on Nintendo Switch

1. Copy `sw-LinkSaver.nro` to your microSD card:
   ```
   sdmc:/switch/LinkSaver/sw-LinkSaver.nro
   ```
2. Insert the microSD card into your console and boot Atmosphere.
3. Open the Homebrew Menu and launch **LinkSaver**.
4. In the **Settings** tab, enter your `LinkSaverBack` server address (e.g., `http://192.168.1.50:8080` or your Cloudflare Tunnel).
5. Tap **Test Connection (Ping)** to verify.
6. Under **Save Folders**, add your emulator save directories using the SD explorer.
7. Return to the **Synchronize** tab and select **Sync All Folders**.
