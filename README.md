# Chess Kumar

A feature-rich chess application built with C++17 and SFML 3. Play against AI opponents, challenge friends over LAN, or enjoy a local two-player game — all with animated menus, visual effects, and a polished UI.

## Features

### Game Modes
- **vs Computer** — Play against an AI opponent with 5 difficulty levels (Beginner to Ultra Pro)
- **Local Multiplayer** — Two players on the same machine
- **LAN Multiplayer** — Host or join games over your local network with automatic discovery

### Gameplay
- Full chess rules including castling, en passant, promotion, and draw detection (stalemate, 50-move rule, insufficient material, threefold repetition)
- Move hints powered by the AI engine
- Board flipping (play as white or black)
- Undo moves
- Move history with algebraic notation
- Move analysis with color-coded annotations (brilliant, excellent, good, inaccuracy, mistake, blunder)
- Post-game review mode with keyboard navigation

### AI
- Custom chess engine with alpha-beta pruning and iterative deepening
- Multiple difficulty levels with dynamic evaluation parameters
- Distinct AI personalities with playing style variations
- Transposition table for improved search performance
- Opening book for principled opening play

### Polish
- Animated main menu with particle effects
- Sound effects for moves, captures, checks, checkmate, and stalemate
- Background music with playlist support (WAV, OGG, FLAC)
- Theme system with multiple color schemes
- Game auto-save and manual save/load
- Smooth piece movement animations
- Legal move highlighting

## Requirements

- **OS:** macOS 11.0+ (Big Sur or later)
- **Dependencies:** SFML 3.1+ (install via MacPorts: `sudo port install sfml`)
- **Compiler:** C++17 compatible (Xcode Command Line Tools)

## Building from Source

```bash
# Install SFML via MacPorts
sudo port install sfml

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(sysctl -n hw.ncpu)

# Run
./build/ChessKumar.app/Contents/MacOS/ChessKumar
```

### Creating a macOS .app Bundle

```bash
./build_app.sh
```

This produces a self-contained `ChessKumar.app` with all dependencies bundled, plus a `ChessKumar.dmg` installer.

### Universal Binary (Apple Silicon + Intel)

```bash
# First, build universal SFML (requires sudo)
sudo port upgrade sfml +universal

# Then build with universal flag
./build_app.sh --universal
```

## Project Structure

```
Chess Kumar/
  src/
    main.cpp           — Entry point, state machine, main loop
    ChessBoard.h/cpp   — Board representation, move generation, validation
    ChessAI.h/cpp      — AI engine with alpha-beta search
    Game.h/cpp         — Game state, rendering, move handling
    Menu.h/cpp         — Main menu with animations
    AudioManager.h/cpp — Sound effects and music playback
    AssetManager.h/cpp — Font and texture loading with platform-aware paths
    OpeningBook.h/cpp  — Opening book for principled play
    NetworkManager.h/cpp — LAN multiplayer via TCP/UDP
    Settings.h         — Game settings and preferences
  assets/
    pieces/            — Chess piece textures
    music/             — Background music
    fonts/             — Fonts (system fallback)
```

## Technologies

- **C++17** — Core language
- **SFML 3.1** — Graphics, windowing, audio, networking
- **CMake** — Build system

## License

This project is provided as-is for educational and personal use.
