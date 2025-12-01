# FFXV Unlocker

A Windows utility for unlocking exclusive and promotional content in **Final Fantasy XV Windows Edition**.

## Background

This project started as a way to spoof the now-defunct Twitch Prime rewards system. The Twitch Prime promotion for FFXV ended years ago, leaving the Kooky Chocobo, bonus Gil, AP, and other rewards permanently inaccessible through legitimate means.

After successfully implementing the Twitch Prime URL spoofing, I decided to expand the project to include all other unavailable exclusive content - including Steam, Origin, Microsoft Store, and promotional items that were tied to time-limited events or specific platform purchases.

## Features

### Twitch Prime Rewards (URL Spoofing)
- Intercepts FFXV's Twitch OAuth2 authentication
- Runs a local HTTP server that mimics Twitch's API responses
- Unlocks all three Twitch Prime reward bundles:
  - Weatherworn Regalia Decal + 16 Rare Coins
  - Kooky Chocobo Tee + 100 AP
  - Kooky Chocobo + 10,000 Gil

### Platform Exclusive Unlocks
- **Normally Unavailable**: Blazefire Saber, Noodle Helmet, King's Knight items, and COMRADES packs
- **Origin Exclusives**: Decal Selection, The Sims 4 Pack
- **Microsoft Store**: FFXV Powerup Pack (Dodanuki sword, Phoenix Downs, Elixirs)
- **Steam Exclusives**: Half-Life crossover content (HEV Suit, Crowbar, Scientist Glasses)
- **Promotional Items**: Intel 8700K accessories, Alien Shield

### Two Unlock Methods

The tool uses two different mechanisms depending on the item:

1. **Byte Table (Direct Memory)** - Most items can be toggled individually by writing to FFXV's internal unlock table at `0x140752038+`. These appear with checkboxes in the UI.

2. **Code Patches (AOB Pattern)** - Steam and Promotional items are protected by anti-tamper measures that crash the game if individually modified. These require code-level patches that bypass ownership checks entirely. Use the "Platform Exclusives" options for these.

## Requirements

- Windows 10/11 (64-bit)
- Final Fantasy XV Windows Edition (Steam, Origin, Microsoft Store, or standalone)
- Administrator privileges (required for memory access)

## Installation

1. Download the latest release from the [Releases](../../releases) page
2. Extract to any folder
3. Run `FFXVUnlocker.exe` as Administrator

## Usage

### Basic Unlock Flow

1. **Launch FFXV** first (the game must be running)
2. **Run FFXVUnlocker.exe** as Administrator
3. The tool will auto-attach to the game process
4. Select the items you want to unlock:
   - Check individual items from the categories
   - Use "UNLOCK ALL ITEMS" for quick selection
   - Use "Platform Exclusives" checkboxes for Steam/Promotional content

### Twitch Prime Rewards

1. Check the Twitch Prime bundles you want
2. Click **"Start Server"** to launch the local HTTP server
3. In-game, go to the Twitch Prime rewards menu
4. The game will connect to localhost instead of Twitch
5. Complete the mock login flow in your browser
6. Rewards will be granted in-game

### Platform Exclusive Options

- **Unlock Without Steam Workshop**: Uses Unlock 3 (DL Bypass) - unlocks everything except Steam Workshop items
- **Unlock With Steam Workshop**: Uses Unlock 1 + Unlock 2 - unlocks everything including Steam Workshop content

**Note**: When either Platform Exclusive option is checked, individual item checkboxes are disabled since the code patches unlock entire categories at once.

## Technical Details

### Architecture

```
FFXVUnlocker/
├── src/
│   ├── main.cpp              # Application entry point
│   ├── MainWindow.cpp        # Qt GUI and state management
│   ├── MemoryEditor.cpp      # Process memory read/write operations
│   ├── PatternScanner.cpp    # AOB pattern scanning
│   └── HttpServer.cpp        # Local HTTP server for Twitch spoofing
├── include/
│   ├── MainWindow.h
│   ├── MemoryEditor.h
│   ├── PatternScanner.h
│   ├── HttpServer.h
│   └── Patches.h             # All patch definitions and unlock items
├── resources/
│   ├── resources.qrc         # Qt resource file (embeds wwwroot)
│   ├── app.rc                # Windows resource file
│   └── wwwroot/              # Embedded web pages for Twitch spoofing
└── CMakeLists.txt
```

### Memory Addresses

All unlock items are stored in a byte table starting at `0x140752038`:

| Offset | Address      | Item                    |
|--------|--------------|-------------------------|
| 0x00   | 0x140752038  | Blazefire Saber         |
| 0x13   | 0x14075204B  | Noodle Helmet           |
| 0x17   | 0x14075204F  | King's Knight Sticker   |
| 0x20   | 0x140752058  | Weatherworn Decal       |
| 0x21   | 0x140752059  | Kooky Chocobo Tee       |
| 0x22   | 0x14075205A  | Kooky Chocobo           |
| ...    | ...          | ...                     |

### Code Patches

Three AOB patterns are used for platform exclusive unlocks:

1. **Unlock 1 (Bounds Bypass)**: `83 F8 33 77 1A` → Changes `ja` to `jmp`
2. **Unlock 2 (Steam Bypass)**: `41 80 F4 01 83 FD 14` → Clears r12 and CF
3. **Unlock 3 (DL Bypass)**: `84 D2 74 4F EB 0D` → NOPs ownership check

## Building from Source

### Prerequisites

- Qt 6.x with MinGW 64-bit
- CMake 3.16+
- Windows SDK

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/FFXVUnlocker.git
cd FFXVUnlocker/FFXVUnlocker

# Create build directory
mkdir build && cd build

# Configure (adjust Qt path as needed)
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64" ..

# Build
cmake --build . --config Release
```

### Dependencies

- **Qt6::Widgets** - GUI framework
- **Qt6::Network** - HTTP server functionality
- **ws2_32** - Windows Sockets (Winsock)
- **psapi** - Process API for memory operations

## Known Limitations

- Must run as Administrator for memory access
- Game must be running before attaching
- Platform Exclusive patches are all-or-nothing (cannot select individual Steam/Promo items)
- Some items may require a game restart or save/load to appear
- COMRADES (multiplayer) items require the COMRADES expansion

## Disclaimer

This tool modifies game memory at runtime. Use at your own risk. This project is not affiliated with Square Enix, Twitch, Amazon, or any other company. All trademarks are property of their respective owners.

This tool is intended for single-player use to restore access to content that is no longer obtainable through official channels. Do not use this tool to gain unfair advantages in any online or competitive context.