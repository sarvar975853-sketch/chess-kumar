#include "ChessBoard.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <sstream>

ChessBoard::ChessBoard() {
    init();
}

void ChessBoard::init() {
    std::memset(board, 0, sizeof(board));
    std::memset(colors, 0, sizeof(colors));

    PieceType backRank[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};

    for (int col = 0; col < 8; ++col) {
        board[0][col] = backRank[col];
        colors[0][col] = BLACK;
        board[1][col] = PAWN;
        colors[1][col] = BLACK;
        board[6][col] = PAWN;
        colors[6][col] = WHITE;
        board[7][col] = backRank[col];
        colors[7][col] = WHITE;
    }

    currentPlayer = WHITE;

    whiteKingMoved = false;
    blackKingMoved = false;
    whiteRookKingsideMoved = false;
    whiteRookQueensideMoved = false;
    blackRookKingsideMoved = false;
    blackRookQueensideMoved = false;

    enPassantRow = -1;
    enPassantCol = -1;
    halfMoveClock = 0;
}

PieceType ChessBoard::getPiece(int row, int col) const {
    if (!isValidPosition(row, col)) return NONE;
    return board[row][col];
}

PieceColor ChessBoard::getColor(int row, int col) const {
    if (!isValidPosition(row, col)) return NO_COLOR;
    return colors[row][col];
}

bool ChessBoard::isEmpty(int row, int col) const {
    return isValidPosition(row, col) && board[row][col] == NONE;
}

bool ChessBoard::isValidPosition(int row, int col) const {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

void ChessBoard::switchPlayer() {
    currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
}

bool ChessBoard::canCastleKingside(PieceColor color) const {
    if (color == WHITE) return !whiteKingMoved && !whiteRookKingsideMoved;
    return !blackKingMoved && !blackRookKingsideMoved;
}

bool ChessBoard::canCastleQueenside(PieceColor color) const {
    if (color == WHITE) return !whiteKingMoved && !whiteRookQueensideMoved;
    return !blackKingMoved && !blackRookQueensideMoved;
}

bool ChessBoard::isSquareAttacked(int row, int col, PieceColor byColor) const {
    // Check pawn attacks
    int pawnDir = (byColor == WHITE) ? 1 : -1;
    if (isValidPosition(row + pawnDir, col - 1) &&
        board[row + pawnDir][col - 1] == PAWN && colors[row + pawnDir][col - 1] == byColor)
        return true;
    if (isValidPosition(row + pawnDir, col + 1) &&
        board[row + pawnDir][col + 1] == PAWN && colors[row + pawnDir][col + 1] == byColor)
        return true;

    // Check knight attacks
    const int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (int i = 0; i < 8; ++i) {
        int r = row + knightMoves[i][0];
        int c = col + knightMoves[i][1];
        if (isValidPosition(r, c) && board[r][c] == KNIGHT && colors[r][c] == byColor)
            return true;
    }

    // Check king attacks
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr;
            int c = col + dc;
            if (isValidPosition(r, c) && board[r][c] == KING && colors[r][c] == byColor)
                return true;
        }
    }

    // Check sliding pieces (bishop, rook, queen)
    const std::vector<std::pair<int,int>> bishopDirs = {{-1,-1},{-1,1},{1,-1},{1,1}};
    const std::vector<std::pair<int,int>> rookDirs = {{-1,0},{1,0},{0,-1},{0,1}};

    for (const auto& dir : bishopDirs) {
        int r = row + dir.first;
        int c = col + dir.second;
        while (isValidPosition(r, c)) {
            if (board[r][c] != NONE) {
                if (colors[r][c] == byColor &&
                    (board[r][c] == BISHOP || board[r][c] == QUEEN))
                    return true;
                break;
            }
            r += dir.first;
            c += dir.second;
        }
    }

    for (const auto& dir : rookDirs) {
        int r = row + dir.first;
        int c = col + dir.second;
        while (isValidPosition(r, c)) {
            if (board[r][c] != NONE) {
                if (colors[r][c] == byColor &&
                    (board[r][c] == ROOK || board[r][c] == QUEEN))
                    return true;
                break;
            }
            r += dir.first;
            c += dir.second;
        }
    }

    return false;
}

bool ChessBoard::isInCheck() const {
    return isInCheck(currentPlayer);
}

bool ChessBoard::isInCheck(PieceColor color) const {
    // Find king position
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (board[row][col] == KING && colors[row][col] == color) {
                PieceColor enemy = (color == WHITE) ? BLACK : WHITE;
                return isSquareAttacked(row, col, enemy);
            }
        }
    }
    return false;
}

void ChessBoard::generateSlidingMoves(int row, int col,
                                       const std::vector<std::pair<int,int>>& directions,
                                       std::vector<Move>& moves) const {
    PieceColor pieceColor = colors[row][col];
    for (const auto& dir : directions) {
        int r = row + dir.first;
        int c = col + dir.second;
        while (isValidPosition(r, c)) {
            if (board[r][c] == NONE) {
                moves.emplace_back(row, col, r, c);
            } else {
                if (colors[r][c] != pieceColor) {
                    moves.emplace_back(row, col, r, c);
                }
                break;
            }
            r += dir.first;
            c += dir.second;
        }
    }
}

void ChessBoard::generatePawnMoves(int row, int col, std::vector<Move>& moves, bool pseudoLegal) const {
    PieceColor pieceColor = colors[row][col];
    int dir = (pieceColor == WHITE) ? -1 : 1;
    int startRow = (pieceColor == WHITE) ? 6 : 1;
    int promoRow = (pieceColor == WHITE) ? 0 : 7;

    // Forward one
    int nr = row + dir;
    if (isValidPosition(nr, col) && board[nr][col] == NONE) {
        if (nr == promoRow) {
            moves.emplace_back(row, col, nr, col, QUEEN);
            moves.emplace_back(row, col, nr, col, ROOK);
            moves.emplace_back(row, col, nr, col, BISHOP);
            moves.emplace_back(row, col, nr, col, KNIGHT);
        } else {
            moves.emplace_back(row, col, nr, col);
        }

        // Forward two from start
        int nr2 = row + 2 * dir;
        if (row == startRow && board[nr2][col] == NONE) {
            moves.emplace_back(row, col, nr2, col);
        }
    }

    // Captures
    for (int dc : {-1, 1}) {
        int nc = col + dc;
        if (!isValidPosition(nr, nc)) continue;

        // Normal capture
        if (board[nr][nc] != NONE && colors[nr][nc] != pieceColor) {
            if (nr == promoRow) {
                moves.emplace_back(row, col, nr, nc, QUEEN);
                moves.emplace_back(row, col, nr, nc, ROOK);
                moves.emplace_back(row, col, nr, nc, BISHOP);
                moves.emplace_back(row, col, nr, nc, KNIGHT);
            } else {
                moves.emplace_back(row, col, nr, nc);
            }
        }

        // En passant
        if (nr == enPassantRow && nc == enPassantCol) {
            moves.emplace_back(row, col, nr, nc);
        }
    }
}

void ChessBoard::generateKnightMoves(int row, int col, std::vector<Move>& moves) const {
    PieceColor pieceColor = colors[row][col];
    const int offsets[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (int i = 0; i < 8; ++i) {
        int r = row + offsets[i][0];
        int c = col + offsets[i][1];
        if (isValidPosition(r, c) && (board[r][c] == NONE || colors[r][c] != pieceColor)) {
            moves.emplace_back(row, col, r, c);
        }
    }
}

void ChessBoard::generateBishopMoves(int row, int col, std::vector<Move>& moves) const {
    generateSlidingMoves(row, col, {{-1,-1},{-1,1},{1,-1},{1,1}}, moves);
}

void ChessBoard::generateRookMoves(int row, int col, std::vector<Move>& moves) const {
    generateSlidingMoves(row, col, {{-1,0},{1,0},{0,-1},{0,1}}, moves);
}

void ChessBoard::generateQueenMoves(int row, int col, std::vector<Move>& moves) const {
    generateSlidingMoves(row, col, {{-1,-1},{-1,1},{1,-1},{1,1},{-1,0},{1,0},{0,-1},{0,1}}, moves);
}

void ChessBoard::generateKingMoves(int row, int col, std::vector<Move>& moves, bool pseudoLegal) const {
    PieceColor pieceColor = colors[row][col];
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int r = row + dr;
            int c = col + dc;
            if (isValidPosition(r, c) && (board[r][c] == NONE || colors[r][c] != pieceColor)) {
                moves.emplace_back(row, col, r, c);
            }
        }
    }

    // Castling (only if generating legal moves, we check conditions)
    if (pseudoLegal) return;

    if (isInCheck(pieceColor)) return;

    PieceColor enemy = (pieceColor == WHITE) ? BLACK : WHITE;

    // Kingside
    if (canCastleKingside(pieceColor)) {
        int rookCol = 7;
        if (board[row][5] == NONE && board[row][6] == NONE &&
            !isSquareAttacked(row, 5, enemy) && !isSquareAttacked(row, 6, enemy)) {
            moves.emplace_back(row, col, row, 6);
        }
    }

    // Queenside
    if (canCastleQueenside(pieceColor)) {
        int rookCol = 0;
        if (board[row][3] == NONE && board[row][2] == NONE && board[row][1] == NONE &&
            !isSquareAttacked(row, 3, enemy) && !isSquareAttacked(row, 2, enemy)) {
            moves.emplace_back(row, col, row, 2);
        }
    }
}

std::vector<Move> ChessBoard::generatePseudoLegalMoves() const {
    std::vector<Move> moves;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (board[row][col] == NONE || colors[row][col] != currentPlayer) continue;

            switch (board[row][col]) {
                case PAWN:   generatePawnMoves(row, col, moves, true); break;
                case KNIGHT: generateKnightMoves(row, col, moves); break;
                case BISHOP: generateBishopMoves(row, col, moves); break;
                case ROOK:   generateRookMoves(row, col, moves); break;
                case QUEEN:  generateQueenMoves(row, col, moves); break;
                case KING:   generateKingMoves(row, col, moves, true); break;
                default: break;
            }
        }
    }
    return moves;
}

std::vector<Move> ChessBoard::generateLegalMoves() const {
    std::vector<Move> pseudoMoves = generatePseudoLegalMoves();
    std::vector<Move> legalMoves;
    legalMoves.reserve(pseudoMoves.size());

    for (const Move& move : pseudoMoves) {
        ChessBoard& self = const_cast<ChessBoard&>(*this);
        BoardState state = self.makeMove(move);
        PieceColor movedColor = static_cast<PieceColor>(!currentPlayer);
        PieceColor oppColor = currentPlayer;
        bool oppKingExists = false;
        for (int r = 0; r < 8 && !oppKingExists; ++r)
            for (int c = 0; c < 8 && !oppKingExists; ++c)
                if (self.board[r][c] == KING && self.colors[r][c] == oppColor)
                    oppKingExists = true;
        if (oppKingExists && !self.isInCheck(movedColor)) {
            legalMoves.push_back(move);
        }
        self.unmakeMove(move, state);
    }

    // Generate castling moves separately (not included in pseudo-legal)
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (board[row][col] == KING && colors[row][col] == currentPlayer) {
                std::vector<Move> kingMoves;
                generateKingMoves(row, col, kingMoves, false);
                for (const Move& km : kingMoves) {
                    if (std::abs(km.toCol - km.fromCol) == 2) {
                        // Castling move - check if already added
                        bool found = false;
                        for (const Move& lm : legalMoves) {
                            if (lm.fromRow == km.fromRow && lm.fromCol == km.fromCol &&
                                lm.toRow == km.toRow && lm.toCol == km.toCol) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            ChessBoard& self2 = const_cast<ChessBoard&>(*this);
                            BoardState cs = self2.makeMove(km);
                            if (!self2.isInCheck(static_cast<PieceColor>(!currentPlayer)))
                                legalMoves.push_back(km);
                            self2.unmakeMove(km, cs);
                        }
                    }
                }
            }
        }
    }

    return legalMoves;
}

bool ChessBoard::isLegalMove(const Move& move) const {
    std::vector<Move> legal = generateLegalMoves();
    for (const Move& lm : legal) {
        if (lm.fromRow == move.fromRow && lm.fromCol == move.fromCol &&
            lm.toRow == move.toRow && lm.toCol == move.toCol) {
            if (lm.promotion == move.promotion || move.promotion == NONE) {
                return true;
            }
        }
    }
    return false;
}

BoardState ChessBoard::makeMove(const Move& move) {
    BoardState state;
    std::memcpy(state.board, board, sizeof(board));
    std::memcpy(state.colors, colors, sizeof(colors));
    state.whiteKingMoved = whiteKingMoved;
    state.blackKingMoved = blackKingMoved;
    state.whiteRookKingsideMoved = whiteRookKingsideMoved;
    state.whiteRookQueensideMoved = whiteRookQueensideMoved;
    state.blackRookKingsideMoved = blackRookKingsideMoved;
    state.blackRookQueensideMoved = blackRookQueensideMoved;
    state.enPassantRow = enPassantRow;
    state.enPassantCol = enPassantCol;
    state.halfMoveClock = halfMoveClock;
    state.currentPlayer = currentPlayer;

    PieceType piece = board[move.fromRow][move.fromCol];
    PieceColor pieceColor = colors[move.fromRow][move.fromCol];

    // Reset en passant
    enPassantRow = -1;
    enPassantCol = -1;

    // Check for en passant capture
    if (piece == PAWN && move.toCol != move.fromCol &&
        board[move.toRow][move.toCol] == NONE) {
        // En passant capture - remove the captured pawn
        board[move.fromRow][move.toCol] = NONE;
        colors[move.fromRow][move.toCol] = NO_COLOR;
    }

    // Check for castling
    if (piece == KING && std::abs(move.toCol - move.fromCol) == 2) {
        if (move.toCol > move.fromCol) {
            // Kingside
            board[move.toRow][5] = board[move.toRow][7];
            colors[move.toRow][5] = colors[move.toRow][7];
            board[move.toRow][7] = NONE;
            colors[move.toRow][7] = NO_COLOR;
        } else {
            // Queenside
            board[move.toRow][3] = board[move.toRow][0];
            colors[move.toRow][3] = colors[move.toRow][0];
            board[move.toRow][0] = NONE;
            colors[move.toRow][0] = NO_COLOR;
        }
    }

    // Set en passant target for double pawn push
    if (piece == PAWN && std::abs(move.toRow - move.fromRow) == 2) {
        enPassantRow = (move.fromRow + move.toRow) / 2;
        enPassantCol = move.fromCol;
    }

    // Update castling rights
    if (piece == KING) {
        if (pieceColor == WHITE) whiteKingMoved = true;
        else blackKingMoved = true;
    }
    if (piece == ROOK) {
        if (move.fromRow == 7 && move.fromCol == 7) whiteRookKingsideMoved = true;
        if (move.fromRow == 7 && move.fromCol == 0) whiteRookQueensideMoved = true;
        if (move.fromRow == 0 && move.fromCol == 7) blackRookKingsideMoved = true;
        if (move.fromRow == 0 && move.fromCol == 0) blackRookQueensideMoved = true;
    }
    // Also update if a rook is captured
    if (move.toRow == 7 && move.toCol == 7) whiteRookKingsideMoved = true;
    if (move.toRow == 7 && move.toCol == 0) whiteRookQueensideMoved = true;
    if (move.toRow == 0 && move.toCol == 7) blackRookKingsideMoved = true;
    if (move.toRow == 0 && move.toCol == 0) blackRookQueensideMoved = true;

    // Move piece
    board[move.toRow][move.toCol] = piece;
    colors[move.toRow][move.toCol] = pieceColor;
    board[move.fromRow][move.fromCol] = NONE;
    colors[move.fromRow][move.fromCol] = NO_COLOR;

    // Handle promotion
    if (move.promotion != NONE && piece == PAWN) {
        board[move.toRow][move.toCol] = move.promotion;
    }

    // Half move clock
    if (piece == PAWN || board[move.toRow][move.toCol] != NONE) {
        halfMoveClock = 0;
    } else {
        halfMoveClock++;
    }

    // Switch player
    currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;

    return state;
}

void ChessBoard::unmakeMove(const Move& move, const BoardState& state) {
    std::memcpy(board, state.board, sizeof(board));
    std::memcpy(colors, state.colors, sizeof(colors));
    whiteKingMoved = state.whiteKingMoved;
    blackKingMoved = state.blackKingMoved;
    whiteRookKingsideMoved = state.whiteRookKingsideMoved;
    whiteRookQueensideMoved = state.whiteRookQueensideMoved;
    blackRookKingsideMoved = state.blackRookKingsideMoved;
    blackRookQueensideMoved = state.blackRookQueensideMoved;
    enPassantRow = state.enPassantRow;
    enPassantCol = state.enPassantCol;
    halfMoveClock = state.halfMoveClock;
    currentPlayer = state.currentPlayer;
}

BoardState ChessBoard::getState() const {
    BoardState state;
    std::memcpy(state.board, board, sizeof(board));
    std::memcpy(state.colors, colors, sizeof(colors));
    state.whiteKingMoved = whiteKingMoved;
    state.blackKingMoved = blackKingMoved;
    state.whiteRookKingsideMoved = whiteRookKingsideMoved;
    state.whiteRookQueensideMoved = whiteRookQueensideMoved;
    state.blackRookKingsideMoved = blackRookKingsideMoved;
    state.blackRookQueensideMoved = blackRookQueensideMoved;
    state.enPassantRow = enPassantRow;
    state.enPassantCol = enPassantCol;
    state.halfMoveClock = halfMoveClock;
    state.currentPlayer = currentPlayer;
    return state;
}

bool ChessBoard::isCheckmate() const {
    if (!isInCheck(currentPlayer)) return false;
    return generateLegalMoves().empty();
}

bool ChessBoard::isStalemate() const {
    if (isInCheck(currentPlayer)) return false;
    return generateLegalMoves().empty();
}

bool ChessBoard::isInsufficientMaterial() const {
    int wPieces = 0, bPieces = 0;
    bool wBishop = false, bBishop = false;
    bool wKnight = false, bKnight = false;
    int wBishopColor = -1, bBishopColor = -1; // 0=light, 1=dark

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            PieceType pt = board[r][c];
            if (pt == NONE || pt == KING) continue;
            if (colors[r][c] == WHITE) {
                wPieces++;
                if (pt == BISHOP) { wBishop = true; wBishopColor = (r + c) % 2; }
                if (pt == KNIGHT) wKnight = true;
            } else {
                bPieces++;
                if (pt == BISHOP) { bBishop = true; bBishopColor = (r + c) % 2; }
                if (pt == KNIGHT) bKnight = true;
            }
        }
    }

    // K vs K
    if (wPieces == 0 && bPieces == 0) return true;
    // K+B vs K or K+N vs K
    if (wPieces == 1 && bPieces == 0) return true;
    if (wPieces == 0 && bPieces == 1) return true;
    // K+B vs K+B (same color bishops)
    if (wPieces == 1 && bPieces == 1 && wBishop && bBishop && wBishopColor == bBishopColor) return true;
    // K+N vs K+N (only two knights — technically not a draw unless no forced mate, but treated as draw)
    if (wPieces == 1 && bPieces == 1 && wKnight && bKnight) return true;

    return false;
}

std::string ChessBoard::getPositionKey() const {
    std::string fen = getFEN();
    // Strip half-move clock and move number (last two fields)
    size_t last = fen.rfind(' ');
    if (last != std::string::npos) fen = fen.substr(0, last);
    last = fen.rfind(' ');
    if (last != std::string::npos) fen = fen.substr(0, last);
    return fen;
}

std::string ChessBoard::getFEN() const {
    std::ostringstream fen;
    int emptyCount = 0;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (board[row][col] == NONE) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen << emptyCount;
                    emptyCount = 0;
                }
                char c = pieceToChar(board[row][col]);
                if (colors[row][col] == WHITE) c = toupper(c);
                fen << c;
            }
        }
        if (emptyCount > 0) {
            fen << emptyCount;
            emptyCount = 0;
        }
        if (row < 7) fen << '/';
    }

    fen << ' ' << (currentPlayer == WHITE ? 'w' : 'b') << ' ';

    bool hasCastling = false;
    if (canCastleKingside(WHITE)) { fen << 'K'; hasCastling = true; }
    if (canCastleQueenside(WHITE)) { fen << 'Q'; hasCastling = true; }
    if (canCastleKingside(BLACK)) { fen << 'k'; hasCastling = true; }
    if (canCastleQueenside(BLACK)) { fen << 'q'; hasCastling = true; }
    if (!hasCastling) fen << '-';

    fen << ' ';
    if (enPassantRow >= 0 && enPassantCol >= 0) {
        fen << char('a' + enPassantCol) << (8 - enPassantRow);
    } else {
        fen << '-';
    }

    fen << ' ' << halfMoveClock << " 1";

    return fen.str();
}

PieceType ChessBoard::charToPiece(char c) {
    switch (tolower(c)) {
        case 'p': return PAWN;
        case 'n': return KNIGHT;
        case 'b': return BISHOP;
        case 'r': return ROOK;
        case 'q': return QUEEN;
        case 'k': return KING;
        default: return NONE;
    }
}

char ChessBoard::pieceToChar(PieceType p) {
    switch (p) {
        case PAWN:   return 'p';
        case KNIGHT: return 'n';
        case BISHOP: return 'b';
        case ROOK:   return 'r';
        case QUEEN:  return 'q';
        case KING:   return 'k';
        default:     return ' ';
    }
}
