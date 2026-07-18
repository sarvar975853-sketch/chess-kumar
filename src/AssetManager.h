#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include "ChessBoard.h"

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    bool initialize();

    const sf::Texture* getPieceTexture(PieceType type, PieceColor color) const;
    sf::Font* getFont() { return fontLoaded ? &font : nullptr; }
    bool hasTextures() const { return texturesLoaded; }
    bool hasFont() const { return fontLoaded; }

    int getSquareSize() const { return squareSize; }
    void setSquareSize(int size) { squareSize = size; }

private:
    static const int PIECE_SIZE = 64;

    struct PieceKey {
        PieceType type;
        PieceColor color;
        bool operator==(const PieceKey& other) const {
            return type == other.type && color == other.color;
        }
    };

    struct PieceKeyHash {
        std::size_t operator()(const PieceKey& key) const {
            return std::hash<int>()(key.type) ^ (std::hash<int>()(key.color) << 1);
        }
    };

    std::unordered_map<PieceKey, sf::Texture, PieceKeyHash> pieceTextures;
    sf::Font font;
    bool texturesLoaded;
    bool fontLoaded;
    int squareSize;

    std::string getAssetsPath() const;
    bool loadPieceTexture(PieceType type, PieceColor color);
    bool loadFont();
    std::string getPieceFilename(PieceType type, PieceColor color) const;
    sf::Texture createFallbackTexture(PieceType type, PieceColor color);
};

#endif
