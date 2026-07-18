#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <libgen.h>
#include <mach-o/dyld.h>
#include "ChessBoard.h"
#include "ChessAI.h"
#include "AssetManager.h"
#include "Menu.h"
#include "Game.h"
#include "AudioManager.h"
#include "Settings.h"
#include "NetworkManager.h"



enum AppState { STATE_MENU, STATE_BOT_SELECT, STATE_SETTINGS, STATE_DIFFICULTY, STATE_LOBBY, STATE_PLAYING, STATE_GAME_OVER };

static AIDifficulty menuOptToDiff(MenuOption opt) {
    switch (opt) {
        case OPTION_BOT_PAWN:   return BOT_PAWN;
        case OPTION_BOT_KNIGHT: return BOT_KNIGHT;
        case OPTION_BOT_BISHOP: return BOT_BISHOP;
        case OPTION_BOT_ROOK:   return BOT_ROOK;
        case OPTION_BOT_QUEEN:  return BOT_QUEEN;
        default:                return BOT_PAWN;
    }
}

static void setupBotFromSettings(Settings& settings, MenuOption opt) {
    AIDifficulty diff = menuOptToDiff(opt);
    settings.aiPersonality = (int)diff;
    switch (diff) {
        case BOT_PAWN:   settings.aiDepth = 2; settings.aiBlunderRate = 25; settings.aiAggression = 80; break;
        case BOT_KNIGHT: settings.aiDepth = 3; settings.aiBlunderRate = 12; settings.aiAggression = 50; break;
        case BOT_BISHOP: settings.aiDepth = 4; settings.aiBlunderRate = 5;  settings.aiAggression = 30; break;
        case BOT_ROOK:   settings.aiDepth = 6; settings.aiBlunderRate = 2;  settings.aiAggression = 60; break;
        case BOT_QUEEN:  settings.aiDepth = 8; settings.aiBlunderRate = 0;  settings.aiAggression = 100; break;
    }
}

static void startGameFromSettings(ChessAI*& ai, Game& game, Settings& settings) {
    AIDifficulty diff;
    if (settings.aiPersonality < 0) {
        // Custom — use a reasonable base difficulty
        if (settings.aiDepth <= 2) diff = BOT_PAWN;
        else if (settings.aiDepth <= 4) diff = BOT_KNIGHT;
        else if (settings.aiDepth <= 6) diff = BOT_ROOK;
        else diff = BOT_QUEEN;
    } else {
        diff = (AIDifficulty)settings.aiPersonality;
    }
    delete ai;
    ai = new ChessAI(diff);
    ai->setDynamicParams(settings.aiDepth, settings.aiBlunderRate, settings.aiAggression, 50);
    game.setAI(ai);
    game.setMode(MODE_VS_BOT);
    game.setDifficulty(diff);
    game.setBotName(ai->getBotName());
    game.init();
}

// ── App icon generation ──

static void drawFilledCircle(sf::RenderTexture& rt, float cx, float cy, float r, const sf::Color& color) {
    sf::CircleShape c(r);
    c.setFillColor(color);
    c.setOrigin(sf::Vector2f(r, r));
    c.setPosition(sf::Vector2f(cx, cy));
    rt.draw(c);
}

static void drawKingShape(sf::RenderTexture& rt, float cx, float cy, float s, const sf::Color& color) {
    // Crown base
    sf::ConvexShape crown;
    crown.setPointCount(10);
    crown.setPoint(0, sf::Vector2f(cx - s*0.55f, cy + s*0.15f));
    crown.setPoint(1, sf::Vector2f(cx - s*0.55f, cy - s*0.15f));
    crown.setPoint(2, sf::Vector2f(cx - s*0.35f, cy - s*0.05f));
    crown.setPoint(3, sf::Vector2f(cx - s*0.2f,  cy - s*0.35f));
    crown.setPoint(4, sf::Vector2f(cx,          cy - s*0.1f));
    crown.setPoint(5, sf::Vector2f(cx + s*0.2f,  cy - s*0.35f));
    crown.setPoint(6, sf::Vector2f(cx + s*0.35f, cy - s*0.05f));
    crown.setPoint(7, sf::Vector2f(cx + s*0.55f, cy - s*0.15f));
    crown.setPoint(8, sf::Vector2f(cx + s*0.55f, cy + s*0.15f));
    crown.setFillColor(color);
    rt.draw(crown);

    // Cross on top
    sf::RectangleShape crossV(sf::Vector2f(s*0.08f, s*0.35f));
    crossV.setFillColor(color);
    crossV.setOrigin(sf::Vector2f(s*0.04f, s*0.35f));
    crossV.setPosition(sf::Vector2f(cx, cy - s*0.35f));
    rt.draw(crossV);

    sf::RectangleShape crossH(sf::Vector2f(s*0.3f, s*0.07f));
    crossH.setFillColor(color);
    crossH.setOrigin(sf::Vector2f(s*0.15f, s*0.035f));
    crossH.setPosition(sf::Vector2f(cx, cy - s*0.52f));
    rt.draw(crossH);

    // Neck
    sf::RectangleShape neck(sf::Vector2f(s*0.3f, s*0.25f));
    neck.setFillColor(color);
    neck.setOrigin(sf::Vector2f(s*0.15f, 0));
    neck.setPosition(sf::Vector2f(cx, cy + s*0.15f));
    rt.draw(neck);

    // Base
    sf::RectangleShape base(sf::Vector2f(s*0.9f, s*0.1f));
    base.setFillColor(color);
    base.setOrigin(sf::Vector2f(s*0.45f, 0));
    base.setPosition(sf::Vector2f(cx, cy + s*0.38f));
    rt.draw(base);

    sf::RectangleShape base2(sf::Vector2f(s*1.0f, s*0.06f));
    base2.setFillColor(color);
    base2.setOrigin(sf::Vector2f(s*0.5f, 0));
    base2.setPosition(sf::Vector2f(cx, cy + s*0.46f));
    rt.draw(base2);
}

static sf::Image generateAppIcon() {
    sf::RenderTexture rt;
    rt.resize({256, 256});
    rt.clear(sf::Color::Transparent);

    float cx = 128, cy = 128;

    // Dark round background
    drawFilledCircle(rt, cx, cy, 124, sf::Color(35, 28, 22));

    // Outer gold ring
    sf::CircleShape ring(124);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(180, 150, 75, 200));
    ring.setOutlineThickness(4);
    ring.setOrigin(sf::Vector2f(124, 124));
    ring.setPosition(sf::Vector2f(cx, cy));
    rt.draw(ring);

    // Inner ring
    sf::CircleShape ring2(112);
    ring2.setFillColor(sf::Color::Transparent);
    ring2.setOutlineColor(sf::Color(180, 150, 75, 120));
    ring2.setOutlineThickness(2);
    ring2.setOrigin(sf::Vector2f(112, 112));
    ring2.setPosition(sf::Vector2f(cx, cy));
    rt.draw(ring2);

    // Inner dark area
    drawFilledCircle(rt, cx, cy, 105, sf::Color(25, 20, 18));

    // Ornamental dots at compass points
    for (float angle = 0; angle < 360; angle += 45) {
        float rad = angle * 3.14159f / 180.0f;
        float dx = cx + std::cos(rad) * 115;
        float dy = cy + std::sin(rad) * 115;
        drawFilledCircle(rt, dx, dy, 3, sf::Color(212, 175, 55, 200));
    }

    // Draw the king piece (vintage gold)
    drawKingShape(rt, cx, cy - 8, 65, sf::Color(212, 175, 55));

    // Shadow under king
    drawKingShape(rt, cx + 2, cy - 6, 65, sf::Color(0, 0, 0, 50));

    // Gold highlight on king (slightly offset smaller king)
    drawKingShape(rt, cx - 1, cy - 10, 62, sf::Color(240, 210, 120));

    // Decorative arc above king
    sf::CircleShape arc(55);
    arc.setFillColor(sf::Color::Transparent);
    arc.setOutlineColor(sf::Color(212, 175, 55, 100));
    arc.setOutlineThickness(1.5f);
    arc.setOrigin(sf::Vector2f(55, 55));
    arc.setPosition(sf::Vector2f(cx, cy + 30));
    rt.draw(arc);

    // Bottom decorative line
    sf::RectangleShape line(sf::Vector2f(120, 1.5f));
    line.setFillColor(sf::Color(180, 150, 75, 120));
    line.setOrigin(sf::Vector2f(60, 0.75f));
    line.setPosition(sf::Vector2f(cx, cy + 70));
    rt.draw(line);

    // Text "CHESS" below
    sf::Font iconFont;
    if (iconFont.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
        sf::Text t(iconFont, "CHESS", 16);
        t.setLetterSpacing(4.0f);
        t.setFillColor(sf::Color(180, 150, 75, 180));
        sf::FloatRect tb = t.getLocalBounds();
        t.setOrigin(sf::Vector2f(tb.position.x + tb.size.x / 2.0f, tb.position.y + tb.size.y / 2.0f));
        t.setPosition(sf::Vector2f(cx, cy + 97));
        rt.draw(t);
    }

    rt.display();
    sf::Image img = rt.getTexture().copyToImage();
    img.saveToFile("assets/app_icon.png");
    return img;
}

// ── Rounded rect helper for menu ──

int main() {
    // Change to executable's own directory so relative asset paths work from any cwd
    char execPath[4096];
    uint32_t pathSize = sizeof(execPath);
    if (_NSGetExecutablePath(execPath, &pathSize) == 0) {
        char* dir = dirname(execPath);
        chdir(dir);
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    Settings settings;
    settings.load();

    sf::VideoMode startMode = settings.fullscreen ? sf::VideoMode::getDesktopMode()
                                                   : sf::VideoMode({900, 700});
    sf::RenderWindow window(startMode, "Chess Kumar",
                            settings.fullscreen ? sf::Style::Default : (sf::Style::Titlebar | sf::Style::Close),
                            settings.fullscreen ? sf::State::Fullscreen : sf::State::Windowed);
    window.setFramerateLimit(60);

    // Set app icon
    sf::Image appIcon = generateAppIcon();
    if (appIcon.getSize().x > 0)
        window.setIcon(appIcon);

    AssetManager assetMgr;
    assetMgr.setSquareSize(64);
    assetMgr.initialize();

    sf::Vector2u startSize = window.getSize();
    sf::View defaultView(sf::FloatRect({0, 0}, {(float)startSize.x, (float)startSize.y}));
    window.setView(defaultView);

    AudioManager audio;
    audio.applySettings(settings);
    audio.startMusic();

    Menu menu;
    menu.setFont(assetMgr.getFont());
    menu.setSettings(&settings);
    menu.setWindowSize(startSize.x, startSize.y);

    NetworkManager netMgr;
    netMgr.onStatusMessage = [&](const std::string& msg) {
        menu.setStatusText(msg);
    };

    Game game;
    game.setAssetManager(&assetMgr);
    game.setAudioManager(&audio);
    game.setNetworkManager(&netMgr);
    game.setSettings(&settings);
    game.setWindowSize(startSize.x, startSize.y);

    ChessAI* ai = nullptr;
    AppState state = STATE_MENU;
    bool running = true;

    bool wasLeftDown = false;

    while (running && window.isOpen()) {
        while (auto eventOpt = window.pollEvent()) {
            const auto& event = *eventOpt;

            if (event.is<sf::Event::Closed>()) {
                if (state == STATE_PLAYING || state == STATE_GAME_OVER)
                    game.showExitConfirm();
                else {
                    running = false;
                    window.close();
                }
            }

            if (auto* key = event.getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) {
                    if (game.isExitConfirmActive()) {
                        game.dismissExitConfirm();
                    } else if (game.reviewMode) {
                        game.exitReviewMode();
                    } else if (state == STATE_PLAYING || state == STATE_GAME_OVER) {
                        game.showExitConfirm();
                    } else if (state == STATE_LOBBY) {
                        netMgr.stopDiscovery();
                        state = STATE_MENU;
                        menu.setScreen(SCREEN_MAIN);
                    } else if (state == STATE_BOT_SELECT || state == STATE_SETTINGS || state == STATE_DIFFICULTY) {
                        state = STATE_MENU;
                        menu.setScreen(SCREEN_MAIN);
                    } else {
                        running = false;
                    }
                }
                // Review mode arrow keys
                if (game.reviewMode) {
                    if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::Up) {
                        game.reviewBackward();
                    } else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::Down) {
                        game.reviewForward();
                    }
                }
            }

            if (state == STATE_PLAYING || state == STATE_GAME_OVER) {
                game.handleEvent(event, window);
            }

            if (auto* mouse = event.getIf<sf::Event::MouseMoved>()) {
                if (state == STATE_MENU || state == STATE_BOT_SELECT || state == STATE_SETTINGS || state == STATE_LOBBY || state == STATE_DIFFICULTY)
                    menu.handleMouseMove(mouse->position.x, mouse->position.y);
            }

            if (auto* click = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (state == STATE_DIFFICULTY && click->button == sf::Mouse::Button::Left) {
                    if (menu.isDraggingSlider())
                        menu.endSliderDrag();
                    else
                        menu.handleSliderPress(click->position.x, click->position.y);
                }
            }

            if (auto* release = event.getIf<sf::Event::MouseButtonReleased>()) {
                if (state == STATE_DIFFICULTY && release->button == sf::Mouse::Button::Left)
                    menu.endSliderDrag();
            }
        }

        // Polling fallback: detect clicks even when SFML macOS events are broken
        sf::Vector2i mpos = sf::Mouse::getPosition(window);
        bool leftDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        bool leftEdge = leftDown && !wasLeftDown;
        bool leftRelease = !leftDown && wasLeftDown;
        wasLeftDown = leftDown;

        // For gameplay: forward synthetic press/release events to game
        if (state == STATE_PLAYING || state == STATE_GAME_OVER) {
            if (leftEdge) {
                sf::Event::MouseButtonPressed mbp;
                mbp.button = sf::Mouse::Button::Left;
                mbp.position = mpos;
                sf::Event pressEvent(mbp);
                game.handleEvent(pressEvent, window);
            }
            if (leftRelease) {
                sf::Event::MouseButtonReleased mbr;
                mbr.button = sf::Mouse::Button::Left;
                mbr.position = mpos;
                sf::Event releaseEvent(mbr);
                game.handleEvent(releaseEvent, window);
            }
        }

        // For menus: handle clicks via polling
        if (leftEdge && (state == STATE_MENU || state == STATE_BOT_SELECT || state == STATE_SETTINGS || state == STATE_DIFFICULTY || state == STATE_LOBBY)) {
            sf::Vector2i mpos = sf::Mouse::getPosition(window);
            int mx = mpos.x, my = mpos.y;

            if (state == STATE_DIFFICULTY) {
                menu.handleSliderPress(mx, my);
            }

            MenuOption opt = menu.handleClick(mx, my);
            if (opt != OPTION_NONE) {
                if (state == STATE_MENU) {
                    if (opt == OPTION_LOCAL_NETWORK) {
                        state = STATE_LOBBY;
                        menu.setPlayerName(netMgr.getPlayerName());
                        menu.setScreen(SCREEN_LOBBY);
                        netMgr.startDiscovery();
                    } else if (opt == OPTION_PLAY_VS_BOT) {
                        state = STATE_BOT_SELECT;
                        menu.setScreen(SCREEN_BOT_SELECT);
                    } else if (opt == OPTION_LOCAL_MULTIPLAYER) {
                        game.setMode(MODE_LOCAL_MULTIPLAYER);
                        game.init();
                        state = STATE_PLAYING;
                    } else if (opt == OPTION_SETTINGS) {
                        state = STATE_SETTINGS;
                        menu.setScreen(SCREEN_SETTINGS);
                    } else if (opt == OPTION_QUIT) {
                        running = false;
                    }
                } else if (state == STATE_BOT_SELECT) {
                    if (opt == OPTION_QUIT) {
                        state = STATE_MENU;
                        menu.setScreen(SCREEN_MAIN);
                    } else if (opt >= OPTION_BOT_PAWN && opt <= OPTION_BOT_QUEEN) {
                        setupBotFromSettings(settings, opt);
                        game.setPlayerColor(menu.isPlayAsBlack() ? BLACK : WHITE);
                        startGameFromSettings(ai, game, settings);
                        state = STATE_PLAYING;
                    } else if (opt == OPTION_CUSTOM_BOT) {
                        settings.aiPersonality = -1;
                        menu.setScreen(SCREEN_DIFFICULTY);
                        state = STATE_DIFFICULTY;
                    } else if (opt == OPTION_PLAY_AS_BLACK) {
                        menu.togglePlayAsBlack();
                        menu.setScreen(SCREEN_BOT_SELECT);
                    }
                } else if (state == STATE_DIFFICULTY) {
                    if (opt == OPTION_BACK_TO_BOT_SELECT) {
                        state = STATE_BOT_SELECT;
                        menu.setScreen(SCREEN_BOT_SELECT);
                    } else if (opt == OPTION_START_GAME) {
                        game.setPlayerColor(menu.isPlayAsBlack() ? BLACK : WHITE);
                        startGameFromSettings(ai, game, settings);
                        state = STATE_PLAYING;
                    } else if (opt == OPTION_QUIT) {
                        state = STATE_MENU;
                        menu.setScreen(SCREEN_MAIN);
                    }
                } else if (state == STATE_LOBBY) {
                    if (opt == OPTION_QUIT) {
                        netMgr.stopDiscovery();
                        state = STATE_MENU;
                        menu.setScreen(SCREEN_MAIN);
                    } else if (opt == OPTION_PEER_PLAY) {
                        std::string ip = menu.getSelectedPeerIP();
                        if (!ip.empty() && netMgr.connectToPeer(ip)) {
                            game.setMode(MODE_NETWORK);
                            game.init();
                            state = STATE_PLAYING;
                        }
                    }
                } else if (state == STATE_SETTINGS) {
                    if (opt == OPTION_QUIT) {
                        state = STATE_MENU;
                        menu.setScreen(SCREEN_MAIN);
                    } else if (opt == OPTION_SOUND_TOGGLE) {
                        settings.soundEnabled = !settings.soundEnabled;
                        audio.applySettings(settings);
                        settings.save();
                        menu.setScreen(SCREEN_SETTINGS);
                    } else if (opt == OPTION_MUSIC_TOGGLE) {
                        settings.musicEnabled = !settings.musicEnabled;
                        audio.applySettings(settings);
                        if (settings.musicEnabled) audio.startMusic();
                        settings.save();
                        menu.setScreen(SCREEN_SETTINGS);
                    } else if (opt == OPTION_THEME_TOGGLE) {
                        settings.darkMode = !settings.darkMode;
                        settings.save();
                        menu.setScreen(SCREEN_SETTINGS);
                    } else if (opt == OPTION_BOARD_THEME) {
                        settings.cycleTheme();
                        settings.save();
                        menu.setScreen(SCREEN_SETTINGS);
                    } else if (opt == OPTION_FULLSCREEN) {
                        settings.fullscreen = !settings.fullscreen;
                        settings.save();
                        window.close();
                        if (settings.fullscreen) {
                            sf::VideoMode vm = sf::VideoMode::getDesktopMode();
                            window.create(vm, "Chess Kumar",
                                          sf::Style::Default, sf::State::Fullscreen);
                        } else {
                            window.create(sf::VideoMode({900, 700}), "Chess Kumar",
                                          sf::Style::Titlebar | sf::Style::Close);
                        }
                        window.setFramerateLimit(60);
                        sf::Vector2u ws = window.getSize();
                        sf::View newView(sf::FloatRect({0, 0}, {(float)ws.x, (float)ws.y}));
                        window.setView(newView);
                        menu.setWindowSize(ws.x, ws.y);
                        game.setWindowSize(ws.x, ws.y);
                        menu.setScreen(SCREEN_SETTINGS);
                    }
                }
            }
        }

        if (game.shouldExitToMenu()) {
            game.clearExitFlag();
            if (game.getMode() == MODE_NETWORK)
                netMgr.disconnect();
            state = STATE_MENU;
            game.reset();
            delete ai; ai = nullptr;
            menu.reset();
        }

        netMgr.updateDiscovery();
        netMgr.updateConnection();

        if (state == STATE_PLAYING)
            game.update(window);

        // If network disconnected during gameplay, return to lobby
        if (state == STATE_PLAYING && game.getMode() == MODE_NETWORK && !netMgr.isConnected()) {
            state = STATE_LOBBY;
            menu.setConnectionState(0);
            menu.setStatusText("Disconnected — searching...");
            menu.setScreen(SCREEN_LOBBY);
            netMgr.startDiscovery();
        }

        audio.updateMusic();

        // Update lobby peer list
        if (state == STATE_LOBBY) {
            std::vector<std::string> names, ips;
            for (auto& p : netMgr.getPeers()) {
                names.push_back(p.name);
                ips.push_back(p.ip);
            }
            menu.setPeers(names, ips);
            menu.setConnectionState((int)netMgr.getState());
            menu.setStatusText(netMgr.getStatusText());
            menu.setPlayerName(netMgr.getPlayerName());
        }

        window.clear(sf::Color(25, 20, 18));
        switch (state) {
            case STATE_MENU:
            case STATE_BOT_SELECT:
            case STATE_SETTINGS:
            case STATE_DIFFICULTY:
            case STATE_LOBBY:
                menu.draw(window);
                break;
            case STATE_PLAYING:
            case STATE_GAME_OVER:
                game.draw(window);
                break;
        }
        window.display();
    }

    audio.shutdown();
    delete ai;
    return 0;
}
