#ifndef OPENINGBOOK_H
#define OPENINGBOOK_H

#include <vector>
#include <optional>
#include <string>
#include "ChessBoard.h"

class OpeningBook {
public:
    OpeningBook();
    std::optional<Move> getBookMove(const std::vector<Move>& history) const;
    void clearCache() { lastMatch = -1; }
private:
    struct OpeningLine {
        std::string name;
        std::vector<Move> moves;
    };
    std::vector<OpeningLine> lines;
    mutable int lastMatch;

    void add(const std::string& name, std::vector<Move> moves);
    static Move sq(const std::string& from, const std::string& to);
};

#endif
