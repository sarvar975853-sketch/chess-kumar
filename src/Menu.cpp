#include "Menu.h"
#include "ChessAI.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

static const float CORNER_R = 14.0f;

static void drawRoundedRect(sf::RenderWindow& window, float x, float y, float w, float h,
                            float r, const sf::Color& color) {
    if (r > w / 2.0f) r = w / 2.0f;
    if (r > h / 2.0f) r = h / 2.0f;

    sf::RectangleShape body(sf::Vector2f(w, h - 2.0f * r));
    body.setFillColor(color);
    body.setPosition(sf::Vector2f(x, y + r));
    window.draw(body);

    sf::RectangleShape topCap(sf::Vector2f(w - 2.0f * r, r));
    topCap.setFillColor(color);
    topCap.setPosition(sf::Vector2f(x + r, y));
    window.draw(topCap);

    sf::RectangleShape bottomCap(sf::Vector2f(w - 2.0f * r, r));
    bottomCap.setFillColor(color);
    bottomCap.setPosition(sf::Vector2f(x + r, y + h - r));
    window.draw(bottomCap);

    sf::CircleShape circle(r);
    circle.setFillColor(color);
    circle.setPosition(sf::Vector2f(x, y));
    window.draw(circle);
    circle.setPosition(sf::Vector2f(x + w - 2.0f * r, y));
    window.draw(circle);
    circle.setPosition(sf::Vector2f(x, y + h - 2.0f * r));
    window.draw(circle);
    circle.setPosition(sf::Vector2f(x + w - 2.0f * r, y + h - 2.0f * r));
    window.draw(circle);
}

static std::string toggleStr(bool on) { return on ? "ON" : "OFF"; }

Menu::Menu()
    : font(nullptr), winW(900), winH(700), currentScreen(SCREEN_MAIN), personalityPreset(-1) {
    applyTheme();
    buildMain();
}

void Menu::setFont(sf::Font* f) { font = f; }
void Menu::setWindowSize(int w, int h) { winW = w; winH = h; updateBounds(); }
void Menu::reset() { currentScreen = SCREEN_MAIN; buildMain(); personalityPreset = -1; playAsBlack = false; }

void Menu::applyTheme() {
    bool dark = !settings || settings->darkMode;
    if (dark) {
        bg = sf::Color(20, 18, 22, 255);
        cardBg = sf::Color(50, 45, 55, 140);
        cardBgHover = sf::Color(60, 55, 70, 180);
        accent = sf::Color(212, 175, 55, 255);
        textColor = sf::Color(240, 235, 225, 255);
        muted = sf::Color(170, 160, 145, 200);
        borderColor = sf::Color(255, 255, 255, 30);
    } else {
        bg = sf::Color(245, 240, 235, 255);
        cardBg = sf::Color(255, 252, 248, 200);
        cardBgHover = sf::Color(255, 255, 255, 230);
        accent = sf::Color(180, 140, 40, 255);
        textColor = sf::Color(40, 35, 30, 255);
        muted = sf::Color(120, 110, 100, 200);
        borderColor = sf::Color(0, 0, 0, 20);
    }
}

sf::Text Menu::makeText(const std::string& s, int size, const sf::Color& col, bool centered) {
    if (!font) { static sf::Font dummy; return sf::Text(dummy, "", 0); }
    sf::Text t(*font, s, size);
    t.setFillColor(col);
    if (centered) {
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(sf::Vector2f(b.size.x / 2, b.size.y / 2));
    }
    return t;
}

void Menu::buildMain() {
    items.clear();
    sliders.clear();
    currentScreen = SCREEN_MAIN;
    items.push_back({"Chess Kumar", "", {}, OPTION_NONE, false});
    items.push_back({"Play vs Bot", "Challenge an AI opponent", {}, OPTION_PLAY_VS_BOT, false});
    items.push_back({"Local Network", "Find players on your network", {}, OPTION_LOCAL_NETWORK, false});
    items.push_back({"Local Multiplayer", "Play with a friend", {}, OPTION_LOCAL_MULTIPLAYER, false});
    items.push_back({"Settings", "Sound, music & theme", {}, OPTION_SETTINGS, false});
    items.push_back({"Quit", "Exit the game", {}, OPTION_QUIT, false});
    updateBounds();
}

void Menu::buildBotSelect() {
    items.clear();
    sliders.clear();
    currentScreen = SCREEN_BOT_SELECT;
    items.push_back({"Choose Your Opponent", "", {}, OPTION_NONE, false});

    // Play as color toggle - visually distinct
    items.push_back({playAsBlack ? "Playing as: Black" : "Playing as: White", "Click to switch color", {}, OPTION_PLAY_AS_BLACK, false});

    struct BotInfo { std::string name; std::string desc; MenuOption opt; };
    BotInfo bots[] = {
        {"Pauly", "Beginner - Reckless attacker", OPTION_BOT_PAWN},
        {"Knightly", "Casual - Development focus", OPTION_BOT_KNIGHT},
        {"Bishop", "Intermediate - Positional play", OPTION_BOT_BISHOP},
        {"Rook", "Advanced - Strong & balanced", OPTION_BOT_ROOK},
        {"Queen", "Expert - Near-perfect play", OPTION_BOT_QUEEN},
    };

    for (auto& b : bots)
        items.push_back({b.name, b.desc, {}, b.opt, false});

    items.push_back({"Custom", "Fine-tune AI difficulty sliders", {}, OPTION_CUSTOM_BOT, false});
    updateBounds();
}

void Menu::buildSettings() {
    items.clear();
    sliders.clear();
    currentScreen = SCREEN_SETTINGS;
    items.push_back({"Settings", "", {}, OPTION_NONE, false});

    std::string soundLabel = "Sound FX: " + toggleStr(settings ? settings->soundEnabled : true);
    std::string musicLabel = "Music: " + toggleStr(settings ? settings->musicEnabled : true);
    std::string themeLabel = (settings && !settings->darkMode) ? "Theme: Light" : "Theme: Dark";
    std::string boardThemeLabel = "Board: " + (settings ? settings->themeName() : std::string("Classic"));

    items.push_back({soundLabel, "Game sound effects", {}, OPTION_SOUND_TOGGLE, false});
    items.push_back({musicLabel, "Background music", {}, OPTION_MUSIC_TOGGLE, false});
    items.push_back({themeLabel, "Switch dark / light mode", {}, OPTION_THEME_TOGGLE, false});
    items.push_back({boardThemeLabel, "Board color scheme", {}, OPTION_BOARD_THEME, false});
    std::string fsLabel = "Fullscreen: " + toggleStr(settings ? settings->fullscreen : false);
    items.push_back({fsLabel, "Toggle fullscreen mode", {}, OPTION_FULLSCREEN, false});
    updateBounds();
}

void Menu::applyPreset(int preset) {
    personalityPreset = preset;
    if (!settings) return;
    switch (preset) {
        case 0: // Pauly
            settings->aiDepth = 2;
            settings->aiBlunderRate = 25;
            settings->aiAggression = 80;
            break;
        case 1: // Knightly
            settings->aiDepth = 3;
            settings->aiBlunderRate = 12;
            settings->aiAggression = 50;
            break;
        case 2: // Bishop
            settings->aiDepth = 4;
            settings->aiBlunderRate = 5;
            settings->aiAggression = 30;
            break;
        case 3: // Rook
            settings->aiDepth = 6;
            settings->aiBlunderRate = 2;
            settings->aiAggression = 60;
            break;
        case 4: // Queen
            settings->aiDepth = 8;
            settings->aiBlunderRate = 0;
            settings->aiAggression = 100;
            break;
        default:
            return;
    }
    settings->aiPersonality = preset;

    // Sync slider values from settings
    for (auto& s : sliders) {
        if (s.label == "Depth") s.value = (float)settings->aiDepth;
        else if (s.label == "Blunder") s.value = (float)settings->aiBlunderRate;
        else if (s.label == "Aggression") s.value = (float)settings->aiAggression;
    }
}

void Menu::syncSettings() {
    if (!settings) return;
    for (auto& s : sliders) {
        if (s.label == "Depth") settings->aiDepth = (int)s.value;
        else if (s.label == "Blunder") settings->aiBlunderRate = (int)s.value;
        else if (s.label == "Aggression") settings->aiAggression = (int)s.value;
    }
    settings->aiPersonality = personalityPreset;
}

void Menu::syncFromSettings() {
    if (!settings) return;
    for (auto& s : sliders) {
        if (s.label == "Depth") s.value = (float)settings->aiDepth;
        else if (s.label == "Blunder") s.value = (float)settings->aiBlunderRate;
        else if (s.label == "Aggression") s.value = (float)settings->aiAggression;
    }
}

std::string Menu::getPersonalityName() const {
    if (personalityPreset < 0) return "Custom";
    static const char* names[] = {"Pauly", "Knightly", "Bishop", "Rook", "Queen"};
    if (personalityPreset < 5) return names[personalityPreset];
    return "Custom";
}

void Menu::buildDifficulty() {
    items.clear();
    sliders.clear();
    currentScreen = SCREEN_DIFFICULTY;
    items.push_back({"Difficulty Settings", "", {}, OPTION_NONE, false});

    if (!settings) {
        settings = new Settings();
    }

    // Sliders
    sliders.push_back({"Depth", 1.0f, 8.0f, (float)settings->aiDepth, 1.0f, {}, false, 0});
    sliders.push_back({"Blunder", 0.0f, 50.0f, (float)settings->aiBlunderRate, 5.0f, {}, false, 0});
    sliders.push_back({"Aggression", 0.0f, 100.0f, (float)settings->aiAggression, 10.0f, {}, false, 0});

    personalityPreset = settings->aiPersonality;

    items.push_back({"Start Game", "Begin match with these settings", {}, OPTION_START_GAME, false});
    items.push_back({"Back", "Return to bot selection", {}, OPTION_BACK_TO_BOT_SELECT, false});
    updateBounds();
}

void Menu::updateBounds() {
    int cx = winW / 2;
    int startY = winH / 5;

    if (currentScreen == SCREEN_DIFFICULTY) {
        for (size_t i = 0; i < items.size(); ++i) {
            if (i == 0) {
                items[i].bounds = sf::FloatRect({(float)(cx - 220), (float)(winH / 7 - 25)}, {440.0f, 48.0f});
                continue;
            }
            int y = startY + 210 + (int)(i - 1) * 52;
            items[i].bounds = sf::FloatRect({(float)(cx - 150), (float)(y - 20)}, {300.0f, 40.0f});
        }

        // Slider tracks
        int slY = startY + 10;
        for (size_t i = 0; i < sliders.size(); ++i) {
            float trackW = 320.0f;
            float trackH = 8.0f;
            float trackX = cx - trackW / 2.0f;
            sliders[i].trackBounds = sf::FloatRect({trackX, (float)(slY + i * 65)}, {trackW, trackH});
        }
        return;
    }

    for (size_t i = 0; i < items.size(); ++i) {
        if (i == 0) {
            items[i].bounds = sf::FloatRect({(float)(cx - 220), (float)(winH / 7 - 25)}, {440.0f, 55.0f});
            continue;
        }
        int y = startY + (int)i * 72;
        // Extra spacing around Play As toggle in bot select screen
        if (currentScreen == SCREEN_BOT_SELECT && items[i].option == OPTION_PLAY_AS_BLACK)
            y += 16;
        if (currentScreen == SCREEN_BOT_SELECT && i > 1 && items[i - 1].option == OPTION_PLAY_AS_BLACK)
            y += 16;
        items[i].bounds = sf::FloatRect({(float)(cx - 190), (float)(y - 24)}, {380.0f, 48.0f});
    }
}

void Menu::setScreen(MenuScreen screen) {
    if (screen == SCREEN_MAIN) buildMain();
    else if (screen == SCREEN_BOT_SELECT) buildBotSelect();
    else if (screen == SCREEN_SETTINGS) buildSettings();
    else if (screen == SCREEN_DIFFICULTY) buildDifficulty();
    else if (screen == SCREEN_LOBBY) buildLobby();
}

void Menu::handleMouseMove(int x, int y) {
    // Lobby screen: track hovered peer
    if (currentScreen == SCREEN_LOBBY) {
        selectedPeer = -1;
        int listY = winH / 4 + 10;
        int itemH = 48;
        int itemW = 340;
        int cx = winW / 2;
        for (size_t i = 0; i < peerNames.size(); ++i) {
            int py = listY + i * (itemH + 8);
            sf::FloatRect bounds = {{(float)(cx - itemW / 2), (float)py}, {(float)itemW, (float)itemH}};
            if (bounds.contains({(float)x, (float)y})) {
                selectedPeer = (int)i;
                break;
            }
        }
        return;
    }

    for (auto& item : items)
        item.hovered = item.bounds.contains({(float)x, (float)y});

    // Slider dragging
    for (auto& s : sliders) {
        if (s.dragging) {
            float trackLeft = s.trackBounds.position.x;
            float trackRight = trackLeft + s.trackBounds.size.x;
            float nx = std::max(trackLeft, std::min((float)x, trackRight));
            float norm = (nx - trackLeft) / s.trackBounds.size.x;
            s.setFromNormalized(norm);

            // Mark as custom when slider is dragged
            personalityPreset = -1;
            syncSettings();
        }
    }
}

MenuOption Menu::handleClick(int x, int y) {
    // Check close button (top-left corner)
    if (currentScreen != SCREEN_MAIN && closeBtnBounds.contains({(float)x, (float)y}))
        return OPTION_QUIT;

    // Lobby screen: check play button on peers
    if (currentScreen == SCREEN_LOBBY) {
        int listY = winH / 4 + 10;
        int itemH = 48;
        int itemW = 340;
        int cx = winW / 2;
        for (size_t i = 0; i < peerNames.size(); ++i) {
            int py = listY + i * (itemH + 8);
            float btnX = cx - itemW / 2 + itemW - 80;
            float btnY = py + (itemH - 28) / 2.0f;
            sf::FloatRect playBtnR = {{btnX, btnY}, {68.0f, 28.0f}};
            if (playBtnR.contains({(float)x, (float)y})) {
                selectedPeer = (int)i;
                return OPTION_PEER_PLAY;
            }
        }
        return OPTION_NONE;
    }

    for (auto& item : items)
        if (item.bounds.contains({(float)x, (float)y}))
            return item.option;
    return OPTION_NONE;
}

bool Menu::handleSliderPress(int x, int y) {
    if (currentScreen != SCREEN_DIFFICULTY) return false;
    for (auto& s : sliders) {
        float thumbX = s.trackBounds.position.x + s.getNormalized() * s.trackBounds.size.x;
        float thumbY = s.trackBounds.position.y + s.trackBounds.size.y / 2.0f;
        float hitRadius = 20.0f;
        float dx = x - thumbX;
        float dy = y - thumbY;
        if (dx * dx + dy * dy <= hitRadius * hitRadius) {
            s.dragging = true;
            return true;
        }
        if (s.trackBounds.contains({(float)x, (float)y})) {
            float norm = (x - s.trackBounds.position.x) / s.trackBounds.size.x;
            s.setFromNormalized(norm);
            personalityPreset = -1;
            syncSettings();
            return true;
        }
    }
    return false;
}

bool Menu::isDraggingSlider() const {
    for (auto& s : sliders)
        if (s.dragging) return true;
    return false;
}

void Menu::drawShadow(sf::RenderWindow& window, const sf::FloatRect& bounds) {
    drawRoundedRect(window,
                    bounds.position.x + 3,
                    bounds.position.y + 4,
                    bounds.size.x, bounds.size.y,
                    CORNER_R, sf::Color(0, 0, 0, 60));
}

void Menu::drawCard(sf::RenderWindow& window, const sf::FloatRect& bounds, bool hovered) {
    if (hovered) {
        drawRoundedRect(window,
                        bounds.position.x - 2,
                        bounds.position.y - 2,
                        bounds.size.x + 4,
                        bounds.size.y + 4,
                        CORNER_R + 1,
                        sf::Color(212, 175, 55, 100));
    }

    drawRoundedRect(window,
                    bounds.position.x, bounds.position.y,
                    bounds.size.x, bounds.size.y,
                    CORNER_R,
                    hovered ? cardBgHover : cardBg);

    sf::RectangleShape innerGlow(sf::Vector2f(bounds.size.x - 4, bounds.size.y / 2.5f));
    innerGlow.setFillColor(sf::Color(255, 255, 255, hovered ? 12 : 6));
    innerGlow.setPosition(sf::Vector2f(bounds.position.x + 2, bounds.position.y + 2));
    window.draw(innerGlow);
}

void Menu::drawSlider(sf::RenderWindow& window, const SliderItem& slider, int idx) {
    int cx = winW / 2;
    int startY = winH / 5;
    int y = startY + 10 + idx * 65;

    if (!font) return;

    // Label
    sf::Text labelT = makeText(slider.label + ": " + slider.display(), 14, textColor, false);
    labelT.setPosition(sf::Vector2f((float)(cx - 160), (float)(y - 20)));
    window.draw(labelT);

    // Personality preset indicator
    if (idx == 0) {
        std::string presetStr = "Personality: " + getPersonalityName();
        sf::Text presetT = makeText(presetStr, 11, muted, true);
        presetT.setPosition(sf::Vector2f((float)(cx), (float)(y - 44)));
        window.draw(presetT);
    }

    // Track
    float trackX = slider.trackBounds.position.x;
    float trackY = slider.trackBounds.position.y;
    float trackW = slider.trackBounds.size.x;
    float trackH = slider.trackBounds.size.y;

    sf::RectangleShape trackBg(sf::Vector2f(trackW, trackH));
    trackBg.setFillColor(sf::Color(80, 75, 70, 180));
    trackBg.setPosition(sf::Vector2f(trackX, trackY));
    window.draw(trackBg);

    // Filled portion
    float fillW = slider.getNormalized() * trackW;
    if (fillW > 2.0f) {
        sf::RectangleShape trackFill(sf::Vector2f(fillW, trackH));
        trackFill.setFillColor(accent);
        trackFill.setPosition(sf::Vector2f(trackX, trackY));
        window.draw(trackFill);
    }

    // Thumb
    float thumbR = 10.0f;
    float thumbX = trackX + slider.getNormalized() * trackW;
    float thumbY = trackY + trackH / 2.0f;

    sf::CircleShape thumb(thumbR);
    thumb.setFillColor(slider.dragging ? sf::Color(240, 215, 120) : sf::Color(200, 175, 75));
    thumb.setOrigin(sf::Vector2f(thumbR, thumbR));
    thumb.setPosition(sf::Vector2f(thumbX, thumbY));
    window.draw(thumb);

    sf::CircleShape thumbOutline(thumbR + 1);
    thumbOutline.setFillColor(sf::Color::Transparent);
    thumbOutline.setOutlineColor(sf::Color(180, 150, 50, 120));
    thumbOutline.setOutlineThickness(1);
    thumbOutline.setOrigin(sf::Vector2f(thumbR + 1, thumbR + 1));
    thumbOutline.setPosition(sf::Vector2f(thumbX, thumbY));
    window.draw(thumbOutline);

    // Min/max labels
    sf::Text minT = makeText(slider.display(), 0, muted);
    sf::Text maxT = makeText(slider.display(), 0, muted);
}

void Menu::drawOrnaments(sf::RenderWindow& window) {
    float t = ornamentClock.getElapsedTime().asSeconds();
    int cx = winW / 2;
    int cy = winH / 7;
    float radius = 80.0f;
    int numDots = 12;
    for (int i = 0; i < numDots; ++i) {
        float angle = t * 0.3f + i * 6.2832f / numDots;
        float x = cx + std::cos(angle) * radius;
        float y = cy + std::sin(angle) * radius * 0.6f;
        float dotR = 2.0f + std::sin(t * 1.5f + i) * 1.0f;
        sf::Color dotColor = accent;
        dotColor.a = static_cast<std::uint8_t>(80 + std::sin(t * 0.8f + i * 0.5f) * 40);
        sf::CircleShape dot(dotR);
        dot.setFillColor(dotColor);
        dot.setPosition(sf::Vector2f(x - dotR, y - dotR));
        window.draw(dot);
    }
    // Larger orbiting ring
    float r2 = 110.0f;
    for (int i = 0; i < 6; ++i) {
        float angle = -t * 0.2f + i * 6.2832f / 6;
        float x = cx + std::cos(angle) * r2;
        float y = cy + std::sin(angle) * r2 * 0.5f;
        float dotR = 3.5f + std::sin(t * 1.2f + i * 0.7f) * 1.5f;
        sf::Color dotColor = muted;
        dotColor.a = static_cast<std::uint8_t>(60 + std::sin(t * 0.6f + i) * 30);
        sf::CircleShape dot(dotR);
        dot.setFillColor(dotColor);
        dot.setPosition(sf::Vector2f(x - dotR, y - dotR));
        window.draw(dot);
    }
}

void Menu::buildLobby() {
    items.clear();
    sliders.clear();
    currentScreen = SCREEN_LOBBY;
    items.push_back({"Local Network", "", {}, OPTION_NONE, false});
    updateBounds();
}

void Menu::drawLobby(sf::RenderWindow& window) {
    if (!font) return;
    int cx = winW / 2;

    // Back button (top-left)
    int backW = 80, backH = 30, backMargin = 12;
    sf::FloatRect backBounds = {{(float)backMargin, (float)backMargin}, {(float)backW, (float)backH}};
    bool backHover = backBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
    drawRoundedRect(window, backBounds.position.x, backBounds.position.y,
                    backW, backH, 8,
                    backHover ? sf::Color(70, 60, 50, 200) : sf::Color(50, 42, 35, 180));
    sf::Text backT = makeText("< Back", 12, backHover ? sf::Color(220, 200, 170) : sf::Color(160, 140, 120), false);
    sf::FloatRect btb = backT.getLocalBounds();
    backT.setPosition(sf::Vector2f(backBounds.position.x + backW / 2.0f - btb.size.x / 2.0f,
                                    backBounds.position.y + backH / 2.0f - btb.size.y / 2.0f - 1));
    window.draw(backT);

    // Store back button bounds for click handling
    closeBtnBounds = backBounds;
    closeBtnHovered = backHover;

    // Title
    sf::Text title = makeText("Local Network", 26, textColor, true);
    title.setPosition(sf::Vector2f((float)cx, (float)(winH / 8)));
    window.draw(title);

    // Player name
    std::string nameStr = "You: " + playerNameLabel;
    sf::Text nameT = makeText(nameStr, 14, muted, true);
    nameT.setPosition(sf::Vector2f((float)cx, (float)(winH / 8 + 40)));
    window.draw(nameT);

    // Status
    sf::Text status = makeText(statusText, 13, accent, true);
    status.setPosition(sf::Vector2f((float)cx, (float)(winH / 8 + 65)));
    window.draw(status);

    // Peers list
    int listY = winH / 4 + 10;
    int itemH = 48;
    int itemW = 340;

    sf::Text label = makeText("Players on Network:", 15, textColor, true);
    label.setPosition(sf::Vector2f((float)cx, (float)(listY - 30)));
    window.draw(label);

    if (peerNames.empty()) {
        sf::Text emptyT = makeText(connectionState == 1 ? "Searching..." : "No players found", 13, muted, true);
        emptyT.setPosition(sf::Vector2f((float)cx, (float)(listY + 20)));
        window.draw(emptyT);
    }

    for (size_t i = 0; i < peerNames.size(); ++i) {
        int y = listY + i * (itemH + 8);
        sf::FloatRect bounds = {{(float)(cx - itemW / 2), (float)y}, {(float)itemW, (float)itemH}};
        bool hover = (int)i == selectedPeer;

        drawRoundedRect(window, bounds.position.x, bounds.position.y,
                        bounds.size.x, bounds.size.y, 10,
                        hover ? cardBgHover : cardBg);

        sf::RectangleShape border(sf::Vector2f(bounds.size.x - 2, bounds.size.y - 2));
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(hover ? accent : sf::Color(100, 90, 80, 60));
        border.setOutlineThickness(1);
        border.setPosition(sf::Vector2f(bounds.position.x + 1, bounds.position.y + 1));
        window.draw(border);

        // Player icon
        sf::CircleShape icon(14);
        icon.setFillColor(accent);
        icon.setPosition(sf::Vector2f(bounds.position.x + 12, bounds.position.y + itemH / 2.0f - 14));
        window.draw(icon);

        sf::Text name = makeText(peerNames[i], 14, textColor, false);
        name.setPosition(sf::Vector2f(bounds.position.x + 40, bounds.position.y + 7));
        window.draw(name);

        sf::Text ipT = makeText(peerIPs[i], 10, muted, false);
        ipT.setPosition(sf::Vector2f(bounds.position.x + 40, bounds.position.y + 26));
        window.draw(ipT);

        // Play button
        float btnX = bounds.position.x + bounds.size.x - 80;
        float btnY = bounds.position.y + (itemH - 28) / 2.0f;
        sf::FloatRect playBtn = {{btnX, btnY}, {68.0f, 28.0f}};
        bool btnHover = playBtn.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
        drawRoundedRect(window, playBtn.position.x, playBtn.position.y,
                        playBtn.size.x, playBtn.size.y, 6,
                        btnHover ? sf::Color(70, 100, 70, 200) : sf::Color(50, 70, 50, 180));

        sf::Text playT = makeText("Play", 12, sf::Color(220, 240, 220, 230), false);
        sf::FloatRect ptb = playT.getLocalBounds();
        playT.setPosition(sf::Vector2f(playBtn.position.x + playBtn.size.x / 2.0f - ptb.size.x / 2.0f,
                                       playBtn.position.y + playBtn.size.y / 2.0f - ptb.size.y / 2.0f - 1));
        window.draw(playT);
    }
}

void Menu::draw(sf::RenderWindow& window) {
    applyTheme();

    sf::RectangleShape bgRect(sf::Vector2f((float)winW, (float)winH));
    bgRect.setFillColor(bg);
    window.draw(bgRect);

    sf::Color accentLine = accent;
    accentLine.a = 80;
    sf::RectangleShape topAccent(sf::Vector2f((float)winW, 2));
    topAccent.setFillColor(accentLine);
    topAccent.setPosition(sf::Vector2f(0, 0));
    window.draw(topAccent);

    sf::RectangleShape bottomAccent(sf::Vector2f((float)winW, 2));
    bottomAccent.setFillColor(accentLine);
    bottomAccent.setPosition(sf::Vector2f(0, (float)winH - 2));
    window.draw(bottomAccent);

    if (currentScreen != SCREEN_DIFFICULTY)
        drawOrnaments(window);

    for (size_t i = 0; i < items.size(); ++i) {
        auto& item = items[i];

        if (i == 0) {
            sf::Text t = makeText(item.text, currentScreen == SCREEN_DIFFICULTY ? 30 : 38, accent, true);
            t.setPosition(sf::Vector2f((float)winW / 2, item.bounds.position.y + item.bounds.size.y / 2));
            window.draw(t);

            if (currentScreen != SCREEN_DIFFICULTY) {
                sf::Text sub(*font, "A Chess Experience", 12);
                sub.setFillColor(muted);
                sf::FloatRect sb = sub.getLocalBounds();
                sub.setOrigin(sf::Vector2f(sb.size.x / 2, 0));
                sub.setPosition(sf::Vector2f((float)winW / 2, item.bounds.position.y + item.bounds.size.y + 4));
                window.draw(sub);
            }
            continue;
        }

        drawShadow(window, item.bounds);
        drawCard(window, item.bounds, item.hovered);

        // Special styling for "Play As" toggle
        if (item.option == OPTION_PLAY_AS_BLACK) {
            sf::Color playAsBg = item.hovered
                ? sf::Color(accent.r, accent.g, accent.b, 40)
                : sf::Color(accent.r, accent.g, accent.b, 20);
            drawRoundedRect(window, item.bounds.position.x + 1, item.bounds.position.y + 1,
                            item.bounds.size.x - 2, item.bounds.size.y - 2,
                            CORNER_R, playAsBg);

            sf::RectangleShape playAsBorder(sf::Vector2f(item.bounds.size.x - 4, item.bounds.size.y - 4));
            playAsBorder.setFillColor(sf::Color::Transparent);
            playAsBorder.setOutlineColor(item.hovered ? accent : sf::Color(accent.r, accent.g, accent.b, 120));
            playAsBorder.setOutlineThickness(1.5f);
            playAsBorder.setPosition(sf::Vector2f(item.bounds.position.x + 2, item.bounds.position.y + 2));
            window.draw(playAsBorder);

            sf::Text titleT = makeText(item.text, 16, item.hovered ? accent : textColor, false);
            titleT.setPosition(sf::Vector2f(item.bounds.position.x + 18, item.bounds.position.y + 8));
            window.draw(titleT);

            if (!item.subtitle.empty()) {
                sf::Text subT = makeText(item.subtitle, 11, muted, false);
                subT.setPosition(sf::Vector2f(item.bounds.position.x + 20, item.bounds.position.y + 28));
                window.draw(subT);
            }
            continue;
        }

        sf::Text titleT = makeText(item.text, currentScreen == SCREEN_DIFFICULTY ? 14 : 18,
                                   item.hovered ? accent : textColor, false);
        titleT.setPosition(sf::Vector2f(item.bounds.position.x + 18, item.bounds.position.y + 5));
        window.draw(titleT);

        if (!item.subtitle.empty()) {
            sf::Text subT = makeText(item.subtitle, 11, muted, false);
            sf::FloatRect sb = subT.getLocalBounds();
            subT.setOrigin(sf::Vector2f(0, 0));
            subT.setPosition(sf::Vector2f(item.bounds.position.x + 20, item.bounds.position.y + 26));
            window.draw(subT);
        }
    }

    // Draw sliders on difficulty screen
    if (currentScreen == SCREEN_DIFFICULTY) {
        for (size_t i = 0; i < sliders.size(); ++i)
            drawSlider(window, sliders[i], i);
    }

    // Draw lobby screen (full custom rendering instead of card items)
    if (currentScreen == SCREEN_LOBBY) {
        drawLobby(window);
        return;
    }

    // Close button (top-left corner) on sub-screens
    if (currentScreen != SCREEN_MAIN) {
        int btnSize = 32;
        int margin = 12;
        closeBtnBounds = sf::FloatRect({(float)margin, (float)margin},
                                        {(float)btnSize, (float)btnSize});
        closeBtnHovered = closeBtnBounds.contains(
            sf::Vector2f(sf::Mouse::getPosition(window)));

        sf::RectangleShape btn(sf::Vector2f((float)btnSize, (float)btnSize));
        btn.setFillColor(closeBtnHovered ? sf::Color(180, 70, 70, 200) : sf::Color(60, 55, 50, 180));
        btn.setPosition(sf::Vector2f((float)margin, (float)margin));
        window.draw(btn);

        if (font) {
            sf::Text x(*font, "X", 16);
            x.setFillColor(closeBtnHovered ? sf::Color(255, 220, 200) : sf::Color(200, 180, 160));
            sf::FloatRect xb = x.getLocalBounds();
            x.setOrigin(sf::Vector2f(xb.size.x / 2, xb.size.y / 2));
            x.setPosition(sf::Vector2f((float)(margin + btnSize / 2),
                                        (float)(margin + btnSize / 2 - 1)));
            window.draw(x);
        }
    }
}
