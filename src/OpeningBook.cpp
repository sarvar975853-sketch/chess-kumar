#include "OpeningBook.h"
#include <cstdlib>
#include <ctime>

OpeningBook::OpeningBook() : lastMatch(-1) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Italian Game
    add("Italian Game", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("b8","c6"),
        sq("f1","c4")
    });

    // Italian - Two Knights
    add("Two Knights", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("b8","c6"),
        sq("f1","c4"), sq("g8","f6")
    });

    // Italian - Giuoco Piano
    add("Giuoco Piano", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("b8","c6"),
        sq("f1","c4"), sq("f8","c5")
    });

    // Sicilian Defense
    add("Sicilian Defense", {
        sq("e2","e4"), sq("c7","c5"),
        sq("g1","f3"), sq("d7","d6"),
        sq("d2","d4")
    });

    // Sicilian - Open
    add("Sicilian Open", {
        sq("e2","e4"), sq("c7","c5"),
        sq("g1","f3"), sq("d7","d6"),
        sq("d2","d4"), Move(3,2,4,3,NONE),
        sq("f3","d4"), sq("g8","f6"),
        sq("b1","c3")
    });

    // French Defense
    add("French Defense", {
        sq("e2","e4"), sq("e7","e6"),
        sq("d2","d4"), sq("d7","d5")
    });

    // French - Advance
    add("French Advance", {
        sq("e2","e4"), sq("e7","e6"),
        sq("d2","d4"), sq("d7","d5"),
        sq("e4","e5")
    });

    // Caro-Kann
    add("Caro-Kann", {
        sq("e2","e4"), sq("c7","c6"),
        sq("d2","d4"), sq("d7","d5")
    });

    // Caro-Kann - Advance
    add("Caro-Kann Advance", {
        sq("e2","e4"), sq("c7","c6"),
        sq("d2","d4"), sq("d7","d5"),
        sq("e4","e5")
    });

    // Ruy Lopez
    add("Ruy Lopez", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("b8","c6"),
        sq("f1","b5")
    });

    // Ruy Lopez - Morphy
    add("Ruy Lopez Morphy", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("b8","c6"),
        sq("f1","b5"), sq("a7","a6"),
        sq("b5","a4")
    });

    // Scotch Game
    add("Scotch Game", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("b8","c6"),
        sq("d2","d4")
    });

    // Scotch - 3...exd4
    add("Scotch Accepted", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("b8","c6"),
        sq("d2","d4"), sq("e5","d4")
    });

    // Petrov Defense
    add("Petrov Defense", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("g8","f6")
    });

    // Philidor Defense
    add("Philidor Defense", {
        sq("e2","e4"), sq("e7","e5"),
        sq("g1","f3"), sq("d7","d6")
    });

    // Pirc Defense
    add("Pirc Defense", {
        sq("e2","e4"), sq("d7","d6")
    });

    // Scandinavian
    add("Scandinavian", {
        sq("e2","e4"), sq("d7","d5")
    });

    // Alekhine Defense
    add("Alekhine Defense", {
        sq("e2","e4"), sq("g8","f6")
    });

    // Queen's Gambit
    add("Queen's Gambit", {
        sq("d2","d4"), sq("d7","d5"),
        sq("c2","c4")
    });

    // QGD Declined
    add("QGD Declined", {
        sq("d2","d4"), sq("d7","d5"),
        sq("c2","c4"), sq("e7","e6")
    });

    // QGD Exchange
    add("QGD Exchange", {
        sq("d2","d4"), sq("d7","d5"),
        sq("c2","c4"), sq("e7","e6"),
        sq("b1","c3"), sq("g8","f6"),
        Move(4,2,3,3,NONE), Move(2,4,3,3,NONE)
    });

    // Queen's Gambit Accepted
    add("QGA", {
        sq("d2","d4"), sq("d7","d5"),
        sq("c2","c4"), sq("d5","c4")
    });

    // King's Indian
    add("King's Indian", {
        sq("d2","d4"), sq("g8","f6"),
        sq("c2","c4"), sq("g7","g6"),
        sq("b1","c3"), sq("f8","g7")
    });

    // Nimzo-Indian
    add("Nimzo-Indian", {
        sq("d2","d4"), sq("g8","f6"),
        sq("c2","c4"), sq("e7","e6"),
        sq("b1","c3"), sq("f8","b4")
    });

    // Queen's Indian
    add("Queen's Indian", {
        sq("d2","d4"), sq("g8","f6"),
        sq("c2","c4"), sq("e7","e6"),
        sq("g1","f3"), sq("b7","b6")
    });

    // London System
    add("London System", {
        sq("d2","d4"), sq("d7","d5"),
        sq("c1","f4")
    });

    // Grunfeld Defense
    add("Grunfeld", {
        sq("d2","d4"), sq("g8","f6"),
        sq("c2","c4"), sq("g7","g6"),
        sq("b1","c3"), sq("d7","d5")
    });

    // English Opening
    add("English Opening", {
        sq("c2","c4")
    });

    // English - 1...e5
    add("English Symmetrical", {
        sq("c2","c4"), sq("e7","e5")
    });

    // English - 1...c5
    add("English Sicilian", {
        sq("c2","c4"), sq("c7","c5")
    });

    // English - 1...Nf6
    add("English Keres", {
        sq("c2","c4"), sq("g8","f6")
    });

    // Modern Defense
    add("Modern Defense", {
        sq("e2","e4"), sq("g7","g6")
    });

    // Vienna Game
    add("Vienna Game", {
        sq("e2","e4"), sq("e7","e5"),
        sq("b1","c3")
    });
}

void OpeningBook::add(const std::string& name, std::vector<Move> moves) {
    lines.push_back({name, std::move(moves)});
}

Move OpeningBook::sq(const std::string& from, const std::string& to) {
    auto parse = [](const std::string& s, int& row, int& col) {
        col = s[0] - 'a';
        row = 8 - (s[1] - '0');
    };
    int fr, fc, tr, tc;
    parse(from, fr, fc);
    parse(to, tr, tc);
    return Move(fr, fc, tr, tc);
}

std::optional<Move> OpeningBook::getBookMove(const std::vector<Move>& history) const {
    if (lines.empty()) return std::nullopt;

    // Try the last successfully matched line first
    if (lastMatch >= 0 && lastMatch < (int)lines.size()) {
        const auto& line = lines[lastMatch];
        if ((int)history.size() < (int)line.moves.size()) {
            bool match = true;
            for (size_t i = 0; i < history.size(); ++i) {
                if (!(history[i] == line.moves[i])) {
                    match = false;
                    break;
                }
            }
            if (match) return line.moves[history.size()];
        }
    }

    // Collect all candidate lines whose prefix matches history
    std::vector<int> candidates;
    for (int i = 0; i < (int)lines.size(); ++i) {
        const auto& line = lines[i];
        if ((int)history.size() >= (int)line.moves.size()) continue;
        bool match = true;
        for (size_t j = 0; j < history.size(); ++j) {
            if (!(history[j] == line.moves[j])) {
                match = false;
                break;
            }
        }
        if (match) candidates.push_back(i);
    }

    if (candidates.empty()) return std::nullopt;

    // Pick a random candidate
    int choice = candidates[std::rand() % candidates.size()];
    lastMatch = choice;
    return lines[choice].moves[history.size()];
}
