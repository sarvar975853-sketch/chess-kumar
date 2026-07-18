#include "ChessAI.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>

const int ChessAI::PAWN_TABLE[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0,
};

const int ChessAI::KNIGHT_TABLE[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

const int ChessAI::BISHOP_TABLE[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};

const int ChessAI::ROOK_TABLE[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0,
};

const int ChessAI::QUEEN_TABLE[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};

const int ChessAI::KING_TABLE[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20,
};

ChessAI::ChessAI(AIDifficulty diff) : difficulty(diff), dynamicParams(false), maxSearchTimeMs(10000), nodesSearched(0), abortSearch(false) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    searchDepth = getSearchDepth();
    blunderChance = getBlunderChance();
    aggressionWeight = 50;
    kingSafetyWeight = 50;
    personality = {100, 100, 50, 50, 50};
    switch (diff) {
        case BOT_PAWN:
            personality = {100, 30, 20, 0, 80};
            aggressionWeight = 80;
            kingSafetyWeight = 0;
            break;
        case BOT_KNIGHT:
            personality = {100, 60, 100, 40, 50};
            aggressionWeight = 50;
            kingSafetyWeight = 40;
            break;
        case BOT_BISHOP:
            personality = {100, 90, 60, 100, 30};
            aggressionWeight = 30;
            kingSafetyWeight = 100;
            break;
        case BOT_ROOK:
            personality = {100, 100, 80, 80, 60};
            aggressionWeight = 60;
            kingSafetyWeight = 80;
            break;
        case BOT_QUEEN:
            personality = {100, 100, 100, 100, 100};
            aggressionWeight = 100;
            kingSafetyWeight = 100;
            break;
        default:
            break;
    }
}

ChessAI::ChessAI() : ChessAI(BOT_PAWN) {}

void ChessAI::setDifficulty(AIDifficulty diff) {
    difficulty = diff;
    dynamicParams = false;
    searchDepth = getSearchDepth();
    blunderChance = getBlunderChance();
}

void ChessAI::setDynamicParams(int depth, int blunderRate, int aggWeight, int kingWeight) {
    dynamicParams = true;
    searchDepth = std::max(1, std::min(12, depth));
    blunderChance = std::max(0, std::min(50, blunderRate));
    aggressionWeight = std::max(0, std::min(100, aggWeight));
    kingSafetyWeight = std::max(0, std::min(100, kingWeight));
    personality.attackWeight = aggressionWeight;
    personality.kingSafetyWeight = kingSafetyWeight;
}

std::string ChessAI::getBotName() const {
    return botNameForDifficulty(difficulty);
}

std::string ChessAI::botNameForDifficulty(AIDifficulty diff) {
    switch (diff) {
        case BOT_PAWN:   return "Pauly";
        case BOT_KNIGHT: return "Knightly";
        case BOT_BISHOP: return "Bishop";
        case BOT_ROOK:   return "Rook";
        case BOT_QUEEN:  return "Queen";
        default:         return "Bot";
    }
}

int ChessAI::getSearchDepth() const {
    if (dynamicParams) return searchDepth;
    switch (difficulty) {
        case BOT_PAWN:   return 2;
        case BOT_KNIGHT: return 3;
        case BOT_BISHOP: return 4;
        case BOT_ROOK:   return 6;
        case BOT_QUEEN:  return 12;
        default:         return 3;
    }
}

int ChessAI::getBlunderChance() const {
    if (dynamicParams) return blunderChance;
    switch (difficulty) {
        case BOT_PAWN:   return 25;
        case BOT_KNIGHT: return 12;
        case BOT_BISHOP: return 5;
        case BOT_ROOK:   return 2;
        case BOT_QUEEN:  return 0;
        default:         return 10;
    }
}

int ChessAI::getPieceValue(PieceType type) const {
    switch (type) {
        case PAWN:   return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK:   return 500;
        case QUEEN:  return 900;
        case KING:   return 20000;
        default:     return 0;
    }
}

int ChessAI::getTableValue(PieceType type, int row, int col, PieceColor pieceColor) const {
    int idx;
    if (pieceColor == WHITE) {
        idx = row * 8 + col;
    } else {
        idx = (7 - row) * 8 + col;
    }
    if (idx < 0 || idx >= 64) return 0;
    switch (type) {
        case PAWN:   return PAWN_TABLE[idx];
        case KNIGHT: return KNIGHT_TABLE[idx];
        case BISHOP: return BISHOP_TABLE[idx];
        case ROOK:   return ROOK_TABLE[idx];
        case QUEEN:  return QUEEN_TABLE[idx];
        case KING:   return KING_TABLE[idx];
        default:     return 0;
    }
}

int ChessAI::evaluateMaterial(const ChessBoard& board) const {
    int score = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            PieceType p = board.getPiece(r, c);
            if (p == NONE) continue;
            int v = getPieceValue(p);
            score += (board.getColor(r, c) == WHITE) ? v : -v;
        }
    return score;
}

int ChessAI::evaluatePosition(const ChessBoard& board) const {
    int score = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            PieceType p = board.getPiece(r, c);
            if (p == NONE) continue;
            PieceColor col = board.getColor(r, c);
            score += (col == WHITE) ? getTableValue(p, r, c, col) : -getTableValue(p, r, c, col);
        }
    return score;
}

int ChessAI::evaluate(const ChessBoard& board) const {
    int material = evaluateMaterial(board);
    int positional = evaluatePosition(board);
    int development = evaluateDevelopment(board);
    int kingSafety = evaluateKingSafety(board);
    int pawnStruct = evaluatePawnStructure(board);

    int score = 0;
    score += material * personality.materialWeight / 100;
    score += positional * personality.positionalWeight / 100;
    score += development * personality.developmentWeight / 100;
    score += kingSafety * personality.kingSafetyWeight / 100;
    score += pawnStruct * (personality.positionalWeight / 2) / 100;

    int blunder = getBlunderChance();
    if (blunder > 0) {
        score += (std::rand() % (blunder * 10)) - (blunder * 5);
    }
    return score;
}

int ChessAI::evaluateDevelopment(const ChessBoard& board) const {
    int score = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            PieceType pt = board.getPiece(r, c);
            if (pt == NONE) continue;
            PieceColor col = board.getColor(r, c);
            int sign = (col == WHITE) ? 1 : -1;

            // Bonus for knights and bishops off their starting squares
            if (pt == KNIGHT || pt == BISHOP) {
                if (col == WHITE && r < 6) score += 15 * sign;
                else if (col == BLACK && r > 1) score += 15 * sign;
                // Bonus for central squares
                if (c >= 2 && c <= 5 && r >= 2 && r <= 5) score += 10 * sign;
            }

            // Bonus for controlling center
            if (pt != PAWN && pt != KING) {
                if (c >= 2 && c <= 5 && r >= 2 && r <= 5) score += 8 * sign;
            }

            // Bonus for castled king (rook moved indicates likely castled)
            if (pt == KING) {
                if (col == WHITE && r == 7 && (c == 6 || c == 2)) score += 20 * sign;
                else if (col == BLACK && r == 0 && (c == 6 || c == 2)) score += 20 * sign;
            }
        }
    }
    // Penalty for unmoved pieces
    if (board.getPiece(7, 1) == KNIGHT && board.getColor(7, 1) == WHITE) score -= 10;
    if (board.getPiece(7, 6) == KNIGHT && board.getColor(7, 6) == WHITE) score -= 10;
    if (board.getPiece(0, 1) == KNIGHT && board.getColor(0, 1) == BLACK) score += 10;
    if (board.getPiece(0, 6) == KNIGHT && board.getColor(0, 6) == BLACK) score += 10;
    return score;
}

int ChessAI::evaluateKingSafety(const ChessBoard& board) const {
    int score = 0;
    auto evalKing = [&](PieceColor color, int sign) {
        // Find king
        int kr = -1, kc = -1;
        for (int r = 0; r < 8 && kr < 0; ++r)
            for (int c = 0; c < 8 && kc < 0; ++c)
                if (board.getPiece(r, c) == KING && board.getColor(r, c) == color)
                    { kr = r; kc = c; }
        if (kr < 0) return;

        // Bonus for pawn shield in front of king
        int pawnDir = (color == WHITE) ? 1 : -1;
        for (int dc = -1; dc <= 1; ++dc) {
            int pr = kr + pawnDir;
            int pc = kc + dc;
            if (pr >= 0 && pr < 8 && pc >= 0 && pc < 8)
                if (board.getPiece(pr, pc) == PAWN && board.getColor(pr, pc) == color)
                    score += 20 * sign;
        }

        // Penalty for open files near king
        for (int dc = -1; dc <= 1; ++dc) {
            int pc = kc + dc;
            if (pc < 0 || pc >= 8) continue;
            bool hasPawn = false;
            for (int r = 0; r < 8; ++r)
                if (board.getPiece(r, pc) == PAWN && board.getColor(r, pc) == color)
                    { hasPawn = true; break; }
            if (!hasPawn) score -= 15 * sign;
        }

        // King on back rank is safer
        int backRank = (color == WHITE) ? 7 : 0;
        if (kr == backRank) score += 10 * sign;
        else score -= 15 * sign;
    };

    evalKing(WHITE, 1);
    evalKing(BLACK, -1);
    return score;
}

int ChessAI::evaluatePawnStructure(const ChessBoard& board) const {
    int score = 0;
    for (PieceColor col : {WHITE, BLACK}) {
        int sign = (col == WHITE) ? 1 : -1;
        std::vector<int> pawnFiles;
        std::vector<int> pawnRows;
        for (int r = 0; r < 8; ++r)
            for (int c = 0; c < 8; ++c)
                if (board.getPiece(r, c) == PAWN && board.getColor(r, c) == col) {
                    pawnFiles.push_back(c);
                    pawnRows.push_back(r);
                }

        // Doubled pawn penalty
        for (size_t i = 0; i < pawnFiles.size(); ++i)
            for (size_t j = i + 1; j < pawnFiles.size(); ++j)
                if (pawnFiles[i] == pawnFiles[j])
                    score -= 15 * sign;

        // Isolated pawn penalty
        for (int f : pawnFiles) {
            bool left = false, right = false;
            for (int of : pawnFiles) {
                if (of == f - 1) left = true;
                if (of == f + 1) right = true;
            }
            if (!left && !right) score -= 20 * sign;
        }

        // Connected pawn bonus
        for (size_t i = 0; i < pawnFiles.size(); ++i)
            for (size_t j = i + 1; j < pawnFiles.size(); ++j)
                if (std::abs(pawnFiles[i] - pawnFiles[j]) == 1)
                    score += 5 * sign;
    }
    return score;
}

int ChessAI::moveScore(const ChessBoard& board, const Move& move) const {
    int score = 0;
    PieceType captured = board.getPiece(move.toRow, move.toCol);
    if (captured != NONE)
        score += 10 * getPieceValue(captured) - getPieceValue(board.getPiece(move.fromRow, move.fromCol));
    if (move.promotion != NONE)
        score += getPieceValue(move.promotion) - 100;

    // Aggression bonus: prefer checks, forward pawn pushes, and central control
    if (aggressionWeight > 50) {
        PieceType pt = board.getPiece(move.fromRow, move.fromCol);
        // Forward pawn push
        if (pt == PAWN) {
            PieceColor col = board.getColor(move.fromRow, move.fromCol);
            int forward = (col == WHITE) ? -1 : 1;
            if ((move.toRow - move.fromRow) == forward && move.toCol == move.fromCol)
                score += 5 * (aggressionWeight - 50) / 50;
        }
        // Central moves
        if (move.toCol >= 2 && move.toCol <= 5 && move.toRow >= 2 && move.toRow <= 5)
            score += 3 * (aggressionWeight - 50) / 50;
    }

    return score;
}

void ChessAI::orderMoves(ChessBoard& board, std::vector<Move>& moves, PieceColor currentColor) {
    std::vector<std::pair<int,int>> scores;
    scores.reserve(moves.size());
    for (size_t i = 0; i < moves.size(); ++i)
        scores.emplace_back(moveScore(board, moves[i]), i);
    std::sort(scores.begin(), scores.end(), [](auto& a, auto& b) { return a.first > b.first; });
    std::vector<Move> ordered;
    ordered.reserve(moves.size());
    for (auto& s : scores) ordered.push_back(moves[s.second]);
    moves = std::move(ordered);
}

int ChessAI::quiescence(ChessBoard& board, int alpha, int beta, bool maximizing, PieceColor aiColor) {
    nodesSearched++;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStart).count();
    if (elapsed > maxSearchTimeMs) { abortSearch = true; return 0; }

    int standPat = evaluate(board);
    if (!maximizing) standPat = -standPat;
    if (maximizing) { if (standPat >= beta) return beta; if (standPat > alpha) alpha = standPat; }
    else { if (standPat <= alpha) return alpha; if (standPat < beta) beta = standPat; }

    std::vector<Move> moves = board.generateLegalMoves();
    std::vector<Move> captures;
    for (auto& m : moves)
        if (board.getPiece(m.toRow, m.toCol) != NONE || m.promotion != NONE)
            captures.push_back(m);

    if (captures.empty()) return standPat;
    orderMoves(board, captures, maximizing ? aiColor : (aiColor == WHITE ? BLACK : WHITE));

    for (auto& move : captures) {
        if (abortSearch) return 0;
        BoardState state = board.makeMove(move);
        int score = quiescence(board, alpha, beta, !maximizing, aiColor);
        board.unmakeMove(move, state);
        if (abortSearch) return 0;
        if (maximizing) { if (score > alpha) alpha = score; if (alpha >= beta) return beta; }
        else { if (score < beta) beta = score; if (beta <= alpha) return alpha; }
    }
    return maximizing ? alpha : beta;
}

int ChessAI::minimax(ChessBoard& board, int depth, int alpha, int beta, bool maximizing, PieceColor aiColor) {
    nodesSearched++;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStart).count();
    if (elapsed > maxSearchTimeMs) { abortSearch = true; return 0; }

    if (depth == 0) {
        if (difficulty >= BOT_ROOK) return quiescence(board, alpha, beta, maximizing, aiColor);
        return evaluate(board);
    }

    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) {
        if (board.isInCheck())
            return maximizing ? -100000 + depth : 100000 - depth;
        return 0;
    }

    orderMoves(board, moves, board.getCurrentPlayer());

    for (auto& move : moves) {
        if (abortSearch) return 0;
        BoardState state = board.makeMove(move);
        int score = minimax(board, depth - 1, alpha, beta, !maximizing, aiColor);
        board.unmakeMove(move, state);
        if (abortSearch) return 0;
        if (maximizing) { if (score > alpha) alpha = score; if (alpha >= beta) return beta; }
        else { if (score < beta) beta = score; if (beta <= alpha) return alpha; }
    }
    return maximizing ? alpha : beta;
}

void ChessAI::iterativeDeepening(ChessBoard& board, int maxDepth, Move& bestMove, PieceColor aiColor) {
    Move currentBest;
    bool foundMove = false;
    bool maximizer = (aiColor == WHITE);

    for (int d = 1; d <= maxDepth; ++d) {
        if (abortSearch) break;
        Move depthBest;
        bool depthFound = false;
        int alpha = -1000000, beta = 1000000;
        std::vector<Move> moves = board.generateLegalMoves();
        if (moves.empty()) break;
        orderMoves(board, moves, aiColor);
        for (auto& move : moves) {
            if (abortSearch) break;
            BoardState state = board.makeMove(move);
            int score = minimax(board, d - 1, alpha, beta, !maximizer, aiColor);
            board.unmakeMove(move, state);
            if (abortSearch) break;
            if (maximizer ? (score > alpha) : (score < beta)) {
                if (maximizer) alpha = score; else beta = score;
                depthBest = move;
                depthFound = true;
            }
        }
        if (depthFound && !abortSearch) { currentBest = depthBest; foundMove = true; }
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - searchStart).count();
        if (elapsed > maxSearchTimeMs / 2) break;
    }
    if (foundMove) bestMove = currentBest;
}

int ChessAI::evaluateClean(const ChessBoard& board) const {
    int material = evaluateMaterial(board);
    int positional = evaluatePosition(board);
    int development = evaluateDevelopment(board);
    int kingSafety = evaluateKingSafety(board);
    int pawnStruct = evaluatePawnStructure(board);

    int score = 0;
    score += material * personality.materialWeight / 100;
    score += positional * personality.positionalWeight / 100;
    score += development * personality.developmentWeight / 100;
    score += kingSafety * personality.kingSafetyWeight / 100;
    score += pawnStruct * (personality.positionalWeight / 2) / 100;
    return score;
}

Move ChessAI::getBestMoveClean(const ChessBoard& board, int depth) {
    PieceColor aiColor = board.getCurrentPlayer();
    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) return Move();

    bool maximizer = (aiColor == WHITE);
    Move bestMove = moves[0];
    int bestScore = maximizer ? -1000000 : 1000000;

    orderMoves(const_cast<ChessBoard&>(board), moves, aiColor);

    for (auto& move : moves) {
        ChessBoard& brd = const_cast<ChessBoard&>(board);
        BoardState state = brd.makeMove(move);
        int score = minimax(brd, depth - 1, -1000000, 1000000, !maximizer, aiColor);
        brd.unmakeMove(move, state);

        if (maximizer ? (score > bestScore) : (score < bestScore)) {
            bestScore = score;
            bestMove = move;
        }
    }
    return bestMove;
}

Move ChessAI::getBestMove(const ChessBoard& board) {
    nodesSearched = 0;
    abortSearch = false;
    searchStart = std::chrono::steady_clock::now();

    PieceColor aiColor = board.getCurrentPlayer();
    std::vector<Move> moves = board.generateLegalMoves();
    if (moves.empty()) return Move();

    if (dynamicParams) {
        maxSearchTimeMs = 500 + searchDepth * 250;
    } else {
        switch (difficulty) {
            case BOT_PAWN:   maxSearchTimeMs = 500; break;
            case BOT_KNIGHT: maxSearchTimeMs = 1000; break;
            case BOT_BISHOP: maxSearchTimeMs = 2000; break;
            case BOT_ROOK:   maxSearchTimeMs = 3000; break;
            case BOT_QUEEN:  maxSearchTimeMs = 2500; break;
        }
    }

    if (difficulty == BOT_QUEEN || (dynamicParams && searchDepth >= 8)) {
        Move bestMove = moves[0];
        iterativeDeepening(const_cast<ChessBoard&>(board), 30, bestMove, aiColor);
        return bestMove;
    }

    int depth = getSearchDepth();
    bool maximizer = (aiColor == WHITE);
    Move bestMove = moves[0];
    int bestScore = maximizer ? -1000000 : 1000000;

    orderMoves(const_cast<ChessBoard&>(board), moves, aiColor);

    for (auto& move : moves) {
        if (abortSearch) break;
        ChessBoard& brd = const_cast<ChessBoard&>(board);
        BoardState state = brd.makeMove(move);
        int score = minimax(brd, depth - 1, -1000000, 1000000, !maximizer, aiColor);
        brd.unmakeMove(move, state);
        if (abortSearch) break;

        int blunder = getBlunderChance();
        if (blunder > 0) score += (std::rand() % (blunder * 8)) - (blunder * 4);

        if (maximizer ? (score > bestScore) : (score < bestScore)) {
            bestScore = score;
            bestMove = move;
        }
    }
    return bestMove;
}
