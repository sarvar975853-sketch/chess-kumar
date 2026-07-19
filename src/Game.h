#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <map>
#include <future>
#include "ChessBoard.h"
#include "ChessAI.h"
#include "AssetManager.h"
#include "Settings.h"
#include "OpeningBook.h"

class AudioManager;
class NetworkManager;

enum GameMode { MODE_LOCAL_MULTIPLAYER, MODE_VS_BOT, MODE_NETWORK };
enum GameResult { RESULT_NONE, RESULT_WHITE_WINS, RESULT_BLACK_WINS, RESULT_DRAW };
enum BotExpr { EXPR_NEUTRAL, EXPR_HAPPY, EXPR_THINKING, EXPR_SURPRISED, EXPR_SAD };

enum MoveAnnotation {
    ANNO_NONE, ANNO_BOOK, ANNO_BRILLIANT, ANNO_GREAT, ANNO_EXCELLENT,
    ANNO_GOOD, ANNO_INACCURACY, ANNO_MISTAKE, ANNO_BLUNDER
};

static const char* ANNOTATION_NAMES[] = {
    "", "Book", "Brilliant", "Great", "Excellent",
    "Good", "Inaccuracy", "Mistake", "Blunder"
};

struct CapturedPiece {
    PieceType type;
    PieceColor color;
};

struct PieceAnim {
    bool active = false;
    PieceType type = NONE;
    PieceColor color = NO_COLOR;
    int fromRow = 0, fromCol = 0;
    int toRow = 0, toCol = 0;
    sf::Clock clock;
    float duration = 0.18f;
};

struct BotDialog {
    std::string text;
    sf::Clock clock;
    float duration = 3.0f;
    bool active = false;
    BotExpr expression = EXPR_NEUTRAL;

    void show(const std::string& msg, BotExpr expr = EXPR_NEUTRAL, float dur = 3.0f) {
        text = msg;
        expression = expr;
        duration = dur;
        clock.restart();
        active = true;
    }

    bool expired() const {
        return !active || clock.getElapsedTime().asSeconds() >= duration;
    }

    void update() {
        if (active && expired()) active = false;
    }
};

struct BotDialogues {
    std::vector<std::string> greeting;
    std::vector<std::string> thinking;
    std::vector<std::string> move;
    std::vector<std::string> capture;
    std::vector<std::string> check;
    std::vector<std::string> checkmate;
    std::vector<std::string> win;
    std::vector<std::string> lose;
    std::vector<std::string> pieceLost;
    std::vector<std::string> draw;
};

class Game {
public:
    Game();
    void setAssetManager(AssetManager* mgr);
    void setAI(ChessAI* ai);
    void setAudioManager(AudioManager* a) { audio = a; }
    void setSettings(Settings* s) { settings = s; recalcLayout(); }
    void setMode(GameMode mode);
    GameMode getMode() const { return mode; }
    void setDifficulty(AIDifficulty diff);
    void setBotName(const std::string& name);
    void setPlayerColor(PieceColor color) { playerColor = color; }
    void setNetworkManager(NetworkManager* nm) { netMgr = nm; }
    void setWindowSize(int w, int h) { winW = w; winH = h; recalcLayout(); }
    void init();
    void reset();
    void handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::RenderWindow& window);
    void draw(sf::RenderWindow& window);
    bool isGameOver() const { return gameOver; }
    GameResult getResult() const { return result; }
    bool isAITurn() const { return aiThinking; }
    bool shouldExitToMenu() const { return exitToMenu; }
    void clearExitFlag() { exitToMenu = false; }
    void showExitConfirm() { exitConfirmActive = true; }
    void dismissExitConfirm() { exitConfirmActive = false; }
    bool isExitConfirmActive() const { return exitConfirmActive; }

    // Post-game review (public for main.cpp access)
    bool reviewMode = false;
    void enterReviewMode();
    void exitReviewMode();
    void reviewForward();
    void reviewBackward();

private:
    ChessBoard board;
    ChessAI* ai;
    AssetManager* assetManager;
    AudioManager* audio = nullptr;
    NetworkManager* netMgr = nullptr;
    Settings* settings = nullptr;
    GameMode mode;
    AIDifficulty difficulty;
    std::string botName;
    BotDialog botDialog;
    BotDialogues botLines;
    sf::Texture botAvatar;

    int boardX, boardY, squareSize, boardSize;
    int winW = 900, winH = 700;
    bool portrait = false;
    int panelWidth = 250;

    int selectedRow, selectedCol;
    bool pieceSelected;

    int lastFromRow, lastFromCol, lastToRow, lastToCol;
    bool hasLastMove;

    PieceColor currentPlayer;
    PieceColor playerColor = WHITE;
    bool gameOver, checkmate, stalemate;
    GameResult result;
    int totalMoves;
    bool aiThinking;
    bool aiMoveReady;
    Move pendingAiMove;
    sf::Clock aiDelayClock;
    sf::Clock undoCooldown;
    std::future<Move> aiFuture;

    bool awaitingPromotion;
    int promoFromRow, promoFromCol, promoToRow, promoToCol;

    std::vector<CapturedPiece> capturedWhite;
    std::vector<CapturedPiece> capturedBlack;

    bool leftMouseDown;

    bool dragging = false;
    int dragRow = 0, dragCol = 0;
    sf::Vector2i dragMousePos;

    bool rightDragging = false;
    int rightDragFromRow = 0, rightDragFromCol = 0;
    int rightDragToRow = 0, rightDragToCol = 0;
    sf::Clock rightDragClock;
    bool showRightDragArrow = false;

    PieceAnim anim;

    bool exitToMenu;
    sf::FloatRect exitBtnBounds;
    sf::FloatRect playAgainBtnBounds;
    bool hoverExitBtn;
    bool hoverPlayAgain;

    bool exitConfirmActive = false;
    sf::FloatRect exitConfirmYesBounds;
    sf::FloatRect exitConfirmNoBounds;

    void buildBotLines();
    void loadBotAvatar();
    std::string pickPhrase(const std::vector<std::string>& phrases) const;
    void recalcLayout();

    void trackCapturedPiece(const Move& move);
    void doAIMove();
    void completeMove(const Move& move);

    void boardToScreen(int row, int col, int& sx, int& sy) const;
    bool screenToBoard(int sx, int sy, int& row, int& col) const;

    void drawBoard(sf::RenderWindow& window);
    void drawLastMove(sf::RenderWindow& window);
    void drawPieces(sf::RenderWindow& window);
    void drawCaptureFlash(sf::RenderWindow& window);
    void drawSelection(sf::RenderWindow& window);
    void drawSidePanel(sf::RenderWindow& window);
    void drawCapturedPiecesAt(sf::RenderWindow& window, const std::vector<CapturedPiece>& pieces,
                               int x, int y, PieceColor side);
    void drawThinking(sf::RenderWindow& window);
    void drawPromotionDialog(sf::RenderWindow& window);
    void drawGameOver(sf::RenderWindow& window);
    void drawSpeechBubble(sf::RenderWindow& window, const std::string& text,
                          float bx, float by, float maxWidth);

    void handleDrop(int mx, int my, sf::RenderWindow& window);
    void handlePromotionClick(int x, int y);
    void drawRightDragArrow(sf::RenderWindow& window);
    void drawExitConfirm(sf::RenderWindow& window);
    void drawCoordinates(sf::RenderWindow& window);
    void drawMoveHistory(sf::RenderWindow& window);
    void drawHint(sf::RenderWindow& window);
    std::string moveToAlgebraic(const Move& move);
    void undoLastMove();
    void showHint();
    void clearHint();

    // Move analysis
    void analyzeLastMove();
    std::string annotationToString(MoveAnnotation a) const;

    // Game save/load
    void saveGame() const;
    static std::string gamesDir();

    // Post-game review (private state)
    int reviewIndex = 0;
    std::vector<MoveAnnotation> annotations;
    std::vector<BoardState> reviewBoardStates;
    void drawReviewOverlay(sf::RenderWindow& window);

    struct Particle {
        float x, y, vx, vy, life, maxLife, size;
        sf::Color color;
    };
    std::vector<Particle> particles;
    void emitParticles(float x, float y, sf::Color color, int count = 8);
    void emitCaptureParticles(int row, int col);
    void emitMoveParticles(int fromRow, int fromCol, int toRow, int toCol);
    void updateParticles();
    void drawParticles(sf::RenderWindow& window);

    float shakeIntensity = 0;
    float shakeMaxIntensity = 0;
    sf::Clock shakeClock;
    void triggerShake(float intensity);
    sf::Vector2f getShakeOffset();

    // Move highlight pulse
    sf::Clock moveHighlightClock;

    // Check king pulse
    sf::Clock checkPulseClock;

    // Game over fade-in
    float gameOverAlpha = 0;
    sf::Clock gameOverClock;

    // Board flip animation
    bool flipAnimating = false;
    float flipProgress = 0;
    sf::Clock flipClock;
    static constexpr float FLIP_DURATION = 0.35f;

    // Capture flash
    sf::Clock captureFlashClock;
    int captureFlashRow = -1, captureFlashCol = -1;

    // Move trail
    struct MoveTrail {
        int fromRow, fromCol, toRow, toCol;
        sf::Clock clock;
        float duration = 0.4f;
        sf::Color color;
        bool active = false;
    };
    MoveTrail moveTrail;

    // Board entrance
    sf::Clock boardEntranceClock;
    bool boardEntranceDone = false;

    bool showCoordinates = true;
    bool boardFlipped = false;
    bool hintActive = false;
    std::string hintText;
    std::vector<std::string> moveHistory;
    std::vector<Move> openingHistory;
    std::vector<std::pair<Move, BoardState>> undoStack;
    OpeningBook openingBook;
    int maxUndo = 0;
    std::map<std::string, int> positionHistory;
    bool drawByFiftyMove = false;
    bool drawByRepetition = false;
    bool drawByInsufficientMaterial = false;

    sf::Clock gameClock;

    int capturesByWhite = 0;
    int capturesByBlack = 0;

    sf::FloatRect undoBtnBounds;
    sf::FloatRect hintBtnBounds;
    sf::FloatRect flipBtnBounds;
    sf::FloatRect coordBtnBounds;
    sf::FloatRect reviewBtnBounds;
    sf::FloatRect reviewPrevBounds;
    sf::FloatRect reviewNextBounds;
    sf::FloatRect reviewExitBounds;
};

#endif
