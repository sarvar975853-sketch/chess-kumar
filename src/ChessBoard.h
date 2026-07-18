#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <vector>
#include <string>

enum PieceType { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum PieceColor { WHITE, BLACK, NO_COLOR };

struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
    PieceType promotion;

    Move() : fromRow(0), fromCol(0), toRow(0), toCol(0), promotion(NONE) {}
    Move(int fr, int fc, int tr, int tc, PieceType p = NONE)
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), promotion(p) {}

    bool operator==(const Move& other) const {
        return fromRow == other.fromRow && fromCol == other.fromCol &&
               toRow == other.toRow && toCol == other.toCol &&
               promotion == other.promotion;
    }
};

struct BoardState {
    PieceType board[8][8];
    PieceColor colors[8][8];
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookKingsideMoved;
    bool whiteRookQueensideMoved;
    bool blackRookKingsideMoved;
    bool blackRookQueensideMoved;
    int enPassantRow;
    int enPassantCol;
    int halfMoveClock;
    PieceColor currentPlayer;
};

class ChessBoard {
public:
    ChessBoard();
    void init();

    PieceType getPiece(int row, int col) const;
    PieceColor getColor(int row, int col) const;
    bool isEmpty(int row, int col) const;

    BoardState makeMove(const Move& move);
    void unmakeMove(const Move& move, const BoardState& state);
    BoardState getState() const;

    std::vector<Move> generateLegalMoves() const;
    std::vector<Move> generatePseudoLegalMoves() const;

    bool isInCheck() const;
    bool isInCheck(PieceColor color) const;

    bool isCheckmate() const;
    bool isStalemate() const;

    bool isFiftyMoveDraw() const { return halfMoveClock >= 100; }
    bool isInsufficientMaterial() const;
    std::string getPositionKey() const;

    PieceColor getCurrentPlayer() const { return currentPlayer; }
    void switchPlayer();

    bool isLegalMove(const Move& move) const;

    void getEnPassant(int& row, int& col) const { row = enPassantRow; col = enPassantCol; }

    bool canCastleKingside(PieceColor color) const;
    bool canCastleQueenside(PieceColor color) const;

    bool isSquareAttacked(int row, int col, PieceColor byColor) const;

    std::string getFEN() const;

    static PieceType charToPiece(char c);
    static char pieceToChar(PieceType p);

private:
    PieceType board[8][8];
    PieceColor colors[8][8];

    PieceColor currentPlayer;

    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteRookKingsideMoved;
    bool whiteRookQueensideMoved;
    bool blackRookKingsideMoved;
    bool blackRookQueensideMoved;

    int enPassantRow;
    int enPassantCol;

    int halfMoveClock;

    void generatePawnMoves(int row, int col, std::vector<Move>& moves, bool pseudoLegal) const;
    void generateKnightMoves(int row, int col, std::vector<Move>& moves) const;
    void generateBishopMoves(int row, int col, std::vector<Move>& moves) const;
    void generateRookMoves(int row, int col, std::vector<Move>& moves) const;
    void generateQueenMoves(int row, int col, std::vector<Move>& moves) const;
    void generateKingMoves(int row, int col, std::vector<Move>& moves, bool pseudoLegal) const;

    bool isValidPosition(int row, int col) const;

    void generateSlidingMoves(int row, int col, const std::vector<std::pair<int,int>>& directions,
                              std::vector<Move>& moves) const;
};

#endif
