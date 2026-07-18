#ifndef MENU_H
#define MENU_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "Settings.h"

enum MenuOption {
    OPTION_NONE,
    OPTION_PLAY_VS_BOT,
    OPTION_LOCAL_MULTIPLAYER,
    OPTION_QUIT,
    OPTION_SETTINGS,
    OPTION_SOUND_TOGGLE,
    OPTION_MUSIC_TOGGLE,
    OPTION_THEME_TOGGLE,
    OPTION_BOARD_THEME,
    OPTION_FULLSCREEN,
    OPTION_BOT_PAWN,
    OPTION_BOT_KNIGHT,
    OPTION_BOT_BISHOP,
    OPTION_BOT_ROOK,
    OPTION_BOT_QUEEN,
    OPTION_CUSTOM_BOT,
    OPTION_PLAY_AS_BLACK,
    OPTION_START_GAME,
    OPTION_BACK_TO_BOT_SELECT,
    OPTION_LOCAL_NETWORK,
    OPTION_PEER_PLAY,
    OPTION_BACK_LOBBY,
};

enum MenuScreen { SCREEN_MAIN, SCREEN_BOT_SELECT, SCREEN_SETTINGS, SCREEN_DIFFICULTY, SCREEN_LOBBY };

struct MenuItem {
    std::string text;
    std::string subtitle;
    sf::FloatRect bounds;
    MenuOption option;
    bool hovered;
};

struct SliderItem {
    std::string label;
    float min, max, value, step;
    sf::FloatRect trackBounds;
    bool dragging;
    int precision;

    float getNormalized() const {
        return (value - min) / (max - min);
    }

    void setFromNormalized(float n) {
        n = std::max(0.0f, std::min(1.0f, n));
        value = min + n * (max - min);
        value = std::round(value / step) * step;
        value = std::max(min, std::min(max, value));
    }

    std::string display() const {
        if (precision == 0) return std::to_string((int)value);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.*f", precision, value);
        return buf;
    }
};

class Menu {
public:
    Menu();
    void setFont(sf::Font* f);
    void setSettings(Settings* s) { settings = s; }
    void setWindowSize(int w, int h);
    void setScreen(MenuScreen screen);
    void reset();

    void handleMouseMove(int x, int y);
    MenuOption handleClick(int x, int y);
    bool handleSliderPress(int x, int y);
    void endSliderDrag() { for (auto& s : sliders) s.dragging = false; }
    bool isDraggingSlider() const;
    std::string getPersonalityName() const;
    bool isPlayAsBlack() const { return playAsBlack; }
    void togglePlayAsBlack() { playAsBlack = !playAsBlack; }

    void setPeers(const std::vector<std::string>& names, const std::vector<std::string>& ips) {
        peerNames = names; peerIPs = ips;
    }
    int getSelectedPeerIndex() const { return selectedPeer; }
    std::string getSelectedPeerIP() const {
        if (selectedPeer >= 0 && selectedPeer < (int)peerIPs.size())
            return peerIPs[selectedPeer];
        return "";
    }
    void setStatusText(const std::string& t) { statusText = t; }
    void setPlayerName(const std::string& n) { playerNameLabel = n; }
    void setConnectionState(int s) { connectionState = s; } // 0=idle,1=searching,2=connecting,3=connected

    void draw(sf::RenderWindow& window);

    Settings* getSettings() const { return settings; }
    int getPersonalityPreset() const { return personalityPreset; }
    void applyPreset(int preset);
    void syncSettings();
    void syncFromSettings();

private:
    sf::Font* font;
    Settings* settings = nullptr;
    int winW, winH;
    MenuScreen currentScreen;
    std::vector<MenuItem> items;
    std::vector<SliderItem> sliders;
    int personalityPreset; // -1=custom, 0-4
    bool playAsBlack = false;
    sf::FloatRect closeBtnBounds;
    bool closeBtnHovered = false;
    sf::Color bg, cardBg, cardBgHover, accent, textColor, muted, borderColor;

    sf::Clock ornamentClock;
    void drawOrnaments(sf::RenderWindow& window);

    std::vector<std::string> peerNames;
    std::vector<std::string> peerIPs;
    int selectedPeer = -1;
    std::string statusText = "Searching for players...";
    std::string playerNameLabel = "Player";
    int connectionState = 0; // 0=idle,1=searching,2=connecting,3=connected

    void applyTheme();
    sf::Text makeText(const std::string& s, int size, const sf::Color& col, bool centered = false);
    void buildMain();
    void buildBotSelect();
    void buildSettings();
    void buildDifficulty();
    void buildLobby();
    void updateBounds();
    void drawShadow(sf::RenderWindow& window, const sf::FloatRect& bounds);
    void drawCard(sf::RenderWindow& window, const sf::FloatRect& bounds, bool hovered);
    void drawSlider(sf::RenderWindow& window, const SliderItem& slider, int idx);
    void drawLobby(sf::RenderWindow& window);
};

#endif
