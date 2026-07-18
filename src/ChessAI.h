#ifndef CHESSAI_H
#define CHESSAI_H

#include "ChessBoard.h"
#include <chrono>
#include <string>

enum AIDifficulty {
    EASY = 0,
    BOT_PAWN = 0,
    MEDIUM,
    BOT_KNIGHT,
    HARD,
    BOT_BISHOP,
    ULTRA_PRO,
    BOT_ROOK,
    BOT_QUEEN
};

struct Personality {
    int materialWeight = 100;
    int positionalWeight = 100;
    int developmentWeight = 50;
    int kingSafetyWeight = 50;
    int attackWeight = 50;
};

class ChessAI {
public:
    ChessAI(AIDifficulty difficulty);
    ChessAI();

    void setDifficulty(AIDifficulty difficulty);
    AIDifficulty getDifficulty() const { return difficulty; }
    const Personality& getPersonality() const { return personality; }

    void setDynamicParams(int depth, int blunderRate, int aggWeight, int kingWeight);
    bool isDynamic() const { return dynamicParams; }

    std::string getBotName() const;
    static std::string botNameForDifficulty(AIDifficulty diff);

    Move getBestMove(const ChessBoard& board);
    Move getBestMoveClean(const ChessBoard& board, int depth = 3);
    int evaluateClean(const ChessBoard& board) const;

private:
    AIDifficulty difficulty;
    Personality personality;
    bool dynamicParams = false;
    std::chrono::steady_clock::time_point searchStart;
    int maxSearchTimeMs;
    int nodesSearched;
    bool abortSearch;

    int searchDepth;
    int blunderChance;
    int aggressionWeight;
    int kingSafetyWeight;

    int getSearchDepth() const;
    int getBlunderChance() const;

    int evaluate(const ChessBoard& board) const;
    int evaluateMaterial(const ChessBoard& board) const;
    int evaluatePosition(const ChessBoard& board) const;
    int evaluateDevelopment(const ChessBoard& board) const;
    int evaluateKingSafety(const ChessBoard& board) const;
    int evaluatePawnStructure(const ChessBoard& board) const;

    int minimax(ChessBoard& board, int depth, int alpha, int beta, bool maximizing, PieceColor aiColor);
    int quiescence(ChessBoard& board, int alpha, int beta, bool maximizing, PieceColor aiColor);

    void orderMoves(ChessBoard& board, std::vector<Move>& moves, PieceColor currentColor);
    int moveScore(const ChessBoard& board, const Move& move) const;
    int getPieceValue(PieceType type) const;

    static const int PAWN_TABLE[64];
    static const int KNIGHT_TABLE[64];
    static const int BISHOP_TABLE[64];
    static const int ROOK_TABLE[64];
    static const int QUEEN_TABLE[64];
    static const int KING_TABLE[64];

    int getTableValue(PieceType type, int row, int col, PieceColor pieceColor) const;
    void iterativeDeepening(ChessBoard& board, int maxDepth, Move& bestMove, PieceColor aiColor);
};

#endif
