#include "Game.h"
#include "AudioManager.h"
#include "NetworkManager.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cfloat>
#include <cstdint>
#include <ctime>
#include <sys/stat.h>
#include <algorithm>

static void drawFilledRoundRect(sf::RenderWindow& window, float x, float y, float w, float h,
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

static float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3) + c1 * std::pow(t - 1.0f, 2);
}

static std::string pick(const std::vector<std::string>& opts) {
    if (opts.empty()) return "";
    return opts[std::rand() % opts.size()];
}

Game::Game()
    : ai(nullptr), assetManager(nullptr), mode(MODE_LOCAL_MULTIPLAYER),
      difficulty(MEDIUM), botName("Bot"),
      boardX(40), boardY(40), squareSize(80), boardSize(640),
      selectedRow(0), selectedCol(0), pieceSelected(false),
      lastFromRow(-1), lastFromCol(-1), lastToRow(-1), lastToCol(-1), hasLastMove(false),
      currentPlayer(WHITE), gameOver(false), checkmate(false), stalemate(false),
      result(RESULT_NONE), totalMoves(0), aiThinking(false), aiMoveReady(false),
      awaitingPromotion(false),
      promoFromRow(0), promoFromCol(0), promoToRow(0), promoToCol(0),
      leftMouseDown(false), exitToMenu(false),
      hoverExitBtn(false), hoverPlayAgain(false) {
    board.init();
}

void Game::setAssetManager(AssetManager* mgr) { assetManager = mgr; }
void Game::setAI(ChessAI* aiPtr) { ai = aiPtr; }
void Game::setMode(GameMode gm) { mode = gm; }
void Game::setDifficulty(AIDifficulty diff) {
    difficulty = diff;
    switch (diff) {
        case EASY:   maxUndo = 999; break;
        case MEDIUM: maxUndo = 3; break;
        default:     maxUndo = 0; break;
    }
}

static sf::Color darkBg() { return sf::Color(25, 20, 18); }
static sf::Color lightBg() { return sf::Color(235, 228, 218); }
static sf::Color panelBg(bool dark) { return dark ? sf::Color(30, 25, 22, 200) : sf::Color(255, 252, 248, 220); }
static sf::Color panelText(bool dark) { return dark ? sf::Color(220, 200, 170) : sf::Color(50, 45, 40); }
static sf::Color panelMuted(bool dark) { return dark ? sf::Color(140, 120, 90, 180) : sf::Color(130, 120, 110, 180); }
static sf::Color accent() { return sf::Color(212, 175, 55); }

void Game::recalcLayout() {
    portrait = winH > winW;
    if (portrait) {
        int margin = 10;
        squareSize = (winW - margin * 2) / 8;
        if (squareSize > 55) squareSize = 55;
        boardSize = squareSize * 8;
        boardX = (winW - boardSize) / 2;
        boardY = 10;
        panelWidth = winW - margin * 2;
    } else {
        squareSize = 75;
        int neededW = 40 + squareSize * 8 + 20 + 250 + 20;
        if (neededW > winW) squareSize = (winW - 40 - 20 - 250 - 20) / 8;
        boardSize = squareSize * 8;
        boardX = 25;
        boardY = (winH - boardSize) / 2;
        if (boardY < 15) boardY = 15;
        panelWidth = 250;
    }
    if (assetManager) assetManager->setSquareSize(squareSize);

    // Pre-initialize button bounds so they are valid before first draw()
    int cx = boardX + boardSize / 2;
    int cy = boardY + boardSize / 2;
    playAgainBtnBounds = sf::FloatRect({(float)(cx - 130), (float)(cy + 20)}, {260.0f, 36.0f});
    int px = boardX + boardSize + 18;
    float exitY = boardY + boardSize - 42;
    exitBtnBounds = sf::FloatRect({(float)(px + 10), exitY}, {230.0f, 32.0f});
}

void Game::setBotName(const std::string& name) {
    botName = name;
    buildBotLines();
    loadBotAvatar();
}

void Game::buildBotLines() {
    if (botName == "Pauly") {
        botLines = {
            {"Hey!", "Hi there!", "Let's play!", "I'm Pauly!", "Welcome!", "Ready when you are!", 
             "This is gonna be fun!", "Hello hello!", "Pauly's here!", "Let's have a good game!",
             "I've been practicing!", "You won't know what hit you!", "Feeling lucky today?"},
            {"Umm...", "Let me think...", "Hmm...", "Okay...", "Wait...", "Let me calculate...",
             "So many choices...", "Which move is best...", "I need a moment...", "This is tricky...",
             "Don't rush me!", "Thinking cap on!", "Almost...", "One moment please..."},
            {"Here I go!", "Take that!", "Your turn!", "Oops!", "Moving along!", "There we go!",
             "How's that?", "Not bad, huh?", "Simple enough!", "Onwards!", "Your move now!",
             "Piece moved!", "Done and done!", "Smooth move!"},
            {"Got one!", "Yay!", "Ha ha! I got your piece!", "Mine now!", "Captured! So good!",
             "That piece is mine!", "One for me!", "Nice capture!", "I'll take that!",
             "Bye bye piece!", "You won't need that!", "Collecting souvenirs!", "Yes! Got it!"},
            {"Check!", "Hehe, check!", "In check!", "Your king is in trouble!", "Check on you!",
             "King's not safe!", "Uh oh for you!", "I love saying check!", "Check-a-roo!",
             "Your majesty is exposed!", "Better move that king!", "Check incoming!"},
            {"Checkmate!", "I won!", "Checkmate! Yay!", "I did it!", "Victory is mine!",
             "Checkmate! I'm so happy!", "Woo hoo! I win!", "That's checkmate!",
             "Game over! I'm the champ!", "What a finish!", "Couldn't have done it better!",
             "Checkmate! This feels great!"},
            {"I win!", "I'm the best!", "Pauly wins!", "So proud of myself!", "What a game!",
             "Winning is fun!", "Another victory!", "Too good today!", "I'm on fire!",
             "Nailed it!", "Victory dance!", "Champion Pauly!"},
            {"Aww...", "Good game!", "You're too good!", "I'll get you next time!", "So close!",
             "You played really well!", "Okay you win this round!", "Rematch rematch!",
             "I almost had you!", "That was fun though!", "You're really strong!",
             "I need more practice!", "GG! Let's go again!"},
            {"Hey! Not my piece!", "Aww, my piece...", "Nooo!", "My poor piece!", "Why always me?",
             "I liked that piece!", "That hurts!", "You're too good at capturing!",
             "One of my best pieces too!", "I'll miss you little piece!", "RIP my piece!",
             "Alright, revenge time!", "You'll pay for that!"},
            {"A draw!", "We tied!", "Fair enough!", "A draw is okay!", "We're evenly matched!",
             "Neither wins, neither loses!", "Tie game!", "So close to winning!",
             "Fair result!", "I'll take a draw!", "Good battle!", "We should go again!"}
        };
    } else if (botName == "Knightly") {
        botLines = {
            {"En garde!", "I challenge you!", "Ready to lose?", "Knightly approaches!", 
             "Prepare for battle!", "The knight rides forth!", "You face Sir Knightly!",
             "A worthy opponent I hope!", "Let the joust begin!", "I am Knightly!",
             "Have you trained well?", "On my honor, I shall fight!", "Swords at the ready!"},
            {"Hmph...", "Strategizing...", "A cunning move...", "I see...", "Interesting...",
             "You have hidden depths...", "A tactical puzzle...", "I must concentrate...",
             "The board reveals its secrets...", "Patience is a virtue...", "Let me analyze...",
             "Every move has meaning...", "I see your pattern..."},
            {"Have at thee!", "Your move.", "A fine move!", "Take this!", "Thus I advance!",
             "Let the battle continue!", "Onward!", "A bold move!", "Well struck!",
             "As fate would have it!", "The die is cast!", "So it begins!", "Take heed!"},
            {"Aha! Captured!", "Mine now!", "Excellent capture!", "A piece falls!",
             "Victory in this skirmish!", "One of yours is taken!", "Ha! A fine gain!",
             "Your ranks grow thinner!", "A satisfying capture!", "Another trophy!",
             "Down goes one of yours!", "The spoils of war!", "A worthy acquisition!"},
            {"Check! Yield!", "You're in check!", "The king is threatened!", "Your king is exposed!",
             "Check! Defend yourself!", "The crown is in danger!", "I have you on the run!",
             "A decisive strike!", "Your king quivers!", "Check! What say you?",
             "The throne is vulnerable!", "King in peril!"},
            {"Checkmate! I triumph!", "Victory is mine!", "A glorious checkmate!", 
             "The kingdom falls!", "Checkmate! I have won!", "A decisive end!",
             "The battle is over!", "I am victorious in combat!", "You fought well but I prevailed!",
             "Checkmate! Lay down your arms!", "What a glorious finish!", "Destiny favors Knightly!"},
            {"I am victorious!", "Another win for Knightly!", "None can defeat me!", 
             "Glory is mine!", "The stronger knight wins!", "I remain undefeated!",
             "Victory tastes sweet!", "A clean win!", "My strategy was flawless!",
             "I have proven my worth!", "The realm celebrates!", "Huzzah! I win!"},
            {"Defeated...", "You fought well.", "I shall return!", "You have bested me...",
             "A noble defeat.", "I accept my loss.", "You were the better strategist.",
             "My sword is lowered...", "Till we meet again!", "I lost but with honor.",
             "You earned this victory.", "I shall train harder!", "Good fight. I salute you."},
            {"You dare take my piece!", "A scratch! Just a scratch!", "Retreat!", 
             "You strike boldly!", "My piece! You villain!", "A temporary setback!",
             "You cut deep!", "I shall remember this!", "That piece will be avenged!",
             "You fight dirty!", "A loss in battle, not the war!", "My forces dwindle..."},
            {"A stalemate...", "Neither wins.", "A draw... acceptable.", "A balanced outcome.",
             "We are evenly matched.", "Honor is satisfied.", "A draw is fair.",
             "Neither victory nor defeat.", "The battle ends in stalemate.",
             "A worthy deadlock!", "We shall meet another day.", "A curious outcome."}
        };
    } else if (botName == "Bishop") {
        botLines = {
            {"Welcome, my child.", "Peace be with you.", "Let us play.", "I am Bishop.",
             "Patience guides us.", "A game of virtue awaits.", "Come, sit with me.",
             "The board is our temple.", "Let us find truth in chess.",
             "I have awaited this match.", "May wisdom guide our hands.",
             "Blessings upon this game.", "Let us begin with grace."},
            {"Let me ponder...", "I see...", "A thoughtful game...", "Wisdom takes time...",
             "The answer will come...", "Quiet contemplation...", "I reflect on the board...",
             "Each move is a prayer...", "The truth reveals itself slowly...",
             "Patience, my child...", "I meditate on the position...", 
             "Let me seek divine inspiration...", "Silence helps me think..."},
            {"A measured move.", "Your turn.", "Patience.", "Thus it is.", "Proceed.",
             "Let the game flow.", "As it should be.", "A deliberate choice.",
             "One step at a time.", "The path unfolds.", "So it is written.",
             "A humble move.", "Your move, my child."},
            {"A capture. Excellent.", "Take only what is needed.", "Ah, a gain.",
             "One piece less in this world.", "Balance shifts.", "A necessary acquisition.",
             "The material world turns.", "A piece departs in peace.",
             "Such is the nature of the game.", "Accept what is taken.",
             "A small victory in a larger war.", "The board breathes."},
            {"Check. Watch yourself.", "The king is exposed!", "A warning.", 
             "Your king is vulnerable.", "I must alert you.", "Check, my child.",
             "The crown is at risk.", "Attend to your king.", "Danger approaches.",
             "A storm gathers.", "Your king needs protection.", "Heed this warning."},
            {"Checkmate. The end.", "The game is concluded.", "A decisive victory.",
             "It is finished.", "The final truth.", "Checkmate. Peace follows.",
             "The king has fallen.", "All games must end.", "A blessed conclusion.",
             "Victory in grace.", "The final judgment.", "Thus the game ends."},
            {"I have prevailed.", "Wisdom wins.", "A peaceful victory.",
             "Righteousness triumphs.", "Grace guided my hand.", "Victory with humility.",
             "The better strategy prevailed.", "A win blessed by patience.",
             "I accept this victory.", "Peace and victory.", "The righteous path led here.",
             "My faith was rewarded."},
            {"You have bested me.", "I accept defeat.", "Your skill shines.",
             "You were the better today.", "I bow to your wisdom.", "Defeat is a teacher.",
             "A humbling experience.", "You played beautifully.", "I learn from this loss.",
             "Grace in defeat as in victory.", "Your light outshone mine.",
             "I accept this outcome with peace."},
            {"A necessary sacrifice.", "You took my piece.", "All part of the plan.",
             "A piece lost is a lesson gained.", "Let it go.", "Material is fleeting.",
             "I release this piece.", "All sacrifices have meaning.",
             "What is lost now is gained later.", "You took what I offered.",
             "A piece gone but not forgotten.", "Sacrifice is the path to wisdom."},
            {"A draw. How fitting.", "Equally matched.", "Peace through stalemate.",
             "Neither wins, neither loses.", "A balanced outcome.", "Harmony in equality.",
             "A fair result for all.", "The scales are balanced.", "Peace is the true victory.",
             "Equilibrium achieved.", "A draw is a blessing.", "Mutual respect on the board."}
        };
    } else if (botName == "Rook") {
        botLines = {
            {"Let's go!", "I'm ready!", "You're going down!", "Rook reporting!", 
             "Time to crush it!", "Get pumped!", "Let's do this!", "I'm feeling strong today!",
             "Ready to rumble!", "No mercy!", "Rook is in the building!",
             "Let's make this quick!", "I'm fired up!"},
            {"Thinking...", "Analyzing...", "Processing...", "Computing...", 
             "Crunching numbers...", "Scanning the board...", "Loading strategy...",
             "System processing...", "Data analysis in progress...", "Running simulations...",
             "Optimizing approach...", "Calculating odds...", "Intel gathering..."},
            {"Move made!", "Your turn now!", "Solid move!", "Boom!", "Executed!",
             "Mission accomplished!", "Deployed!", "Confirmed!", "Next move ready!",
             "Step complete!", "Incoming!", "Take that!", "Rolling out!"},
            {"Captured! Nice!", "Gotcha!", "One down!", "Eliminated!", "Target acquired!",
             "Piece deleted!", "Taken out!", "That's one for Rook!", "Success!",
             "Neutralized!", "Piece eliminated!", "Confirmed kill!", "Done deal!"},
            {"Check! Deal with it!", "King in trouble!", "Check incoming!", 
             "Your king is compromised!", "Check! Panic time!", "King exposed!",
             "Security breach on the king!", "Check! What now?", "Pressure on the crown!",
             "King under fire!", "Checkmate coming soon!", "Your king is mine!"},
            {"Checkmate! Done!", "Game over! I win!", "Rook dominates!", 
             "Checkmate! Total victory!", "Mission complete!", "Rook takes the crown!",
             "Game over! You're done!", "That's a wrap!", "Dominant finish!",
             "Nobody beats Rook!", "Victory achieved!", "System shutdown for you!"},
            {"Victory!", "Rook wins again!", "Too strong!", "I'm unstoppable!",
             "Another win in the books!", "Flawless victory!", "Rook reigns supreme!",
             "Easy win!", "Dominating performance!", "Victory secured!",
             "Rank up!", "Maximum efficiency!"},
            {"You got me...", "Good game!", "Rematch?", "You're actually good!",
             "I'll be back!", "System defeated...", "Unexpected outcome!",
             "I slipped up!", "Error in my calculations!", "Fine, you win this round!",
             "Rebooting for rematch!", "You're pretty strong!"},
            {"Hey! My piece!", "You'll pay for that!", "Revenge will come!", 
             "System breach detected!", "You took one of mine!", "That's a mistake!",
             "Unauthorized removal!", "My piece! That hurts!", "You'll regret that!",
             "Countdown to revenge!", "I owe you one!", "Data loss detected!"},
            {"A tie? Okay.", "Draw is fair.", "We'll meet again!", "System draw.",
             "Acceptable outcome.", "Evenly matched!", "Standoff detected.",
             "Draw protocol initiated.", "Too close to call!", "We go again sometime!",
             "Statistics say it's fair.", "Equal performance."}
        };
    } else if (botName == "Queen") {
        botLines = {
            {"Darling, let's play.", "I do hope you're ready.", "Queen is here.", "Shall we begin?",
             "How delightful.", "I've been expecting you.", "Entertain me, won't you?",
             "Let's see what you've got.", "I do love a challenging game.",
             "Prepare yourself, sweetie.", "The throne awaits.", "Another challenger. Splendid.",
             "I hope you're not boring."},
            {"How intriguing...", "I see your plan...", "Clever...", "Let me consider...",
             "How quaint...", "You have my attention...", "Not bad at all...",
             "This requires thought...", "A curious position...", "Let me grace this with thought...",
             "You're more interesting than I thought...", "I do love a thinker...",
             "Mmm, let me think about this..."},
            {"As expected.", "Your turn, dear.", "Quite simple.", "Naturally.", 
             "There we go.", "Perfect execution.", "Flawless, if I may say so.",
             "One must maintain standards.", "Done and dusted.", "Effortless.",
             "Predictable but effective.", "A queen's work is never done.", "Your turn now."},
            {"Captured, how delightful.", "One less piece.", "A small victory.",
             "How satisfying.", "Piece taken. How lovely.", "You barely needed that piece.",
             "A little something for me.", "Consider it a gift from you to me.",
             "Oh, I needed that.", "That's mine now, sweetie.", "How generous of you.",
             "This will do nicely."},
            {"Check. How predictable.", "Your king is vulnerable.", "Check, darling.",
             "Your king is in quite a spot.", "Oh my, your king is exposed.",
             "Check. Do try to keep up.", "The king is threatened. How unfortunate.",
             "I do love putting kings in danger.", "Check. Now what will you do?",
             "A delicate situation for you.", "Your king looks so distressed."},
            {"Checkmate. Obviously.", "I win. As expected.", "Game over, darling.",
             "Checkmate. Was there any doubt?", "Of course I won. I always do.",
             "That's checkmate. Took me long enough.", "Victory is the only option.",
             "I told you I'd win.", "Checkmate. You did try though.",
             "The game has ended. I won, naturally.", "That was inevitable.",
             "A queen always wins. Remember that."},
            {"Of course I won.", "Victory is natural.", "You tried your best.",
             "Winning is a habit, darling.", "I do love winning.", "Another win for the Queen.",
             "Success is my birthright.", "That went exactly as planned.",
             "Who else could have done it better?", "Perfection, as always.",
             "The queen keeps her crown.", "A predictable outcome."},
            {"You won... Impressive.", "Well played.", "I underestimated you.",
             "I suppose you earned that.", "Not bad at all, darling.", "You actually did it.",
             "I'm rather impressed.", "You'll have to teach me your secrets.",
             "That was quite good, I admit.", "You're stronger than you look.",
             "I don't lose often. Remember this.", "Fine. You win. This time."},
            {"You took my piece? Brave.", "A daring move.", "How unfortunate for me.",
             "You dare? How bold.", "I'll remember that, darling.", "That piece was my favorite.",
             "You'll regret touching my things.", "A grave mistake, sweetie.",
             "How rude of you.", "You've made an enemy today.",
             "That piece will be avenged.", "I don't appreciate that."},
            {"A draw? Acceptable.", "Equally matched.", "We shall meet again.",
             "A tie. How mediocre.", "I suppose a draw is fair.", "Neither of us won. Boring.",
             "We're clearly both exceptional.", "A temporary truce.", "Until next time, darling.",
             "An acceptable outcome.", "We'll have to do this again.", "A draw suits us both."}
        };
    } else {
        botLines = {
            {"Let's play!", "Hi!", "Good luck!", "I'm ready!", "Let's have fun!",
             "Hello there!", "Nice to meet you!", "This will be fun!", "I love chess!",
             "Ready when you are!", "Here we go!"},
            {"Hmm...", "Thinking...", "Let me see...", "Good move...",
             "Interesting...", "What's next...", "I need to focus...",
             "Consider options...", "What's the best move...", "Tricky..."},
            {"Your turn!", "Take that!", "Moving!", "There!", "Done!",
             "How about that?", "Made my move!", "Your move!", "Go ahead!",
             "Piece moved!", "Next!"},
            {"Captured!", "Got one!", "Ha! Captured!", "Mine now!",
             "Nice!", "Took your piece!", "One for me!", "Excellent capture!",
             "That's mine!", "Your piece is gone!"},
            {"Check!", "In check!", "Check on you!", "King is in check!",
             "Your king is exposed!", "Checkmate soon!", "Check incoming!"},
            {"Checkmate!", "I win!", "Game over!", "I got you!",
             "Yes! Checkmate!", "Victory!", "I won this game!"},
            {"I win!", "Victory!", "I'm the best!", "Another win!",
             "I'm too good!", "Winning is fun!", "Champion!"},
            {"Good game!", "You win!", "You got me!", "Well played!",
             "I'll get you next time!", "Great match!", "You're good!"},
            {"My piece!", "Oh no!", "You took my piece!", "Not my piece!",
             "That hurts!", "I needed that!", "How dare you!"},
            {"A draw!", "Tie game!", "We're evenly matched!", "Fair enough!",
             "A draw is fine!", "Good battle!", "We go again!"}
        };
    }
}

void Game::loadBotAvatar() {
    std::string path = "assets/avatars/" + botName + ".png";
    if (!botAvatar.loadFromFile(path)) {
        sf::RenderTexture rt;
        rt.resize({80, 80});
        rt.clear(sf::Color::Transparent);

        static const struct { float r, g, b; } colors[] = {
            {200, 160, 80}, {80, 160, 200}, {180, 100, 200}, {160, 180, 90}, {200, 120, 100}
        };
        unsigned hc = 0;
        for (char c : botName) hc += static_cast<unsigned>(c);
        auto& c = colors[hc % 5];

        sf::CircleShape face(38);
        face.setFillColor(sf::Color(c.r, c.g, c.b));
        face.setOutlineColor(sf::Color(c.r*0.6f, c.g*0.6f, c.b*0.6f));
        face.setOutlineThickness(2);
        face.setPosition({2, 2});
        rt.draw(face);

        sf::CircleShape eye(5);
        eye.setFillColor(sf::Color(40, 35, 40));
        eye.setPosition({20, 28});
        rt.draw(eye);
        eye.setPosition({52, 28});
        rt.draw(eye);

        sf::CircleShape pupil(2);
        pupil.setFillColor(sf::Color::White);
        pupil.setPosition({23, 31});
        rt.draw(pupil);
        pupil.setPosition({55, 31});
        rt.draw(pupil);

        sf::CircleShape mouth(4);
        mouth.setFillColor(sf::Color(40, 35, 40));
        mouth.setPosition({35, 58});
        rt.draw(mouth);

        rt.display();
        botAvatar = rt.getTexture();
    }
}

void Game::init() {
    board.init();
    pieceSelected = false;
    dragging = false;
    currentPlayer = WHITE; // White always moves first in chess
    boardFlipped = (playerColor == BLACK);
    gameOver = false;
    checkmate = false;
    stalemate = false;
    result = RESULT_NONE;
    totalMoves = 0;
    aiThinking = false;
    aiMoveReady = false;
    awaitingPromotion = false;
    hasLastMove = false;
    leftMouseDown = false;
    rightDragging = false;
    showRightDragArrow = false;
    exitConfirmActive = false;
    hintActive = false;
    hintText.clear();
    undoStack.clear();
    undoCooldown.restart();
    particles.clear();
    shakeIntensity = 0;
    positionHistory.clear();
    drawByFiftyMove = false;
    drawByRepetition = false;
    drawByInsufficientMaterial = false;
    moveHistory.clear();
    openingHistory.clear();
    openingBook.clearCache();
    annotations.clear();
    reviewBoardStates.clear();
    reviewMode = false;
    reviewIndex = 0;
    anim.active = false;
    botDialog.active = false;
    exitToMenu = false;
    hoverExitBtn = false;
    hoverPlayAgain = false;
    capturedWhite.clear();
    capturedBlack.clear();
    gameClock.restart();
    capturesByWhite = 0;
    capturesByBlack = 0;

    if (mode == MODE_VS_BOT && !botDialog.active) {
        botDialog.show(pick(botLines.greeting), EXPR_HAPPY, 2.5f);
    }

    if (mode == MODE_NETWORK && netMgr) {
        // Host = White, Client = Black
        playerColor = (netMgr->getRole() == NetRole::HOST) ? WHITE : BLACK;
        boardFlipped = false; // Don't flip for network play
    }
}

void Game::reset() { init(); }

void Game::boardToScreen(int row, int col, int& sx, int& sy) const {
    int r = boardFlipped ? (7 - row) : row;
    int c = boardFlipped ? (7 - col) : col;
    sx = boardX + c * squareSize;
    sy = boardY + r * squareSize;
}

bool Game::screenToBoard(int sx, int sy, int& row, int& col) const {
    col = (sx - boardX) / squareSize;
    row = (sy - boardY) / squareSize;
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return false;
    if (boardFlipped) { row = 7 - row; col = 7 - col; }
    return true;
}

void Game::trackCapturedPiece(const Move& move) {
    PieceType captured = board.getPiece(move.toRow, move.toCol);
    if (captured != NONE) {
        PieceColor capturer = board.getColor(move.fromRow, move.fromCol);
        if (capturer == WHITE) {
            capturedWhite.push_back({captured, BLACK});
            capturesByWhite++;
        } else {
            capturedBlack.push_back({captured, WHITE});
            capturesByBlack++;
        }
    }
}

std::string Game::pickPhrase(const std::vector<std::string>& phrases) const {
    return pick(phrases);
}

void Game::completeMove(const Move& move) {
    PieceType movingType = board.getPiece(move.fromRow, move.fromCol);
    PieceColor movingColor = board.getColor(move.fromRow, move.fromCol);

    bool wasCapture = board.getPiece(move.toRow, move.toCol) != NONE;

    // Save state for undo + generate algebraic notation before makeMove
    undoStack.push_back({move, board.getState()});
    if (maxUndo > 0 && (int)undoStack.size() > maxUndo)
        undoStack.erase(undoStack.begin());
    std::string alg = moveToAlgebraic(move);

    trackCapturedPiece(move);
    hasLastMove = true;
    lastFromRow = move.fromRow;
    lastFromCol = move.fromCol;
    lastToRow = move.toRow;
    lastToCol = move.toCol;

    totalMoves++;
    board.makeMove(move);
    moveHistory.push_back(alg);
    openingHistory.push_back(move);
    currentPlayer = board.getCurrentPlayer();

    // Send move over network in network mode
    if (mode == MODE_NETWORK && netMgr)
        netMgr->sendMove(move);
    std::string posKey = board.getPositionKey();
    positionHistory[posKey]++;
    pieceSelected = false;

    anim.type = movingType;
    anim.color = movingColor;
    anim.fromRow = move.fromRow;
    anim.fromCol = move.fromCol;
    anim.toRow = move.toRow;
    anim.toCol = move.toCol;
    anim.clock.restart();
    anim.active = true;

    // Particles + shake
    emitMoveParticles(move.fromRow, move.fromCol, move.toRow, move.toCol);
    if (wasCapture) {
        emitCaptureParticles(move.toRow, move.toCol);
        triggerShake(4.0f);
    }

    if (audio) {
        if (wasCapture) audio->playCapture();
        else audio->playMove();
    }

    bool gaveCheck = board.isInCheck(currentPlayer);
    if (gaveCheck) {
        triggerShake(3.0f);
        if (audio) audio->playCheck();
    }

    // Draw by rule
    if (!gameOver) {
        if (board.isFiftyMoveDraw()) {
            drawByFiftyMove = true;
            gameOver = true;
            result = RESULT_DRAW;
        } else if (positionHistory[posKey] >= 3) {
            drawByRepetition = true;
            gameOver = true;
            result = RESULT_DRAW;
        } else if (board.isInsufficientMaterial()) {
            drawByInsufficientMaterial = true;
            gameOver = true;
            result = RESULT_DRAW;
        }
    }

    if (mode == MODE_VS_BOT && movingColor != playerColor && !gameOver) {
        bool gaveCheck = board.isInCheck(playerColor);
        if (gameOver && (drawByFiftyMove || drawByRepetition || drawByInsufficientMaterial)) {
            std::string msg;
            if (drawByFiftyMove) msg = pickPhrase(botLines.draw) + " (50-move rule)";
            else if (drawByRepetition) msg = pickPhrase(botLines.draw) + " (3-fold repetition)";
            else msg = pickPhrase(botLines.draw) + " (insufficient material)";
            botDialog.show(msg, EXPR_NEUTRAL, 4.0f);
        } else if (board.isCheckmate()) {
            botDialog.show(pickPhrase(botLines.checkmate), EXPR_HAPPY, 4.0f);
        } else if (board.isStalemate()) {
            botDialog.show(pickPhrase(botLines.draw), EXPR_NEUTRAL, 4.0f);
        } else if (gaveCheck) {
            if (audio) audio->playCheck();
            std::vector<std::string> combined = botLines.check;
            if (wasCapture) {
                combined.insert(combined.end(), botLines.capture.begin(), botLines.capture.end());
            }
            botDialog.show(pick(combined), EXPR_HAPPY);
        } else if (wasCapture) {
            botDialog.show(pickPhrase(botLines.capture), EXPR_HAPPY);
        } else {
            botDialog.show(pickPhrase(botLines.move), EXPR_NEUTRAL);
        }
    }

    if (board.isCheckmate()) {
        checkmate = true;
        gameOver = true;
        result = (currentPlayer == WHITE) ? RESULT_BLACK_WINS : RESULT_WHITE_WINS;
        triggerShake(8.0f);
        if (audio) audio->playCheckmate();
        if (mode == MODE_VS_BOT) {
            bool playerWins = (result == RESULT_WHITE_WINS && playerColor == WHITE) ||
                              (result == RESULT_BLACK_WINS && playerColor == BLACK);
            if (playerWins)
                botDialog.show(pickPhrase(botLines.lose), EXPR_SAD, 5.0f);
            else
                botDialog.show(pickPhrase(botLines.win), EXPR_HAPPY, 5.0f);
        }
    } else if (board.isStalemate() || gameOver) {
        if (board.isStalemate()) {
            stalemate = true;
            if (audio) audio->playStalemate();
        }
        if (mode == MODE_VS_BOT) {
            std::string msg;
            if (drawByFiftyMove) msg = pickPhrase(botLines.draw) + " (50-move rule)";
            else if (drawByRepetition) msg = pickPhrase(botLines.draw) + " (3-fold repetition)";
            else if (drawByInsufficientMaterial) msg = pickPhrase(botLines.draw) + " (insufficient material)";
            else msg = pickPhrase(botLines.draw);
            botDialog.show(msg, EXPR_NEUTRAL, 4.0f);
        }
    }

    // Analyze move for annotation (only in bot and local modes)
    if (!gameOver && mode != MODE_NETWORK) {
        analyzeLastMove();
    }

    // Auto-save game when it ends
    if (gameOver) {
        saveGame();
    }
}

void Game::analyzeLastMove() {
    if (moveHistory.empty() || !ai) return;
    if (mode == MODE_NETWORK) return;

    // Save current board state for review
    reviewBoardStates.push_back(board.getState());

    Move lastMove = openingHistory.back();

    // Use clean AI (no blunder noise) for accurate analysis
    ChessAI analysisAI(BOT_KNIGHT);
    analysisAI.setDynamicParams(3, 0, 50, 50);

    // Unmake the move to analyze from the previous position
    BoardState prevState = undoStack.back().second;
    Move moveBefore = undoStack.back().first;
    board.unmakeMove(moveBefore, prevState);

    PieceColor sideToMove = board.getCurrentPlayer();
    Move bestMove = analysisAI.getBestMoveClean(board, 3);

    BoardState bestState = board.makeMove(bestMove);
    int bestEval = analysisAI.evaluateClean(board);
    board.unmakeMove(bestMove, bestState);

    // Remake the actual move
    board.makeMove(moveBefore);

    int playedEval = analysisAI.evaluateClean(board);

    int evalDiff;
    if (sideToMove == WHITE) {
        evalDiff = bestEval - playedEval;
    } else {
        evalDiff = playedEval - bestEval;
    }

    MoveAnnotation anno = ANNO_GOOD;
    bool isBestMove = (lastMove == bestMove);

    std::optional<Move> bookMove = openingBook.getBookMove(openingHistory);
    if (bookMove.has_value() && lastMove == bookMove.value()) {
        anno = ANNO_BOOK;
    } else if (isBestMove) {
        PieceType movingPiece = board.getPiece(lastMove.fromRow, lastMove.fromCol);
        PieceType capturedPiece = board.getPiece(lastMove.toRow, lastMove.toCol);
        int movingValue = 0, capturedValue = 0;
        switch (movingPiece) {
            case PAWN: movingValue = 100; break;
            case KNIGHT: movingValue = 320; break;
            case BISHOP: movingValue = 330; break;
            case ROOK: movingValue = 500; break;
            case QUEEN: movingValue = 900; break;
            default: break;
        }
        switch (capturedPiece) {
            case PAWN: capturedValue = 100; break;
            case KNIGHT: capturedValue = 320; break;
            case BISHOP: capturedValue = 330; break;
            case ROOK: capturedValue = 500; break;
            case QUEEN: capturedValue = 900; break;
            default: break;
        }
        if (movingValue > capturedValue + 100) {
            anno = ANNO_BRILLIANT;
        } else {
            anno = ANNO_GREAT;
        }
    } else if (evalDiff <= 10) {
        anno = ANNO_EXCELLENT;
    } else if (evalDiff <= 30) {
        anno = ANNO_GOOD;
    } else if (evalDiff <= 100) {
        anno = ANNO_INACCURACY;
    } else if (evalDiff <= 250) {
        anno = ANNO_MISTAKE;
    } else {
        anno = ANNO_BLUNDER;
    }

    annotations.push_back(anno);
}

std::string Game::annotationToString(MoveAnnotation a) const {
    if (a >= 0 && a <= ANNO_BLUNDER) return ANNOTATION_NAMES[a];
    return "";
}

std::string Game::gamesDir() {
    const char* home = std::getenv("HOME");
    std::string dir = home ? std::string(home) + "/Library/Application Support/Chess Kumar/games"
                            : std::string("./games");
    mkdir(dir.c_str(), 0755);
    return dir;
}

void Game::saveGame() const {
    std::string dir = gamesDir();
    int fileNum = 1;
    for (int i = 1; i < 1000; ++i) {
        std::string path = dir + "/game_" + std::to_string(i) + ".txt";
        std::ifstream test(path);
        if (!test.good()) { fileNum = i; break; }
        if (i == 999) fileNum = i;
    }

    std::string path = dir + "/game_" + std::to_string(fileNum) + ".txt";
    std::ofstream f(path, std::ios::trunc);
    if (!f) return;

    auto now = std::time(nullptr);
    char timeBuf[64];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

    std::string resultStr;
    if (result == RESULT_WHITE_WINS) resultStr = "1-0";
    else if (result == RESULT_BLACK_WINS) resultStr = "0-1";
    else resultStr = "1/2-1/2";

    std::string modeStr;
    if (mode == MODE_VS_BOT) modeStr = "vs_bot:" + botName;
    else if (mode == MODE_LOCAL_MULTIPLAYER) modeStr = "local";
    else modeStr = "network";

    std::string colorStr = (playerColor == WHITE) ? "white" : "black";
    int elapsedSec = static_cast<int>(gameClock.getElapsedTime().asSeconds());

    f << "date=" << timeBuf << "\n";
    f << "mode=" << modeStr << "\n";
    f << "playerColor=" << colorStr << "\n";
    f << "result=" << resultStr << "\n";
    f << "totalMoves=" << totalMoves << "\n";
    f << "time=" << elapsedSec << "\n";
    f << "checkmate=" << (checkmate ? 1 : 0) << "\n";
    f << "stalemate=" << (stalemate ? 1 : 0) << "\n";
    f << "drawByFiftyMove=" << (drawByFiftyMove ? 1 : 0) << "\n";
    f << "drawByRepetition=" << (drawByRepetition ? 1 : 0) << "\n";
    f << "drawByInsufficientMaterial=" << (drawByInsufficientMaterial ? 1 : 0) << "\n";
    f << "capturesWhite=" << capturesByWhite << "\n";
    f << "capturesBlack=" << capturesByBlack << "\n";

    f << "moves=" << moveHistory.size() << "\n";
    for (size_t i = 0; i < moveHistory.size(); ++i) {
        std::string annoStr = (i < annotations.size()) ? annotationToString(annotations[i]) : "";
        f << "move_" << i << "=" << moveHistory[i];
        if (!annoStr.empty()) f << " {" << annoStr << "}";
        f << "\n";
    }
    f.close();
}

void Game::enterReviewMode() {
    if (moveHistory.empty()) return;
    reviewMode = true;
    reviewIndex = 0;

    reviewBoardStates.clear();
    ChessBoard tempBoard;
    tempBoard.init();
    reviewBoardStates.push_back(tempBoard.getState());

    for (size_t i = 0; i < openingHistory.size(); ++i) {
        BoardState st = tempBoard.makeMove(openingHistory[i]);
        reviewBoardStates.push_back(st);
    }
}

void Game::exitReviewMode() {
    reviewMode = false;
    reviewIndex = 0;
}

void Game::reviewForward() {
    if (!reviewMode) return;
    if (reviewIndex < (int)moveHistory.size()) reviewIndex++;
}

void Game::reviewBackward() {
    if (!reviewMode) return;
    if (reviewIndex > 0) reviewIndex--;
}

void Game::drawReviewOverlay(sf::RenderWindow& window) {
    if (!reviewMode || !assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    bool dm = !settings || settings->darkMode;

    int cx = boardX + boardSize / 2;

    sf::Text header(*font, "Game Review", 20);
    header.setFillColor(sf::Color(212, 175, 55, 255));
    sf::FloatRect htb = header.getLocalBounds();
    header.setOrigin(sf::Vector2f(htb.size.x / 2, htb.size.y / 2));
    header.setPosition(sf::Vector2f(cx, boardY - 25));
    window.draw(header);

    int px = boardX + boardSize + 18;
    int py = boardY;
    int pw = panelWidth;

    sf::RectangleShape panel(sf::Vector2f((float)pw, (float)boardSize));
    panel.setFillColor(dm ? sf::Color(30, 25, 22, 220) : sf::Color(255, 252, 248, 200));
    panel.setPosition(sf::Vector2f((float)(px - 8), (float)py));
    window.draw(panel);

    sf::Text title(*font, "Move List", 14);
    title.setFillColor(dm ? sf::Color(220, 200, 170) : sf::Color(50, 45, 40));
    title.setPosition(sf::Vector2f((float)(px + 8), (float)(py + 8)));
    window.draw(title);

    float lineH = 14.0f;
    int maxLines = (int)((boardSize - 80) / lineH);
    int startIdx = std::max(0, reviewIndex - maxLines + 1);

    for (int i = startIdx; i < (int)moveHistory.size() && i - startIdx < maxLines; ++i) {
        float y = py + 30 + (i - startIdx) * lineH;
        bool isCurrent = (i == reviewIndex);

        std::string moveNum = std::to_string(i / 2 + 1) + (i % 2 == 0 ? "." : "...");
        std::string moveStr = moveHistory[i];
        std::string annoStr = (i < (int)annotations.size()) ? annotationToString(annotations[i]) : "";

        sf::Color moveColor;
        if (isCurrent) moveColor = sf::Color(212, 175, 55, 255);
        else moveColor = dm ? sf::Color(180, 160, 130, 200) : sf::Color(80, 70, 60, 200);

        if (isCurrent) {
            sf::RectangleShape highlight(sf::Vector2f((float)(pw - 20), lineH - 2));
            highlight.setFillColor(sf::Color(212, 175, 55, 30));
            highlight.setPosition(sf::Vector2f((float)(px + 6), y - 1));
            window.draw(highlight);
        }

        sf::Text numT(*font, moveNum, 10);
        numT.setFillColor(dm ? sf::Color(140, 120, 90) : sf::Color(120, 110, 100));
        numT.setPosition(sf::Vector2f((float)(px + 10), y));
        window.draw(numT);

        sf::Text moveT(*font, moveStr, 10);
        moveT.setFillColor(moveColor);
        sf::FloatRect numBounds = numT.getLocalBounds();
        moveT.setPosition(sf::Vector2f((float)(px + 10 + numBounds.size.x + 8), y));
        window.draw(moveT);

        if (!annoStr.empty()) {
            sf::Color annoColor;
            switch (annotations[i]) {
                case ANNO_BRILLIANT: annoColor = sf::Color(0, 200, 255); break;
                case ANNO_GREAT: annoColor = sf::Color(80, 200, 80); break;
                case ANNO_EXCELLENT: annoColor = sf::Color(120, 220, 120); break;
                case ANNO_GOOD: annoColor = sf::Color(180, 200, 180); break;
                case ANNO_INACCURACY: annoColor = sf::Color(255, 200, 50); break;
                case ANNO_MISTAKE: annoColor = sf::Color(255, 140, 50); break;
                case ANNO_BLUNDER: annoColor = sf::Color(255, 60, 60); break;
                case ANNO_BOOK: annoColor = sf::Color(180, 160, 220); break;
                default: annoColor = sf::Color(180, 160, 130); break;
            }
            sf::Text annoT(*font, annoStr, 9);
            annoT.setFillColor(annoColor);
            sf::FloatRect moveBounds = moveT.getLocalBounds();
            annoT.setPosition(sf::Vector2f((float)(px + 10 + numBounds.size.x + 8 + moveBounds.size.x + 8), y));
            window.draw(annoT);
        }
    }

    if (reviewIndex < (int)moveHistory.size()) {
        sf::Text currentInfo(*font, "Move " + std::to_string(reviewIndex + 1) + " / " + std::to_string(moveHistory.size()), 11);
        currentInfo.setFillColor(dm ? sf::Color(160, 140, 120) : sf::Color(100, 90, 80));
        currentInfo.setPosition(sf::Vector2f((float)(px + 8), (float)(py + boardSize - 50)));
        window.draw(currentInfo);
    }

    float btnY = py + boardSize - 30;
    float btnW = 70.0f;
    float btnH = 24.0f;

    reviewPrevBounds = sf::FloatRect({(float)(px + 8), btnY}, {btnW, btnH});
    bool prevHover = reviewPrevBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
    bool canPrev = reviewIndex > 0;
    drawFilledRoundRect(window, reviewPrevBounds.position.x, reviewPrevBounds.position.y,
                        btnW, btnH, 6,
                        canPrev ? (prevHover ? sf::Color(70, 60, 50, 230) : sf::Color(55, 45, 35, 200))
                                : sf::Color(35, 30, 25, 160));
    sf::Text prevT(*font, "< Prev", 10);
    prevT.setFillColor(canPrev ? (prevHover ? sf::Color(220, 200, 170) : sf::Color(180, 160, 130))
                               : sf::Color(100, 90, 80));
    sf::FloatRect ptb = prevT.getLocalBounds();
    prevT.setPosition(sf::Vector2f(reviewPrevBounds.position.x + btnW/2 - ptb.size.x/2,
                                    reviewPrevBounds.position.y + btnH/2 - ptb.size.y/2 - 1));
    window.draw(prevT);

    reviewNextBounds = sf::FloatRect({(float)(px + 85), btnY}, {btnW, btnH});
    bool nextHover = reviewNextBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
    bool canNext = reviewIndex < (int)moveHistory.size() - 1;
    drawFilledRoundRect(window, reviewNextBounds.position.x, reviewNextBounds.position.y,
                        btnW, btnH, 6,
                        canNext ? (nextHover ? sf::Color(70, 60, 50, 230) : sf::Color(55, 45, 35, 200))
                                : sf::Color(35, 30, 25, 160));
    sf::Text nextT(*font, "Next >", 10);
    nextT.setFillColor(canNext ? (nextHover ? sf::Color(220, 200, 170) : sf::Color(180, 160, 130))
                               : sf::Color(100, 90, 80));
    sf::FloatRect ntb = nextT.getLocalBounds();
    nextT.setPosition(sf::Vector2f(reviewNextBounds.position.x + btnW/2 - ntb.size.x/2,
                                    reviewNextBounds.position.y + btnH/2 - ntb.size.y/2 - 1));
    window.draw(nextT);

    reviewExitBounds = sf::FloatRect({(float)(px + 162), btnY}, {68.0f, btnH});
    bool exitHover = reviewExitBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
    drawFilledRoundRect(window, reviewExitBounds.position.x, reviewExitBounds.position.y,
                        68.0f, btnH, 6,
                        exitHover ? sf::Color(180, 70, 70, 200) : sf::Color(140, 50, 50, 160));
    sf::Text exitT(*font, "Exit", 10);
    exitT.setFillColor(sf::Color(240, 220, 200));
    sf::FloatRect etb = exitT.getLocalBounds();
    exitT.setPosition(sf::Vector2f(reviewExitBounds.position.x + 34 - etb.size.x/2,
                                    reviewExitBounds.position.y + btnH/2 - etb.size.y/2 - 1));
    window.draw(exitT);
}

void Game::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    int mx, my;

    // ── Mouse move (for drag & right-drag arrow) ──
    if (auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        mx = mm->position.x;
        my = mm->position.y;
        if (dragging) dragMousePos = {mx, my};
        if (rightDragging) {
            int row, col;
            if (screenToBoard(mx, my, row, col)) {
                rightDragToRow = row;
                rightDragToCol = col;
            }
        }
        return;
    }

    // ── Mouse button release (drop / right-drag arrow) ──
    if (auto* rel = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (rel->button == sf::Mouse::Button::Left && dragging)
            handleDrop(rel->position.x, rel->position.y, window);
        else if (rel->button == sf::Mouse::Button::Right && rightDragging) {
            rightDragging = false;
            showRightDragArrow = true;
            rightDragClock.restart();
        }
        return;
    }

    // ── Mouse button press ──
    auto* click = event.getIf<sf::Event::MouseButtonPressed>();
    if (!click) return;

    // Right-click → start arrow trail
    if (click->button == sf::Mouse::Button::Right) {
        showRightDragArrow = false;
        int row, col;
        if (screenToBoard(click->position.x, click->position.y, row, col)) {
            rightDragging = true;
            rightDragFromRow = row;
            rightDragFromCol = col;
            rightDragToRow = row;
            rightDragToCol = col;
        }
        return;
    }

    if (click->button != sf::Mouse::Button::Left) return;

    mx = click->position.x;
    my = click->position.y;

    // ── Exit confirmation dialog (if active, only Yes/No clicks matter) ──
    if (exitConfirmActive) {
        if (exitConfirmYesBounds.contains({(float)mx, (float)my})) {
            exitConfirmActive = false;
            exitToMenu = true;
        } else if (exitConfirmNoBounds.contains({(float)mx, (float)my})) {
            exitConfirmActive = false;
        }
        return;
    }

    if (anim.active && !gameOver && !reviewMode) return;

    // ── Review mode navigation ──
    if (reviewMode) {
        if (reviewPrevBounds.contains({(float)mx, (float)my})) {
            reviewBackward();
            return;
        }
        if (reviewNextBounds.contains({(float)mx, (float)my})) {
            reviewForward();
            return;
        }
        if (reviewExitBounds.contains({(float)mx, (float)my})) {
            exitReviewMode();
            return;
        }
        // Keyboard shortcuts also handled in main, but allow click anywhere else to do nothing
        return;
    }

    // ── Non-board buttons ──

    // Game over buttons
    if (gameOver) {
        if (playAgainBtnBounds.contains({(float)mx, (float)my})) {
            init();
            return;
        }
        if (reviewBtnBounds.contains({(float)mx, (float)my})) {
            enterReviewMode();
            return;
        }
        if (exitBtnBounds.contains({(float)mx, (float)my}) ||
            (mx >= exitBtnBounds.position.x && mx <= exitBtnBounds.position.x + exitBtnBounds.size.x &&
             my >= exitBtnBounds.position.y && my <= exitBtnBounds.position.y + exitBtnBounds.size.y)) {
            showExitConfirm();
            return;
        }
        return;
    }

    // Action buttons
    if (undoBtnBounds.contains({(float)mx, (float)my})) {
        undoLastMove();
        if (mode == MODE_VS_BOT) {
            aiMoveReady = false;
            aiThinking = false;
            undoCooldown.restart();
        }
        return;
    }
    if (hintBtnBounds.contains({(float)mx, (float)my})) {
        if (hintActive) clearHint(); else showHint();
        return;
    }
    if (flipBtnBounds.contains({(float)mx, (float)my})) {
        boardFlipped = !boardFlipped;
        return;
    }
    if (coordBtnBounds.contains({(float)mx, (float)my})) {
        showCoordinates = !showCoordinates;
        return;
    }

    // Exit button in side panel
    if (exitBtnBounds.contains({(float)mx, (float)my}) ||
        (mx >= exitBtnBounds.position.x && mx <= exitBtnBounds.position.x + exitBtnBounds.size.x &&
         my >= exitBtnBounds.position.y && my <= exitBtnBounds.position.y + exitBtnBounds.size.y)) {
        showExitConfirm();
        return;
    }

    if (mode == MODE_VS_BOT && currentPlayer != playerColor) return;

    if (awaitingPromotion) {
        handlePromotionClick(mx, my);
        return;
    }

    // ── Board interaction (both click-to-move and drag-and-drop) ──
    int row, col;
    if (!screenToBoard(mx, my, row, col)) return;

    PieceType pt = board.getPiece(row, col);
    PieceColor pc = board.getColor(row, col);

    // If already dragging (second click on target → click-to-move)
    if (dragging) {
        // Clicking another own piece → reselect
        if (pt != NONE && pc == currentPlayer) {
            pieceSelected = true;
            selectedRow = row;
            selectedCol = col;
            dragRow = row;
            dragCol = col;
            dragMousePos = {mx, my};
            if (audio) audio->playPieceSelect();
            return;
        }
        // Click on empty/enemy → try move
        handleDrop(mx, my, window);
        return;
    }

    // Click on own piece → select AND start drag
    if (pt != NONE && pc == currentPlayer) {
        pieceSelected = true;
        selectedRow = row;
        selectedCol = col;
        dragRow = row;
        dragCol = col;
        dragging = true;
        dragMousePos = {mx, my};
        if (audio) audio->playPieceSelect();
        return;
    }

    // Click on empty/enemy with previous selection → try move
    if (pieceSelected) {
        handleDrop(mx, my, window);
    }
}

void Game::handleDrop(int mx, int my, sf::RenderWindow& window) {
    // Works for both drag-to-move (dragging set) and click-to-move (pieceSelected set)
    int fromRow, fromCol;
    if (dragging) {
        fromRow = dragRow;
        fromCol = dragCol;
    } else if (pieceSelected) {
        fromRow = selectedRow;
        fromCol = selectedCol;
    } else {
        return;
    }

    dragging = false;
    pieceSelected = false;

    if (anim.active || gameOver) return;

    int toRow, toCol;
    if (!screenToBoard(mx, my, toRow, toCol)) return;

    // Same square = click without drag → keep selection for click-to-move
    if (toRow == fromRow && toCol == fromCol) {
        pieceSelected = true;
        return;
    }

    // Promotion check
    PieceType moving = board.getPiece(fromRow, fromCol);
    if (moving == PAWN) {
        int promoRank = (currentPlayer == WHITE) ? 0 : 7;
        if (toRow == promoRank) {
            promoFromRow = fromRow;
            promoFromCol = fromCol;
            promoToRow = toRow;
            promoToCol = toCol;
            awaitingPromotion = true;
            return;
        }
    }

    Move move(fromRow, fromCol, toRow, toCol);
    if (board.isLegalMove(move)) {
        if (mode == MODE_VS_BOT && board.getPiece(move.toRow, move.toCol) != NONE)
            botDialog.show(pickPhrase(botLines.pieceLost), EXPR_SAD, 2.0f);
        completeMove(move);
    }
}

void Game::handlePromotionClick(int x, int y) {
    int centerX = boardX + boardSize / 2;
    int centerY = boardY + boardSize / 2;
    int dialogX = centerX - 140;
    int dialogY = centerY + 5;

    int idx = (x - dialogX) / 75;
    if (idx >= 0 && idx < 4) {
        PieceType types[] = {QUEEN, ROOK, BISHOP, KNIGHT};
        completeMove(Move(promoFromRow, promoFromCol, promoToRow, promoToCol, types[idx]));
        awaitingPromotion = false;
    }
}

void Game::doAIMove() {
    if (!ai) return;

    // Check opening book first
    if (mode == MODE_VS_BOT) {
        std::optional<Move> bookMove = openingBook.getBookMove(openingHistory);
        if (bookMove.has_value()) {
            pendingAiMove = bookMove.value();
            aiMoveReady = true;
            aiDelayClock.restart();
            return;
        }
    }

    botDialog.show(pickPhrase(botLines.thinking), EXPR_THINKING, 1.0f);

    // Run AI in background thread to avoid blocking the render loop
    ChessBoard boardCopy = board;
    ChessAI* aiPtr = ai;
    aiFuture = std::async(std::launch::async, [aiPtr, boardCopy]() mutable {
        return aiPtr->getBestMove(boardCopy);
    });
}

// ── Particles ──

void Game::emitParticles(float x, float y, sf::Color color, int count) {
    for (int i = 0; i < count; ++i) {
        float angle = (std::rand() % 360) * 3.14159f / 180.0f;
        float speed = 0.5f + (std::rand() % 80) / 100.0f;
        Particle p;
        p.x = x + (std::rand() % 10 - 5);
        p.y = y + (std::rand() % 10 - 5);
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.life = 0.4f + (std::rand() % 30) / 100.0f;
        p.maxLife = p.life;
        p.size = 3.0f + (std::rand() % 4);
        p.color = color;
        particles.push_back(p);
    }
}

void Game::emitCaptureParticles(int row, int col) {
    int dr = boardFlipped ? (7 - row) : row;
    int dc = boardFlipped ? (7 - col) : col;
    int sx = boardX + dc * squareSize + squareSize / 2;
    int sy = boardY + dr * squareSize + squareSize / 2;
    sf::Color c1(255, 220, 80, 220);
    sf::Color c2(255, 160, 40, 200);
    sf::Color c3(200, 200, 200, 180);
    emitParticles(sx, sy, c1, 4);
    emitParticles(sx, sy, c2, 3);
    emitParticles(sx, sy, c3, 2);
}

void Game::emitMoveParticles(int fromRow, int fromCol, int toRow, int toCol) {
    int fdr = boardFlipped ? (7 - fromRow) : fromRow;
    int fdc = boardFlipped ? (7 - fromCol) : fromCol;
    int tdr = boardFlipped ? (7 - toRow) : toRow;
    int tdc = boardFlipped ? (7 - toCol) : toCol;
    int sx = boardX + fdc * squareSize + squareSize / 2;
    int sy = boardY + fdr * squareSize + squareSize / 2;
    int ex = boardX + tdc * squareSize + squareSize / 2;
    int ey = boardY + tdr * squareSize + squareSize / 2;
    sf::Color c(180, 220, 255, 150);
    emitParticles(ex, ey, c, 5);
}

void Game::updateParticles() {
    float dt = 0.016f;
    for (auto& p : particles) {
        p.x += p.vx * dt * 60.0f;
        p.y += p.vy * dt * 60.0f;
        p.vx *= 0.96f;
        p.vy *= 0.96f;
        p.vy += 0.08f;
        p.life -= dt;
    }
    auto it = std::remove_if(particles.begin(), particles.end(),
                              [](const Particle& p) { return p.life <= 0; });
    particles.erase(it, particles.end());
}

void Game::drawParticles(sf::RenderWindow& window) {
    for (auto& p : particles) {
        float t = p.life / p.maxLife;
        sf::CircleShape circle(p.size * (0.3f + 0.7f * t));
        sf::Color c = p.color;
        c.a = static_cast<std::uint8_t>(p.color.a * t);
        circle.setFillColor(c);
        circle.setOrigin(sf::Vector2f(circle.getRadius(), circle.getRadius()));
        circle.setPosition(sf::Vector2f(p.x, p.y));
        window.draw(circle);
    }
}

// ── Camera Shake ──

void Game::triggerShake(float intensity) {
    shakeMaxIntensity = intensity;
    shakeIntensity = intensity;
    shakeClock.restart();
}

sf::Vector2f Game::getShakeOffset() {
    float elapsed = shakeClock.getElapsedTime().asSeconds();
    if (elapsed > 0.45f) return {0, 0};
    float decay = 1.0f - elapsed * 2.2f;
    float intensity = shakeMaxIntensity * std::max(0.0f, decay);
    float angle = (std::rand() % 360) * 3.14159f / 180.0f;
    float mag = intensity * (0.6f + (std::rand() % 40) / 100.0f);
    return {std::cos(angle) * mag, std::sin(angle) * mag};
}

void Game::update(sf::RenderWindow& window) {
    botDialog.update();
    updateParticles();

    if (gameOver) return;

    // Network mode: check for incoming moves
    if (mode == MODE_NETWORK && netMgr && currentPlayer != playerColor) {
        if (netMgr->hasPendingMove()) {
            Move netMove = netMgr->consumePendingMove();
            // Check for special moves (resign, draw)
            if (netMove.fromRow == -2) {
                // Opponent resigned — we win
                gameOver = true;
                result = (playerColor == WHITE) ? RESULT_WHITE_WINS : RESULT_BLACK_WINS;
                return;
            }
            if (netMove.fromRow == -3) {
                // Draw accepted
                gameOver = true;
                result = RESULT_DRAW;
                return;
            }
            if (board.isLegalMove(netMove)) {
                completeMove(netMove);
            }
        }
        return;
    }

    if (mode == MODE_VS_BOT && currentPlayer != playerColor && !aiThinking && !aiMoveReady) {
        if (undoCooldown.getElapsedTime().asMilliseconds() < 1500) return;
        aiThinking = true;
        doAIMove();
        return;
    }

    // Poll the async AI result
    if (aiThinking && !aiMoveReady && aiFuture.valid()) {
        auto status = aiFuture.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            pendingAiMove = aiFuture.get();
            aiMoveReady = true;
            aiDelayClock.restart();
        }
    }

    if (aiMoveReady) {
        int delayMs = 300;
        switch (difficulty) {
            case EASY:   delayMs = 600; break;
            case MEDIUM: delayMs = 500; break;
            case HARD:   delayMs = 400; break;
            case ULTRA_PRO: delayMs = 300; break;
            default: delayMs = 200; break;
        }
        if (aiDelayClock.getElapsedTime().asMilliseconds() >= delayMs) {
            completeMove(pendingAiMove);
            aiMoveReady = false;
            aiThinking = false;
        }
    }
}

// ── Draw functions ──

void Game::draw(sf::RenderWindow& window) {
    sf::Vector2f shakeOff = getShakeOffset();
    sf::View shakeView = window.getDefaultView();
    shakeView.move(shakeOff);
    window.setView(shakeView);

    bool dm = !settings || settings->darkMode;
    sf::RectangleShape bgRect(sf::Vector2f((float)winW, (float)winH));
    bgRect.setFillColor(dm ? darkBg() : lightBg());
    window.draw(bgRect);

    if (reviewMode) {
        // In review mode: draw board with pieces from the review position
        drawBoard(window);
        drawCoordinates(window);

        // Draw last move highlight for the current review move
        if (reviewIndex >= 0 && reviewIndex < (int)openingHistory.size()) {
            Move revMove = openingHistory[reviewIndex];
            const auto& th = settings ? settings->getTheme() : BOARD_THEMES[0];
            sf::Color highlight(th.accentR, th.accentG, th.accentB, 70);
            for (int r : {revMove.fromRow, revMove.toRow}) {
                int c = (r == revMove.fromRow) ? revMove.fromCol : revMove.toCol;
                int dr = boardFlipped ? (7 - r) : r;
                int dc = boardFlipped ? (7 - c) : c;
                sf::RectangleShape sq(sf::Vector2f(squareSize, squareSize));
                sq.setFillColor(highlight);
                sq.setPosition(sf::Vector2f(boardX + dc * squareSize, boardY + dr * squareSize));
                window.draw(sq);
            }
        }

        // Draw pieces at review position (reviewIndex+1 because index 0 = starting pos)
        int stateIdx = reviewIndex + 1;
        if (!reviewBoardStates.empty() && stateIdx < (int)reviewBoardStates.size()) {
            const BoardState& st = reviewBoardStates[stateIdx];
            for (int row = 0; row < 8; ++row) {
                for (int col = 0; col < 8; ++col) {
                    PieceType pt = st.board[row][col];
                    PieceColor pc = st.colors[row][col];
                    if (pt == NONE) continue;
                    const sf::Texture* tex = assetManager ? assetManager->getPieceTexture(pt, pc) : nullptr;
                    if (!tex) continue;
                    float s = (squareSize - 8.0f) / tex->getSize().x;
                    int r2 = boardFlipped ? (7 - row) : row;
                    int c2 = boardFlipped ? (7 - col) : col;
                    float px = boardX + c2 * squareSize + 4;
                    float py = boardY + r2 * squareSize + 4;
                    sf::Sprite shadow(*tex);
                    shadow.setScale(sf::Vector2f(s, s));
                    shadow.setPosition(sf::Vector2f(px + 2, py + 3));
                    shadow.setColor(sf::Color(0, 0, 0, 60));
                    window.draw(shadow);
                    sf::Sprite sp(*tex);
                    sp.setScale(sf::Vector2f(s, s));
                    sp.setPosition(sf::Vector2f(px, py));
                    window.draw(sp);
                }
            }
        }

        drawReviewOverlay(window);
    } else {
        drawBoard(window);
        drawCoordinates(window);
        drawLastMove(window);
        drawSelection(window);
        drawPieces(window);
        drawSidePanel(window);
        drawMoveHistory(window);

        if (aiThinking) drawThinking(window);
        if (awaitingPromotion) drawPromotionDialog(window);
        if (gameOver) drawGameOver(window);

        // Dragged piece on top
        if (dragging) {
            PieceType dpt = board.getPiece(dragRow, dragCol);
            PieceColor dpc = board.getColor(dragRow, dragCol);
            const sf::Texture* tex = assetManager ? assetManager->getPieceTexture(dpt, dpc) : nullptr;
            if (tex) {
                float s = (squareSize - 8.0f) / tex->getSize().x;
                sf::Sprite spr(*tex);
                spr.setScale(sf::Vector2f(s, s));
                spr.setPosition(sf::Vector2f((float)(dragMousePos.x - squareSize / 2), (float)(dragMousePos.y - squareSize / 2)));
                spr.setColor(sf::Color(255, 255, 255, 220));
                window.draw(spr);
            }
        }

        drawRightDragArrow(window);
        drawHint(window);
        drawParticles(window);
        if (exitConfirmActive) drawExitConfirm(window);
    }

    window.setView(window.getDefaultView());
}

// ── Algebraic notation ──
std::string Game::moveToAlgebraic(const Move& move) {
    if (move.fromRow < 0 || move.fromRow > 7 || move.fromCol < 0 || move.fromCol > 7) return "";
    if (board.getPiece(move.fromRow, move.fromCol) == NONE) return "";
    static const char* files = "abcdefgh";
    static const char* pchar = "  NBRQK";
    PieceType pt = board.getPiece(move.fromRow, move.fromCol);
    PieceColor pc = board.getColor(move.fromRow, move.fromCol);
    std::string res;
    if (pt == KING && std::abs(move.toCol - move.fromCol) == 2)
        return (move.toCol > move.fromCol) ? "O-O" : "O-O-O";
    if (pt != PAWN) res += pchar[pt];
    bool capture = board.getPiece(move.toRow, move.toCol) != NONE ||
                   (pt == PAWN && move.toCol != move.fromCol &&
                    board.getPiece(move.toRow, move.toCol) == NONE);
    if (capture) {
        if (pt == PAWN) res += files[move.fromCol];
        res += "x";
    }
    res += files[move.toCol];
    res += std::to_string(8 - move.toRow);
    // Check/stalemate suffix added after the move is made — omitted here
    return res;
}

// ── Undo ──
void Game::undoLastMove() {
    if (mode == MODE_VS_BOT && difficulty >= HARD) return;
    if (undoStack.empty()) return;
    Move move = undoStack.back().first;
    BoardState state = undoStack.back().second;
    undoStack.pop_back();
    board.unmakeMove(move, state);
    if (!capturedWhite.empty() && capturedWhite.back().color == board.getColor(move.toRow, move.toCol))
        capturedWhite.pop_back();
    else if (!capturedBlack.empty() && capturedBlack.back().color == board.getColor(move.toRow, move.toCol))
        capturedBlack.pop_back();
    else if (!capturedWhite.empty() && capturedWhite.back().color == BLACK)
        capturedWhite.pop_back();
    else if (!capturedBlack.empty() && capturedBlack.back().color == WHITE)
        capturedBlack.pop_back();
    if (!moveHistory.empty()) moveHistory.pop_back();
    if (!openingHistory.empty()) openingHistory.pop_back();
    currentPlayer = board.getCurrentPlayer();
    pieceSelected = false;
    hasLastMove = undoStack.empty() ? false : true;
    if (hasLastMove) {
        lastFromRow = undoStack.back().first.fromRow;
        lastFromCol = undoStack.back().first.fromCol;
        lastToRow = undoStack.back().first.toRow;
        lastToCol = undoStack.back().first.toCol;
    }
}

// ── Hint ──
void Game::showHint() {
    if (!ai && mode == MODE_LOCAL_MULTIPLAYER) {
        ChessAI tempAI(MEDIUM);
        Move best = tempAI.getBestMove(board);
        if (best.fromRow >= 0) {
            hintText = "Best: " + moveToAlgebraic(best);
            hintActive = true;
        } else {
            hintText = "No moves available.";
            hintActive = true;
        }
        return;
    }
    if (!ai) return;
    Move best = ai->getBestMove(board);
    if (best.fromRow >= 0) {
        hintText = "Best: " + moveToAlgebraic(best);
        hintActive = true;
    }
}

void Game::clearHint() { hintActive = false; hintText.clear(); }

// ── Coordinates ──
void Game::drawCoordinates(sf::RenderWindow& window) {
    if (!showCoordinates || !assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    bool dm = !settings || settings->darkMode;
    const auto& th = settings ? settings->getTheme() : BOARD_THEMES[0];
    sf::Color coordCol(th.accentR, th.accentG, th.accentB, dm ? 160 : 140);
    static const char* files = "abcdefgh";
    for (int i = 0; i < 8; ++i) {
        int ci = boardFlipped ? (7 - i) : i;
        // File letters below board
        float fx = boardX + ci * squareSize + squareSize / 2.0f;
        float fy = boardY + boardSize + squareSize * 0.06f;
        sf::Text ft(*font, std::string(1, files[i]), (unsigned)(squareSize * 0.25f));
        ft.setFillColor(coordCol);
        sf::FloatRect fb = ft.getLocalBounds();
        ft.setPosition(sf::Vector2f(fx - fb.size.x / 2, fy));
        window.draw(ft);
        // Rank numbers on the left
        int ri = boardFlipped ? (7 - i) : i;
        float rx = boardX - squareSize * 0.15f;
        float ry = boardY + ri * squareSize + squareSize / 2.0f;
        sf::Text rt(*font, std::to_string(8 - i), (unsigned)(squareSize * 0.25f));
        rt.setFillColor(coordCol);
        sf::FloatRect rb = rt.getLocalBounds();
        rt.setPosition(sf::Vector2f(rx - rb.size.x / 2, ry - rb.size.y / 2));
        window.draw(rt);
    }
}

// ── Move history ──
void Game::drawMoveHistory(sf::RenderWindow& window) {
    if (!assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    bool dm = !settings || settings->darkMode;
    sf::Color pMuted = panelMuted(dm);
    int px = boardX + boardSize + 18;
    int py = boardY + boardSize - 100;
    float lineH = 12.0f;
    int maxLines = 6;
    int start = std::max(0, (int)moveHistory.size() - maxLines);
    for (int i = 0; i < maxLines && start + i < (int)moveHistory.size(); ++i) {
        int idx = start + i;
        std::string label = std::to_string(idx / 2 + 1) + (idx % 2 == 0 ? ". " : ".. ") + moveHistory[idx];
        sf::Text t(*font, label, 9);

        // Color blunders/mistakes/inaccuracies
        if (idx < (int)annotations.size()) {
            switch (annotations[idx]) {
                case ANNO_BLUNDER:    t.setFillColor(sf::Color(255, 80, 80, 220)); break;
                case ANNO_MISTAKE:    t.setFillColor(sf::Color(255, 160, 60, 220)); break;
                case ANNO_INACCURACY: t.setFillColor(sf::Color(255, 220, 80, 220)); break;
                case ANNO_BRILLIANT:  t.setFillColor(sf::Color(0, 200, 255, 220)); break;
                case ANNO_GREAT:      t.setFillColor(sf::Color(80, 220, 80, 220)); break;
                default:              t.setFillColor(pMuted); break;
            }
        } else {
            t.setFillColor(pMuted);
        }
        t.setPosition(sf::Vector2f((float)(px + 6), (float)(py + i * lineH)));
        window.draw(t);
    }
}

// ── Hint overlay ──
void Game::drawHint(sf::RenderWindow& window) {
    if (!hintActive || hintText.empty() || !assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    int cx = boardX + boardSize / 2;
    int cy = boardY + boardSize - 30;
    sf::Text t(*font, hintText, 15);
    t.setFillColor(sf::Color(100, 200, 100, 240));
    sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin(sf::Vector2f(tb.size.x / 2, tb.size.y / 2));
    t.setPosition(sf::Vector2f(cx, cy));
    // Background pill
    float pad = 8.0f;
    drawFilledRoundRect(window, cx - tb.size.x / 2 - pad, cy - tb.size.y / 2 - pad,
                        tb.size.x + pad * 2, tb.size.y + pad * 2, 10,
                        sf::Color(0, 0, 0, 180));
    window.draw(t);
}

void Game::drawRightDragArrow(sf::RenderWindow& window) {
    // Show arrow while right-dragging or for 2 s after release
    bool show = rightDragging || (showRightDragArrow && rightDragClock.getElapsedTime().asSeconds() < 2.0f);
    if (!show) return;

    int fr = rightDragFromRow, fc = rightDragFromCol;
    int tr = rightDragToRow, tc = rightDragToCol;
    if (fr == tr && fc == tc) return;

    int dfr = boardFlipped ? (7 - fr) : fr;
    int dfc = boardFlipped ? (7 - fc) : fc;
    int dtr = boardFlipped ? (7 - tr) : tr;
    int dtc = boardFlipped ? (7 - tc) : tc;

    float fx = boardX + dfc * squareSize + squareSize / 2.0f;
    float fy = boardY + dfr * squareSize + squareSize / 2.0f;
    float tx = boardX + dtc * squareSize + squareSize / 2.0f;
    float ty = boardY + dtr * squareSize + squareSize / 2.0f;

    float dx = tx - fx;
    float dy = ty - fy;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return;

    float ux = dx / len;
    float uy = dy / len;

    sf::Color arrowColor(100, 150, 255, 180);
    float thickness = 8.0f;
    float headLen = 14.0f;
    float headHalf = 7.0f;

    // Shorten line so it stops before the arrowhead
    float lineEnd = len - headLen;
    float lex = fx + ux * lineEnd;
    float ley = fy + uy * lineEnd;

    // ── Arrow shaft (thick line as a rotated rectangle) ──
    sf::RectangleShape shaft({lineEnd, thickness});
    shaft.setFillColor(arrowColor);
    shaft.setOrigin({0.0f, thickness / 2.0f});
    shaft.setPosition({fx, fy});
    shaft.setRotation(sf::radians(std::atan2(dy, dx)));
    window.draw(shaft);

    // ── Arrowhead (triangle) ──
    sf::ConvexShape head;
    head.setPointCount(3);
    head.setPoint(0, {headLen, 0.0f});
    head.setPoint(1, {0.0f, -headHalf});
    head.setPoint(2, {0.0f, headHalf});
    head.setFillColor(arrowColor);
    head.setOrigin({0.0f, 0.0f});
    head.setPosition({tx, ty});
    head.setRotation(sf::radians(std::atan2(dy, dx)));
    window.draw(head);
}

void Game::drawExitConfirm(sf::RenderWindow& window) {
    if (!assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    int cx = boardX + boardSize / 2;
    int cy = boardY + boardSize / 2;

    // Dim overlay
    sf::RectangleShape overlay(sf::Vector2f(boardSize, boardSize));
    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    overlay.setPosition(sf::Vector2f(boardX, boardY));
    window.draw(overlay);

    // Card
    drawFilledRoundRect(window, cx - 150, cy - 60, 300, 140, 14,
                        sf::Color(30, 24, 20, 240));

    sf::RectangleShape cardBorder(sf::Vector2f(298, 138));
    cardBorder.setFillColor(sf::Color::Transparent);
    cardBorder.setOutlineColor(sf::Color(212, 175, 55, 80));
    cardBorder.setOutlineThickness(1);
    cardBorder.setPosition(sf::Vector2f(cx - 149, cy - 59));
    window.draw(cardBorder);

    // Title
    sf::Text title(*font, "Want to Exit?", 20);
    title.setFillColor(sf::Color(220, 200, 170, 255));
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(sf::Vector2f(tb.size.x / 2, tb.size.y / 2));
    title.setPosition(sf::Vector2f(cx, cy - 32));
    window.draw(title);

    // Subtitle
    sf::Text sub(*font, "Your game will be lost.", 12);
    sub.setFillColor(sf::Color(180, 160, 130, 200));
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setOrigin(sf::Vector2f(sb.size.x / 2, sb.size.y / 2));
    sub.setPosition(sf::Vector2f(cx, cy - 6));
    window.draw(sub);

    // "No" (cancel) button
    exitConfirmNoBounds = sf::FloatRect({(float)(cx - 130), (float)(cy + 18)}, {120.0f, 32.0f});
    bool noHover = exitConfirmNoBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
    drawFilledRoundRect(window, exitConfirmNoBounds.position.x, exitConfirmNoBounds.position.y,
                        exitConfirmNoBounds.size.x, exitConfirmNoBounds.size.y, 8,
                        noHover ? sf::Color(70, 60, 50, 230) : sf::Color(55, 45, 35, 200));
    sf::RectangleShape noBorder(sf::Vector2f(118, 30));
    noBorder.setFillColor(sf::Color::Transparent);
    noBorder.setOutlineColor(noHover ? sf::Color(212, 175, 55, 180) : sf::Color(212, 175, 55, 80));
    noBorder.setOutlineThickness(1);
    noBorder.setPosition(sf::Vector2f(cx - 129, cy + 19));
    window.draw(noBorder);
    sf::Text noText(*font, "Stay", 13);
    noText.setFillColor(noHover ? sf::Color(220, 200, 170, 255) : sf::Color(180, 160, 130, 220));
    sf::FloatRect nt = noText.getLocalBounds();
    noText.setPosition(sf::Vector2f(
        exitConfirmNoBounds.position.x + exitConfirmNoBounds.size.x / 2.0f - nt.size.x / 2.0f,
        exitConfirmNoBounds.position.y + exitConfirmNoBounds.size.y / 2.0f - nt.size.y / 2.0f - 1
    ));
    window.draw(noText);

    // "Yes" (confirm) button
    exitConfirmYesBounds = sf::FloatRect({(float)(cx + 10), (float)(cy + 18)}, {120.0f, 32.0f});
    bool yesHover = exitConfirmYesBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
    drawFilledRoundRect(window, exitConfirmYesBounds.position.x, exitConfirmYesBounds.position.y,
                        exitConfirmYesBounds.size.x, exitConfirmYesBounds.size.y, 8,
                        yesHover ? sf::Color(180, 70, 70, 240) : sf::Color(140, 50, 50, 180));
    sf::RectangleShape yesBorder(sf::Vector2f(118, 30));
    yesBorder.setFillColor(sf::Color::Transparent);
    yesBorder.setOutlineColor(yesHover ? sf::Color(220, 120, 120, 200) : sf::Color(180, 80, 80, 100));
    yesBorder.setOutlineThickness(1);
    yesBorder.setPosition(sf::Vector2f(cx + 11, cy + 19));
    window.draw(yesBorder);
    sf::Text yesText(*font, "Exit", 13);
    yesText.setFillColor(sf::Color(240, 220, 220, 230));
    sf::FloatRect yt = yesText.getLocalBounds();
    yesText.setPosition(sf::Vector2f(
        exitConfirmYesBounds.position.x + exitConfirmYesBounds.size.x / 2.0f - yt.size.x / 2.0f,
        exitConfirmYesBounds.position.y + exitConfirmYesBounds.size.y / 2.0f - yt.size.y / 2.0f - 1
    ));
    window.draw(yesText);
}

void Game::drawBoard(sf::RenderWindow& window) {
    const auto& theme = settings ? settings->getTheme() : BOARD_THEMES[0];
    sf::Color light(theme.lightR, theme.lightG, theme.lightB);
    sf::Color dark(theme.darkR, theme.darkG, theme.darkB);
    bool dm = !settings || settings->darkMode;
    sf::Color frameCol = dm ? sf::Color(40, 28, 16) : sf::Color(160, 130, 100);
    sf::Color goldTrim(theme.accentR, theme.accentG, theme.accentB, dm ? 180 : 120);

    sf::RectangleShape outerFrame(sf::Vector2f(boardSize + 24, boardSize + 24));
    outerFrame.setFillColor(frameCol);
    outerFrame.setPosition(sf::Vector2f(boardX - 12, boardY - 12));
    window.draw(outerFrame);

    sf::RectangleShape goldBorder(sf::Vector2f(boardSize + 12, boardSize + 12));
    goldBorder.setFillColor(goldTrim);
    goldBorder.setPosition(sf::Vector2f(boardX - 6, boardY - 6));
    window.draw(goldBorder);

    sf::RectangleShape innerFrame(sf::Vector2f(boardSize + 4, boardSize + 4));
    innerFrame.setFillColor(frameCol);
    innerFrame.setPosition(sf::Vector2f(boardX - 2, boardY - 2));
    window.draw(innerFrame);

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            int dr = boardFlipped ? (7 - row) : row;
            int dc = boardFlipped ? (7 - col) : col;
            sf::RectangleShape sq(sf::Vector2f(squareSize, squareSize));
            sq.setFillColor((row + col) % 2 == 0 ? light : dark);
            sq.setPosition(sf::Vector2f(boardX + dc * squareSize, boardY + dr * squareSize));
            window.draw(sq);
        }
    }
}

void Game::drawLastMove(sf::RenderWindow& window) {
    if (!hasLastMove) return;
    const auto& th = settings ? settings->getTheme() : BOARD_THEMES[0];
    sf::Color highlight(th.accentR, th.accentG, th.accentB, 70);

    for (int r : {lastFromRow, lastToRow}) {
        int c = (r == lastFromRow) ? lastFromCol : lastToCol;
        int dr = boardFlipped ? (7 - r) : r;
        int dc = boardFlipped ? (7 - c) : c;
        sf::RectangleShape sq(sf::Vector2f(squareSize, squareSize));
        sq.setFillColor(highlight);
        sq.setPosition(sf::Vector2f(boardX + dc * squareSize, boardY + dr * squareSize));
        window.draw(sq);
    }
}

void Game::drawPieces(sf::RenderWindow& window) {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            PieceType pt = board.getPiece(row, col);
            if (pt == NONE) continue;

            if (anim.active && row == anim.toRow && col == anim.toCol)
                continue;

            if (dragging && row == dragRow && col == dragCol)
                continue;

            PieceColor c = board.getColor(row, col);
            const sf::Texture* tex = assetManager ? assetManager->getPieceTexture(pt, c) : nullptr;
            if (!tex) continue;

            float s = (squareSize - 8.0f) / tex->getSize().x;
            int dr = boardFlipped ? (7 - row) : row;
            int dc = boardFlipped ? (7 - col) : col;
            float px = boardX + dc * squareSize + 4;
            float py = boardY + dr * squareSize + 4;

            sf::Sprite shadow(*tex);
            shadow.setScale(sf::Vector2f(s, s));
            shadow.setPosition(sf::Vector2f(px + 2, py + 3));
            shadow.setColor(sf::Color(0, 0, 0, 60));
            window.draw(shadow);

            sf::Sprite sp(*tex);
            sp.setScale(sf::Vector2f(s, s));
            sp.setPosition(sf::Vector2f(px, py));
            window.draw(sp);

            if (pt == KING && !gameOver && board.isInCheck(c)) {
                sf::CircleShape glow(squareSize * 0.55f);
                glow.setFillColor(sf::Color::Transparent);
                glow.setOutlineColor(sf::Color(200, 60, 60, 200));
                glow.setOutlineThickness(3);
                glow.setPosition(sf::Vector2f(px, py));
                window.draw(glow);
            }
        }
    }

    if (anim.active) {
        float t = anim.clock.getElapsedTime().asSeconds() / anim.duration;
        if (t >= 1.0f) {
            anim.active = false;
            return;
        }
        t = easeOutBack(std::min(t, 1.0f));

        float curRow = anim.fromRow + (anim.toRow - anim.fromRow) * t;
        float curCol = anim.fromCol + (anim.toCol - anim.fromCol) * t;

        const sf::Texture* tex = assetManager ? assetManager->getPieceTexture(anim.type, anim.color) : nullptr;
        if (tex) {
            float s = (squareSize - 8.0f) / tex->getSize().x;
            float drawRow = boardFlipped ? (7.0f - curRow) : curRow;
            float drawCol = boardFlipped ? (7.0f - curCol) : curCol;
            float px = boardX + drawCol * squareSize + 4;
            float py = boardY + drawRow * squareSize + 4;

            sf::Sprite shadow(*tex);
            shadow.setScale(sf::Vector2f(s, s));
            shadow.setPosition(sf::Vector2f(px + 2, py + 3));
            shadow.setColor(sf::Color(0, 0, 0, 60));
            window.draw(shadow);

            sf::Sprite sp(*tex);
            sp.setScale(sf::Vector2f(s, s));
            sp.setPosition(sf::Vector2f(px, py));
            window.draw(sp);
        }
    }
}

void Game::drawSelection(sf::RenderWindow& window) {
    if (!pieceSelected) return;
    const auto& th = settings ? settings->getTheme() : BOARD_THEMES[0];
    sf::Color ac(th.accentR, th.accentG, th.accentB);

    int sdr = boardFlipped ? (7 - selectedRow) : selectedRow;
    int sdc = boardFlipped ? (7 - selectedCol) : selectedCol;

    sf::RectangleShape sel(sf::Vector2f(squareSize, squareSize));
    sel.setFillColor(sf::Color(ac.r, ac.g, ac.b, 45));
    sel.setPosition(sf::Vector2f(boardX + sdc * squareSize, boardY + sdr * squareSize));
    window.draw(sel);

    sf::RectangleShape outline(sf::Vector2f(squareSize, squareSize));
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineColor(sf::Color(ac.r, ac.g, ac.b, 180));
    outline.setOutlineThickness(2);
    outline.setPosition(sf::Vector2f(boardX + sdc * squareSize, boardY + sdr * squareSize));
    window.draw(outline);

    PieceType selPiece = board.getPiece(selectedRow, selectedCol);
    PieceColor selColorType = board.getColor(selectedRow, selectedCol);
    if (selPiece == NONE || selColorType != currentPlayer) return;

    std::vector<Move> legal = board.generateLegalMoves();

    for (const Move& m : legal) {
        if (m.fromRow != selectedRow || m.fromCol != selectedCol) continue;
        int mdr = boardFlipped ? (7 - m.toRow) : m.toRow;
        int mdc = boardFlipped ? (7 - m.toCol) : m.toCol;
        float cx = boardX + mdc * squareSize + squareSize / 2.0f;
        float cy = boardY + mdr * squareSize + squareSize / 2.0f;

        if (board.getPiece(m.toRow, m.toCol) != NONE) {
            sf::CircleShape ring(squareSize * 0.42f);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(sf::Color(ac.r, ac.g, ac.b, 180));
            ring.setOutlineThickness(3);
            ring.setOrigin(sf::Vector2f(ring.getRadius(), ring.getRadius()));
            ring.setPosition(sf::Vector2f(cx, cy));
            window.draw(ring);
        } else {
            sf::CircleShape dot(squareSize * 0.12f);
            dot.setFillColor(sf::Color(ac.r, ac.g, ac.b, 120));
            dot.setOrigin(sf::Vector2f(dot.getRadius(), dot.getRadius()));
            dot.setPosition(sf::Vector2f(cx, cy));
            window.draw(dot);
        }
    }
}

void Game::drawSpeechBubble(sf::RenderWindow& window, const std::string& text,
                            float bx, float by, float maxWidth) {
    if (!assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();

    sf::Text txt(*font, text, 11);
    txt.setFillColor(sf::Color(240, 235, 225, 230));
    sf::FloatRect tb = txt.getLocalBounds();
    float tw = tb.size.x + 20;
    float th = tb.size.y + 14;
    if (tw > maxWidth) tw = maxWidth;

    float bx2 = bx + (maxWidth - tw) / 2.0f;

    drawFilledRoundRect(window, bx2, by, tw, th, 8, sf::Color(40, 35, 45, 220));

    const auto& boardTh = settings ? settings->getTheme() : BOARD_THEMES[0];
    sf::RectangleShape border(sf::Vector2f(tw - 2, th - 2));
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(boardTh.accentR, boardTh.accentG, boardTh.accentB, 60));
    border.setOutlineThickness(1);
    border.setPosition(sf::Vector2f(bx2 + 1, by + 1));
    window.draw(border);

    float ptrX = bx2 + tw / 2.0f;
    sf::ConvexShape ptr;
    ptr.setPointCount(3);
    ptr.setPoint(0, sf::Vector2f(ptrX - 6, by));
    ptr.setPoint(1, sf::Vector2f(ptrX + 6, by));
    ptr.setPoint(2, sf::Vector2f(ptrX, by - 6));
    ptr.setFillColor(sf::Color(40, 35, 45, 220));
    window.draw(ptr);

    txt.setPosition(sf::Vector2f(bx2 + 10, by + 7));
    window.draw(txt);
}

void Game::drawSidePanel(sf::RenderWindow& window) {
    if (!assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    bool dm = !settings || settings->darkMode;
    sf::Color pBg = panelBg(dm);
    sf::Color pText = panelText(dm);
    sf::Color pMuted = panelMuted(dm);
    const auto& theme = settings ? settings->getTheme() : BOARD_THEMES[0];
    sf::Color ac(theme.accentR, theme.accentG, theme.accentB);

    if (!portrait) {
        // Landscape: panel to the right
        int px = boardX + boardSize + 18;
        int py = boardY;

        sf::RectangleShape panelBg(sf::Vector2f((float)panelWidth, (float)boardSize));
        panelBg.setFillColor(pBg);
        panelBg.setOutlineColor(sf::Color(100, 80, 55, dm ? 60 : 20));
        panelBg.setOutlineThickness(1);
        panelBg.setPosition(sf::Vector2f((float)(px - 8), (float)py));
        window.draw(panelBg);

        sf::RectangleShape panelAccent(sf::Vector2f(2, (float)boardSize));
        panelAccent.setFillColor(sf::Color(ac.r, ac.g, ac.b, dm ? 60 : 30));
        panelAccent.setPosition(sf::Vector2f((float)(px - 8), (float)py));
        window.draw(panelAccent);

        int avatarSize = 55;
        int avatarX = px + 12;
        int avatarY = py + 10;

        sf::Color glowColor;
        switch (botDialog.expression) {
            case EXPR_HAPPY:    glowColor = sf::Color(80, 200, 80, dm ? 80 : 100); break;
            case EXPR_THINKING: glowColor = sf::Color(200, 200, 50, dm ? 80 : 100); break;
            case EXPR_SURPRISED:glowColor = sf::Color(200, 100, 50, dm ? 80 : 100); break;
            case EXPR_SAD:      glowColor = sf::Color(100, 100, 200, dm ? 80 : 100); break;
            default:            glowColor = sf::Color(180, 150, 75, dm ? 50 : 70); break;
        }
        sf::CircleShape glow(avatarSize / 2.0f + 5);
        glow.setFillColor(glowColor);
        glow.setPosition(sf::Vector2f((float)(avatarX - 5), (float)(avatarY - 5)));
        window.draw(glow);

        sf::Sprite avatar(botAvatar);
        if (botAvatar.getSize().x > 0) {
            float avs = avatarSize / (float)botAvatar.getSize().x;
            avatar.setScale(sf::Vector2f(avs, avs));
        }
        avatar.setPosition(sf::Vector2f((float)avatarX, (float)avatarY));
        window.draw(avatar);

        sf::Text topName(*font, mode == MODE_VS_BOT ? botName : "Player 2 (Black)", 13);
        topName.setFillColor(pText);
        topName.setPosition(sf::Vector2f((float)(px + (mode == MODE_VS_BOT ? avatarSize + 12 : 8)), (float)(py + 22)));
        window.draw(topName);

        if (mode == MODE_VS_BOT && botDialog.active)
            drawSpeechBubble(window, botDialog.text, (float)(px + 8), (float)(py + avatarSize + 14), (float)(panelWidth - 16));

        float bubbleH = botDialog.active ? 50 : 8;
        PieceColor oppColor = (playerColor == WHITE) ? BLACK : WHITE;
        drawCapturedPiecesAt(window, (playerColor == WHITE) ? capturedBlack : capturedWhite, px + 10, py + avatarSize + 10 + (int)bubbleH, playerColor);

        float vsY = py + boardSize / 2 - 16;
        sf::Text vs(*font, "vs", 11);
        vs.setFillColor(pMuted);
        vs.setPosition(sf::Vector2f((float)(px + 10), vsY));
        window.draw(vs);

        float by = py + boardSize / 2 + 14;

        sf::Text youName(*font, (playerColor == WHITE) ? "You (White)" : "You (Black)", 13);
        youName.setFillColor(pText);
        youName.setPosition(sf::Vector2f((float)(px + 10), by));
        window.draw(youName);

        drawCapturedPiecesAt(window, (playerColor == WHITE) ? capturedWhite : capturedBlack, px + 10, (int)by + 24, oppColor);

        if (!gameOver) {
            std::string turn = (currentPlayer == playerColor) ? "Your turn" : (mode == MODE_VS_BOT ? botName + "'s turn" : (playerColor == WHITE ? "Black's turn" : "White's turn"));
            sf::Text turnText(*font, turn, 11);
            turnText.setFillColor(currentPlayer == playerColor ? pText : sf::Color(180, 150, 200, 200));
            turnText.setPosition(sf::Vector2f((float)(px + 10), by + 60));
            window.draw(turnText);
        }

        if (board.isInCheck() && !gameOver) {
            sf::Text check(*font, "Check!", 14);
            check.setFillColor(sf::Color(200, 70, 70, 220));
            check.setPosition(sf::Vector2f((float)(px + 10), by + 82));
            window.draw(check);
        }

        // Action buttons row (Undo / Hint / Flip / Coord)
        float btnY = py + boardSize - 82;
        float btnW = 50.0f;
        float btnH = 28.0f;
        float gap = 6.0f;
        float totalW = btnW * 4 + gap * 3;
        float btnStart = px + 10 + (230 - totalW) / 2.0f;

        auto drawMiniBtn = [&](const sf::FloatRect& bounds, const std::string& label,
                               sf::Color bg, sf::Color border, bool active) {
            sf::Color fg = active ? sf::Color(220, 200, 170, 255) : sf::Color(160, 140, 120, 200);
            drawFilledRoundRect(window, bounds.position.x, bounds.position.y,
                                bounds.size.x, bounds.size.y, 6, bg);
            sf::RectangleShape brd(sf::Vector2f(bounds.size.x - 2, bounds.size.y - 2));
            brd.setFillColor(sf::Color::Transparent);
            brd.setOutlineColor(border);
            brd.setOutlineThickness(1);
            brd.setPosition(sf::Vector2f(bounds.position.x + 1, bounds.position.y + 1));
            window.draw(brd);
            sf::Text lbl(*font, label, 9);
            lbl.setFillColor(fg);
            sf::FloatRect lb = lbl.getLocalBounds();
            lbl.setPosition(sf::Vector2f(
                bounds.position.x + bounds.size.x / 2.0f - lb.size.x / 2.0f,
                bounds.position.y + bounds.size.y / 2.0f - lb.size.y / 2.0f - 1));
            window.draw(lbl);
        };

        undoBtnBounds = sf::FloatRect({btnStart, btnY}, {btnW, btnH});
        bool canUndo = !undoStack.empty() && !(mode == MODE_VS_BOT && difficulty >= HARD);
        drawMiniBtn(undoBtnBounds, "Undo",
                    canUndo ? sf::Color(55, 50, 45, 200) : sf::Color(35, 30, 25, 160),
                    canUndo ? sf::Color(212, 175, 55, 100) : sf::Color(80, 80, 70, 60), canUndo);

        hintBtnBounds = sf::FloatRect({btnStart + (btnW + gap), btnY}, {btnW, btnH});
        drawMiniBtn(hintBtnBounds, "Hint",
                    sf::Color(50, 70, 50, 200), sf::Color(100, 180, 100, 100), true);

        flipBtnBounds = sf::FloatRect({btnStart + (btnW + gap) * 2, btnY}, {btnW, btnH});
        drawMiniBtn(flipBtnBounds, boardFlipped ? "W" : "B",
                    sf::Color(55, 50, 45, 200), sf::Color(212, 175, 55, 100), true);

        coordBtnBounds = sf::FloatRect({btnStart + (btnW + gap) * 3, btnY}, {btnW, btnH});
        drawMiniBtn(coordBtnBounds, "Coord",
                    showCoordinates ? sf::Color(55, 60, 50, 200) : sf::Color(35, 30, 25, 160),
                    showCoordinates ? sf::Color(100, 180, 100, 100) : sf::Color(80, 80, 70, 60),
                    showCoordinates);

        float exitY = py + boardSize - 42;
        exitBtnBounds = sf::FloatRect({(float)(px + 10), exitY}, {230.0f, 32.0f});
        bool exitHover = exitBtnBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
        drawFilledRoundRect(window, exitBtnBounds.position.x, exitBtnBounds.position.y,
                            exitBtnBounds.size.x, exitBtnBounds.size.y, 8,
                            exitHover ? sf::Color(180, 70, 70, 200) : sf::Color(140, 50, 50, 160));
        sf::Text exitText(*font, "Exit to Menu", 11);
        exitText.setFillColor(sf::Color(240, 220, 200, 230));
        sf::FloatRect eb = exitText.getLocalBounds();
        exitText.setPosition(sf::Vector2f(exitBtnBounds.position.x + exitBtnBounds.size.x / 2.0f - eb.size.x / 2.0f,
                                           exitBtnBounds.position.y + exitBtnBounds.size.y / 2.0f - eb.size.y / 2.0f - 1));
        window.draw(exitText);

    } else {
        // Portrait: compact panel below board
        int px = 8;
        int py = boardY + boardSize + 8;
        int pw = winW - 16;

        sf::RectangleShape panelBg(sf::Vector2f((float)pw, 70.0f));
        panelBg.setFillColor(pBg);
        panelBg.setOutlineColor(sf::Color(100, 80, 55, dm ? 60 : 20));
        panelBg.setOutlineThickness(1);
        panelBg.setPosition(sf::Vector2f((float)px, (float)py));
        window.draw(panelBg);

        int avatarSize = 36;
        int avatarX = px + 8;
        int avatarY = py + 8;

        sf::Color glowColor;
        switch (botDialog.expression) {
            case EXPR_HAPPY:    glowColor = sf::Color(80, 200, 80, dm ? 80 : 100); break;
            case EXPR_THINKING: glowColor = sf::Color(200, 200, 50, dm ? 80 : 100); break;
            case EXPR_SURPRISED:glowColor = sf::Color(200, 100, 50, dm ? 80 : 100); break;
            case EXPR_SAD:      glowColor = sf::Color(100, 100, 200, dm ? 80 : 100); break;
            default:            glowColor = sf::Color(180, 150, 75, dm ? 50 : 70); break;
        }
        sf::CircleShape glow(avatarSize / 2.0f + 4);
        glow.setFillColor(glowColor);
        glow.setPosition(sf::Vector2f((float)(avatarX - 4), (float)(avatarY - 4)));
        window.draw(glow);

        sf::Sprite avatar(botAvatar);
        if (botAvatar.getSize().x > 0) {
            float avs = avatarSize / (float)botAvatar.getSize().x;
            avatar.setScale(sf::Vector2f(avs, avs));
        }
        avatar.setPosition(sf::Vector2f((float)avatarX, (float)avatarY));
        window.draw(avatar);

        std::string nameStr = mode == MODE_VS_BOT ? botName : "Player 2";
        sf::Text nameT(*font, nameStr, 11);
        nameT.setFillColor(pText);
        nameT.setPosition(sf::Vector2f((float)(avatarX + avatarSize + 8), (float)(avatarY + 2)));
        window.draw(nameT);

        std::string turnStr;
        if (gameOver) turnStr = "Game Over";
        else turnStr = (currentPlayer == playerColor) ? "Your turn" : (mode == MODE_VS_BOT ? botName + "'s turn" : (playerColor == WHITE ? "Black's turn" : "White's turn"));
        sf::Text turnT(*font, turnStr, 9);
        turnT.setFillColor(pMuted);
        turnT.setPosition(sf::Vector2f((float)(avatarX + avatarSize + 8), (float)(avatarY + 18)));
        window.draw(turnT);

        if (mode == MODE_VS_BOT && botDialog.active)
            drawSpeechBubble(window, botDialog.text, (float)(avatarSize + 70), (float)(avatarY - 4), (float)(pw - avatarSize - 80));

        PieceColor oppColor = (playerColor == WHITE) ? BLACK : WHITE;
        int capsX = px + pw - 110;
        drawCapturedPiecesAt(window, (playerColor == WHITE) ? capturedBlack : capturedWhite, capsX, avatarY + 2, playerColor);
        drawCapturedPiecesAt(window, (playerColor == WHITE) ? capturedWhite : capturedBlack, capsX, avatarY + 22, oppColor);

        float exitY = py + 74;
        exitBtnBounds = sf::FloatRect({(float)(px + 4), exitY}, {(float)(pw - 8), 26.0f});
        bool exitHover = exitBtnBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));
        drawFilledRoundRect(window, exitBtnBounds.position.x, exitBtnBounds.position.y,
                            exitBtnBounds.size.x, exitBtnBounds.size.y, 6,
                            exitHover ? sf::Color(180, 70, 70, 200) : sf::Color(140, 50, 50, 160));
        sf::Text exitText(*font, "Exit to Menu", 10);
        exitText.setFillColor(sf::Color(240, 220, 200, 230));
        sf::FloatRect eb = exitText.getLocalBounds();
        exitText.setPosition(sf::Vector2f(exitBtnBounds.position.x + exitBtnBounds.size.x / 2.0f - eb.size.x / 2.0f,
                                           exitBtnBounds.position.y + exitBtnBounds.size.y / 2.0f - eb.size.y / 2.0f - 1));
        window.draw(exitText);
    }
}

void Game::drawCapturedPiecesAt(sf::RenderWindow& window, const std::vector<CapturedPiece>& pieces,
                                 int x, int y, PieceColor side) {
    int count = 0;
    for (const auto& cp : pieces) {
        if (cp.color != side) continue;
        const sf::Texture* tex = assetManager ? assetManager->getPieceTexture(cp.type, cp.color) : nullptr;
        if (!tex) continue;

        float s = 20.0f / tex->getSize().x;
        float px = x + count * 18;
        float py = y;

        sf::Sprite shadow(*tex);
        shadow.setScale(sf::Vector2f(s, s));
        shadow.setPosition(sf::Vector2f(px + 1, py + 1));
        shadow.setColor(sf::Color(0, 0, 0, 40));
        window.draw(shadow);

        sf::Sprite sp(*tex);
        sp.setScale(sf::Vector2f(s, s));
        sp.setPosition(sf::Vector2f(px, py));
        window.draw(sp);
        count++;
        if (count >= 12) break;
    }
}

void Game::drawThinking(sf::RenderWindow& window) {
    if (!assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    int cx = boardX + boardSize / 2;
    int cy = boardY + boardSize / 2;

    sf::RectangleShape overlay(sf::Vector2f(boardSize, boardSize));
    overlay.setFillColor(sf::Color(0, 0, 0, 120));
    overlay.setPosition(sf::Vector2f(boardX, boardY));
    window.draw(overlay);

    static sf::Clock dotClock;
    static int dots = 1;
    if (dotClock.getElapsedTime().asMilliseconds() > 400) {
        dots = (dots % 3) + 1;
        dotClock.restart();
    }

    std::string prefix = (aiMoveReady) ? "Move ready" : "Thinking";
    sf::Text txt(*font, prefix + std::string(dots, '.'), 26);
    txt.setFillColor(sf::Color(220, 200, 170, 230));
    sf::FloatRect b = txt.getLocalBounds();
    txt.setOrigin(sf::Vector2f(b.size.x / 2, b.size.y / 2));
    txt.setPosition(sf::Vector2f(cx, cy - 10));
    window.draw(txt);

    sf::Text sub(*font, botName + " is calculating...", 13);
    sub.setFillColor(sf::Color(160, 140, 120, 200));
    sf::FloatRect sb = sub.getLocalBounds();
    sub.setOrigin(sf::Vector2f(sb.size.x / 2, sb.size.y / 2));
    sub.setPosition(sf::Vector2f(cx, cy + 28));
    window.draw(sub);
}

void Game::drawPromotionDialog(sf::RenderWindow& window) {
    if (!assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    int cx = boardX + boardSize / 2;
    int cy = boardY + boardSize / 2;

    drawFilledRoundRect(window, cx - 170, cy - 50, 340, 100, 12,
                        sf::Color(35, 28, 22, 230));

    sf::RectangleShape promoBorder(sf::Vector2f(338, 98));
    promoBorder.setFillColor(sf::Color::Transparent);
    promoBorder.setOutlineColor(sf::Color(180, 150, 75, 80));
    promoBorder.setOutlineThickness(1);
    promoBorder.setPosition(sf::Vector2f(cx - 169, cy - 49));
    window.draw(promoBorder);

    sf::Text label(*font, "Promote pawn to:", 13);
    label.setFillColor(sf::Color(220, 200, 170, 220));
    label.setPosition(sf::Vector2f(cx - 80, cy - 42));
    window.draw(label);

    PieceType types[] = {QUEEN, ROOK, BISHOP, KNIGHT};
    for (int i = 0; i < 4; ++i) {
        int px = cx - 130 + i * 75;
        int py = cy - 5;
        const sf::Texture* tex = assetManager->getPieceTexture(types[i], currentPlayer);
        if (tex) {
            float s = 40.0f / tex->getSize().x;

            sf::Sprite shadow(*tex);
            shadow.setScale(sf::Vector2f(s, s));
            shadow.setPosition(sf::Vector2f(px + 1, py + 2));
            shadow.setColor(sf::Color(0, 0, 0, 50));
            window.draw(shadow);

            sf::Sprite sp(*tex);
            sp.setScale(sf::Vector2f(s, s));
            sp.setPosition(sf::Vector2f(px, py));
            window.draw(sp);
        }
        std::string keys = "QRBN";
        sf::Text key(*font, std::string(1, keys[i]), 9);
        key.setFillColor(sf::Color(160, 140, 100, 180));
        key.setPosition(sf::Vector2f(px + 16, py + 44));
        window.draw(key);
    }
}

void Game::drawGameOver(sf::RenderWindow& window) {
    if (!assetManager || !assetManager->hasFont()) return;
    sf::Font* font = assetManager->getFont();
    int cx = boardX + boardSize / 2;
    int cy = boardY + boardSize / 2;

    // Dim overlay
    sf::RectangleShape overlay(sf::Vector2f(boardSize, boardSize));
    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    overlay.setPosition(sf::Vector2f(boardX, boardY));
    window.draw(overlay);

    // Bigger result card
    drawFilledRoundRect(window, cx - 200, cy - 100, 400, 230, 16,
                        sf::Color(30, 24, 20, 240));

    sf::RectangleShape resultBorder(sf::Vector2f(398, 228));
    resultBorder.setFillColor(sf::Color::Transparent);
    resultBorder.setOutlineColor(sf::Color(212, 175, 55, 80));
    resultBorder.setOutlineThickness(1);
    resultBorder.setPosition(sf::Vector2f(cx - 199, cy - 99));
    window.draw(resultBorder);

    // ── Result title ──
    std::string resultText;
    sf::Color resultColor;
    if (result == RESULT_WHITE_WINS) {
        if (mode == MODE_VS_BOT)
            resultText = (playerColor == WHITE) ? "You Win!" : (botName + " Wins!");
        else
            resultText = "White Wins!";
        resultColor = sf::Color(220, 200, 170, 255);
    } else if (result == RESULT_BLACK_WINS) {
        if (mode == MODE_VS_BOT)
            resultText = (playerColor == BLACK) ? "You Win!" : (botName + " Wins!");
        else
            resultText = "Black Wins!";
        resultColor = sf::Color(180, 150, 200, 255);
    } else {
        resultText = "Draw";
        resultColor = sf::Color(200, 200, 200, 255);
    }

    sf::Text titleText(*font, resultText, 32);
    titleText.setFillColor(resultColor);
    sf::FloatRect tb = titleText.getLocalBounds();
    titleText.setOrigin(sf::Vector2f(tb.size.x / 2, tb.size.y / 2));
    titleText.setPosition(sf::Vector2f(cx, cy - 82));
    window.draw(titleText);

    // ── Subtext ──
    std::string subText;
    if (checkmate) subText = "by Checkmate";
    else if (stalemate) subText = "by Stalemate";
    else if (drawByFiftyMove) subText = "by 50-move rule";
    else if (drawByRepetition) subText = "by 3-fold repetition";
    else if (drawByInsufficientMaterial) subText = "by insufficient material";
    if (!subText.empty()) {
        sf::Text subT(*font, subText, 13);
        subT.setFillColor(sf::Color(200, 200, 200, 160));
        sf::FloatRect sb = subT.getLocalBounds();
        subT.setOrigin(sf::Vector2f(sb.size.x / 2, sb.size.y / 2));
        subT.setPosition(sf::Vector2f(cx, cy - 58));
        window.draw(subT);
    }

    // ── Stats line ──
    int elapsedSec = static_cast<int>(gameClock.getElapsedTime().asSeconds());
    int mins = elapsedSec / 60;
    int secs = elapsedSec % 60;
    std::string timeStr = std::to_string(mins) + "m " + std::to_string(secs) + "s";

    std::string statsStr = std::to_string(totalMoves) + " move" + (totalMoves == 1 ? "" : "s")
        + "  \u00b7  " + timeStr
        + "  \u00b7  " + std::to_string(capturesByWhite + capturesByBlack) + " capture" + ((capturesByWhite + capturesByBlack) == 1 ? "" : "s");

    sf::Text statsT(*font, statsStr, 12);
    statsT.setFillColor(sf::Color(160, 140, 100, 180));
    sf::FloatRect stb = statsT.getLocalBounds();
    statsT.setOrigin(sf::Vector2f(stb.size.x / 2, stb.size.y / 2));
    statsT.setPosition(sf::Vector2f(cx, cy - 34));
    window.draw(statsT);

    // ── Captures breakdown ──
    std::string capStr = "White: " + std::to_string(capturesByWhite) + "  |  Black: " + std::to_string(capturesByBlack);
    sf::Text capT(*font, capStr, 11);
    capT.setFillColor(sf::Color(140, 120, 80, 160));
    sf::FloatRect cb = capT.getLocalBounds();
    capT.setOrigin(sf::Vector2f(cb.size.x / 2, cb.size.y / 2));
    capT.setPosition(sf::Vector2f(cx, cy - 16));
    window.draw(capT);

    // ── Move history (last 6) ──
    int maxLines = 6;
    int start = std::max(0, (int)moveHistory.size() - maxLines);
    int lineCount = std::min(maxLines, (int)moveHistory.size());
    float histY = cy - 4;
    float lineH = 11.0f;
    for (int i = 0; i < lineCount; ++i) {
        int idx = start + i;
        std::string label = std::to_string(idx / 2 + 1) + (idx % 2 == 0 ? ". " : ".. ") + moveHistory[idx];
        sf::Text t(*font, label, 10);
        t.setFillColor(sf::Color(180, 160, 130, 200));
        sf::FloatRect lb = t.getLocalBounds();
        t.setOrigin(sf::Vector2f(lb.size.x / 2.0f, 0));
        t.setPosition(sf::Vector2f(cx, histY + i * lineH));
        window.draw(t);
    }

    float buttonAreaTop = histY + lineCount * lineH + 8;

    // ── Play Again button ──
    playAgainBtnBounds = sf::FloatRect({(float)(cx - 130), buttonAreaTop}, {260.0f, 34.0f});
    bool paHover = playAgainBtnBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));

    drawFilledRoundRect(window, playAgainBtnBounds.position.x, playAgainBtnBounds.position.y,
                        playAgainBtnBounds.size.x, playAgainBtnBounds.size.y, 10,
                        paHover ? sf::Color(70, 60, 50, 230) : sf::Color(55, 45, 35, 200));

    sf::RectangleShape paBorder(sf::Vector2f(258, 32));
    paBorder.setFillColor(sf::Color::Transparent);
    paBorder.setOutlineColor(paHover ? sf::Color(212, 175, 55, 180) : sf::Color(212, 175, 55, 80));
    paBorder.setOutlineThickness(1);
    paBorder.setPosition(sf::Vector2f(cx - 129, buttonAreaTop + 1));
    window.draw(paBorder);

    sf::Text paText(*font, "Play Again", 14);
    paText.setFillColor(paHover ? sf::Color(220, 200, 170, 255) : sf::Color(180, 160, 130, 220));
    sf::FloatRect ptb = paText.getLocalBounds();
    paText.setPosition(sf::Vector2f(
        playAgainBtnBounds.position.x + playAgainBtnBounds.size.x / 2.0f - ptb.size.x / 2.0f,
        playAgainBtnBounds.position.y + playAgainBtnBounds.size.y / 2.0f - ptb.size.y / 2.0f - 1
    ));
    window.draw(paText);

    // ── Review Game button ──
    float reviewBtnY = buttonAreaTop + 42;
    reviewBtnBounds = sf::FloatRect({(float)(cx - 130), reviewBtnY}, {260.0f, 30.0f});
    bool revHover = reviewBtnBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));

    drawFilledRoundRect(window, reviewBtnBounds.position.x, reviewBtnBounds.position.y,
                        reviewBtnBounds.size.x, reviewBtnBounds.size.y, 8,
                        revHover ? sf::Color(50, 70, 90, 240) : sf::Color(40, 55, 70, 180));

    sf::Text revText(*font, "Review Game", 12);
    revText.setFillColor(sf::Color(200, 220, 240, 230));
    sf::FloatRect rtb = revText.getLocalBounds();
    revText.setPosition(sf::Vector2f(
        reviewBtnBounds.position.x + reviewBtnBounds.size.x / 2.0f - rtb.size.x / 2.0f,
        reviewBtnBounds.position.y + reviewBtnBounds.size.y / 2.0f - rtb.size.y / 2.0f - 1
    ));
    window.draw(revText);

    // ── Exit button ──
    float exitBtnY = reviewBtnY + 38;
    exitBtnBounds = sf::FloatRect({(float)(cx - 130), exitBtnY}, {260.0f, 30.0f});
    bool exitHover = exitBtnBounds.contains(sf::Vector2f(sf::Mouse::getPosition(window)));

    drawFilledRoundRect(window, exitBtnBounds.position.x, exitBtnBounds.position.y,
                        exitBtnBounds.size.x, exitBtnBounds.size.y, 8,
                        exitHover ? sf::Color(180, 70, 70, 240) : sf::Color(140, 50, 50, 180));

    sf::Text exitText(*font, "Exit to Menu", 12);
    exitText.setFillColor(sf::Color(240, 220, 220, 230));
    sf::FloatRect etb = exitText.getLocalBounds();
    exitText.setPosition(sf::Vector2f(
        exitBtnBounds.position.x + exitBtnBounds.size.x / 2.0f - etb.size.x / 2.0f,
        exitBtnBounds.position.y + exitBtnBounds.size.y / 2.0f - etb.size.y / 2.0f - 1
    ));
    window.draw(exitText);
}
