#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <sys/stat.h>

struct BoardTheme {
    std::string name;
    int lightR, lightG, lightB;
    int darkR, darkG, darkB;
    int accentR, accentG, accentB;
};

static const BoardTheme BOARD_THEMES[] = {
    {"Classic",      235, 209, 166, 139, 90, 43,  212, 175, 55},
    {"Ocean",        220, 235, 245, 80,  130, 185, 150, 200, 220},
    {"Forest",       230, 235, 210, 90,  140, 70,  150, 210, 120},
    {"Marble",       240, 238, 235, 180, 175, 170, 200, 180, 150},
    {"Ember",        255, 220, 190, 160, 70,  45,  240, 180, 60},
    {"Royal",        240, 225, 245, 110, 60,  120, 200, 170, 220},
};
static const int NUM_THEMES = sizeof(BOARD_THEMES) / sizeof(BOARD_THEMES[0]);

struct Settings {
    bool soundEnabled = true;
    bool musicEnabled = true;
    bool darkMode = true;
    int boardTheme = 0;

    // AI difficulty sliders
    int aiDepth = 3;
    int aiBlunderRate = 12;
    int aiAggression = 50;
    int aiPersonality = -1; // -1=custom, 0=Pauly, 1=Knightly, 2=Bishop, 3=Rook, 4=Queen
    bool fullscreen = false;

    const BoardTheme& getTheme() const { return BOARD_THEMES[boardTheme % NUM_THEMES]; }
    std::string themeName() const { return BOARD_THEMES[boardTheme % NUM_THEMES].name; }
    void cycleTheme() { boardTheme = (boardTheme + 1) % NUM_THEMES; }

    // ── Persistence ──
    // Preferences (dark/light mode, board theme, sound/music, fullscreen) are
    // remembered across launches instead of resetting to defaults every time.
    static std::string configPath() {
        const char* home = std::getenv("HOME");
        std::string dir = home ? std::string(home) + "/Library/Application Support/Chess Kumar"
                                : std::string(".");
        mkdir(dir.c_str(), 0755); // no-op if it already exists
        return dir + "/settings.cfg";
    }

    void save() const {
        std::ofstream f(configPath(), std::ios::trunc);
        if (!f) return;
        f << "soundEnabled=" << (soundEnabled ? 1 : 0) << "\n";
        f << "musicEnabled=" << (musicEnabled ? 1 : 0) << "\n";
        f << "darkMode="     << (darkMode ? 1 : 0)     << "\n";
        f << "boardTheme="   << boardTheme              << "\n";
        f << "fullscreen="   << (fullscreen ? 1 : 0)   << "\n";
    }

    void load() {
        std::ifstream f(configPath());
        if (!f) return; // no saved settings yet — keep defaults
        std::string line;
        while (std::getline(f, line)) {
            std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            int value = std::atoi(line.substr(eq + 1).c_str());
            if (key == "soundEnabled") soundEnabled = value != 0;
            else if (key == "musicEnabled") musicEnabled = value != 0;
            else if (key == "darkMode") darkMode = value != 0;
            else if (key == "boardTheme") boardTheme = (value % NUM_THEMES + NUM_THEMES) % NUM_THEMES;
            else if (key == "fullscreen") fullscreen = value != 0;
        }
    }
};

#endif
