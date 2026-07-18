#include "AssetManager.h"
#include "ChessBoard.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <climits>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
static std::string getExecutableDir() {
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) {
        char resolved[PATH_MAX];
        if (realpath(buf, resolved)) {
            return std::string(dirname(resolved));
        }
        return std::string(dirname(buf));
    }
    return "";
}
#endif

AssetManager::AssetManager()
    : texturesLoaded(false), fontLoaded(false), squareSize(80) {
}

AssetManager::~AssetManager() {
}

std::string AssetManager::getAssetsPath() const {
    struct stat info;

#ifdef __APPLE__
    // On macOS, resolve from the executable's actual location (works from .app bundle)
    std::string exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        // Contents/MacOS -> Contents/Resources/assets
        std::string resPath = exeDir + "/../Resources/assets";
        if (stat(resPath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
            return resPath;
        }
        // Contents/MacOS -> Contents/MacOS/assets (post-build copy)
        std::string macPath = exeDir + "/assets";
        if (stat(macPath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
            return macPath;
        }
    }
#endif

    // Try bundle-relative path (works when CWD is Contents/MacOS)
    std::string bundlePath = "../Resources/assets";
    if (stat(bundlePath.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
        return bundlePath;
    }

    // Try current directory
    std::string path = "assets";
    if (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
        return path;
    }

    // Try parent directory
    path = "../assets";
    if (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR)) {
        return path;
    }

    // Fall back to creating local assets dir
    MKDIR("assets");
    return "assets";
}

std::string AssetManager::getPieceFilename(PieceType type, PieceColor color) const {
    std::string name;
    name += (color == WHITE) ? 'w' : 'b';
    switch (type) {
        case PAWN:   name += "P"; break;
        case KNIGHT: name += "N"; break;
        case BISHOP: name += "B"; break;
        case ROOK:   name += "R"; break;
        case QUEEN:  name += "Q"; break;
        case KING:   name += "K"; break;
        default:     name += "x"; break;
    }
    name += ".png";
    return name;
}

bool AssetManager::loadPieceTexture(PieceType type, PieceColor color) {
    std::string assetsPath = getAssetsPath();
    std::string filepath = assetsPath + "/" + getPieceFilename(type, color);

    sf::Texture texture;
    if (texture.loadFromFile(filepath)) {
        PieceKey key = {type, color};
        pieceTextures[key] = std::move(texture);
        return true;
    }

    // Create fallback if file not found
    PieceKey key = {type, color};
    pieceTextures[key] = createFallbackTexture(type, color);
    return false;
}

sf::Texture AssetManager::createFallbackTexture(PieceType type, PieceColor color) {
    int cx = PIECE_SIZE / 2;
    int cy = PIECE_SIZE / 2;
    int radius = PIECE_SIZE / 2 - 4;

    sf::Color pieceColor = (color == WHITE) ? sf::Color::White : sf::Color(40, 40, 40);
    sf::Color borderColor = (color == WHITE) ? sf::Color::Black : sf::Color(200, 200, 200);

    sf::RenderTexture renderTex;
    renderTex.resize({PIECE_SIZE, PIECE_SIZE});
    renderTex.clear(sf::Color::Transparent);

    sf::CircleShape circle(static_cast<float>(radius));
    circle.setFillColor(pieceColor);
    circle.setOutlineColor(borderColor);
    circle.setOutlineThickness(2.0f);
    circle.setPosition(sf::Vector2f(4.0f, 4.0f));
    renderTex.draw(circle);

    sf::RectangleShape rect;
    rect.setFillColor(borderColor);

    float r = static_cast<float>(radius);

    switch (type) {
        case PAWN: {
            sf::CircleShape head(r * 0.3f);
            head.setFillColor(borderColor);
            head.setPosition(sf::Vector2f(static_cast<float>(cx) - head.getRadius(),
                              static_cast<float>(cy) - r * 0.6f));
            renderTex.draw(head);
            sf::RectangleShape body(sf::Vector2f(r * 0.5f, r * 0.4f));
            body.setFillColor(borderColor);
            body.setPosition(sf::Vector2f(static_cast<float>(cx) - r * 0.25f,
                              static_cast<float>(cy) + r * 0.1f));
            renderTex.draw(body);
            break;
        }
        case KNIGHT: {
            sf::ConvexShape horse;
            horse.setPointCount(6);
            horse.setPoint(0, sf::Vector2f(cx - r*0.4f, cy + r*0.4f));
            horse.setPoint(1, sf::Vector2f(cx + r*0.4f, cy + r*0.4f));
            horse.setPoint(2, sf::Vector2f(cx + r*0.4f, cy));
            horse.setPoint(3, sf::Vector2f(cx + r*0.1f, cy - r*0.1f));
            horse.setPoint(4, sf::Vector2f(cx - r*0.1f, cy - r*0.5f));
            horse.setPoint(5, sf::Vector2f(cx - r*0.3f, cy - r*0.3f));
            horse.setFillColor(borderColor);
            renderTex.draw(horse);
            break;
        }
        case BISHOP: {
            sf::ConvexShape bishop;
            bishop.setPointCount(8);
            bishop.setPoint(0, sf::Vector2f(cx, cy - r));
            bishop.setPoint(1, sf::Vector2f(cx + r*0.4f, cy - r*0.6f));
            bishop.setPoint(2, sf::Vector2f(cx + r*0.6f, cy - r*0.2f));
            bishop.setPoint(3, sf::Vector2f(cx + r*0.5f, cy + r*0.5f));
            bishop.setPoint(4, sf::Vector2f(cx, cy + r*0.7f));
            bishop.setPoint(5, sf::Vector2f(cx - r*0.5f, cy + r*0.5f));
            bishop.setPoint(6, sf::Vector2f(cx - r*0.6f, cy - r*0.2f));
            bishop.setPoint(7, sf::Vector2f(cx - r*0.4f, cy - r*0.6f));
            bishop.setFillColor(borderColor);
            renderTex.draw(bishop);
            break;
        }
        case ROOK: {
            sf::RectangleShape base(sf::Vector2f(r * 1.2f, r * 0.3f));
            base.setFillColor(borderColor);
            base.setPosition(sf::Vector2f(cx - r * 0.6f, cy + r * 0.3f));
            renderTex.draw(base);
            sf::RectangleShape tower(sf::Vector2f(r * 0.2f, r * 0.7f));
            tower.setFillColor(borderColor);
            tower.setPosition(sf::Vector2f(cx - r * 0.5f, cy - r * 0.4f));
            renderTex.draw(tower);
            sf::RectangleShape tower2(sf::Vector2f(r * 0.2f, r * 0.7f));
            tower2.setFillColor(borderColor);
            tower2.setPosition(sf::Vector2f(cx + r * 0.3f, cy - r * 0.4f));
            renderTex.draw(tower2);
            sf::RectangleShape top(sf::Vector2f(r * 1.3f, r * 0.2f));
            top.setFillColor(borderColor);
            top.setPosition(sf::Vector2f(cx - r * 0.65f, cy - r * 0.5f));
            renderTex.draw(top);
            break;
        }
        case QUEEN: {
            sf::ConvexShape crown;
            crown.setPointCount(10);
            crown.setPoint(0, sf::Vector2f(cx - r*0.6f, cy + r*0.5f));
            crown.setPoint(1, sf::Vector2f(cx - r*0.6f, cy - r*0.3f));
            crown.setPoint(2, sf::Vector2f(cx - r*0.4f, cy - r*0.1f));
            crown.setPoint(3, sf::Vector2f(cx - r*0.2f, cy - r*0.5f));
            crown.setPoint(4, sf::Vector2f(cx, cy - r*0.2f));
            crown.setPoint(5, sf::Vector2f(cx + r*0.2f, cy - r*0.5f));
            crown.setPoint(6, sf::Vector2f(cx + r*0.4f, cy - r*0.1f));
            crown.setPoint(7, sf::Vector2f(cx + r*0.6f, cy - r*0.3f));
            crown.setPoint(8, sf::Vector2f(cx + r*0.6f, cy + r*0.5f));
            crown.setFillColor(borderColor);
            renderTex.draw(crown);
            break;
        }
        case KING: {
            sf::ConvexShape crown;
            crown.setPointCount(12);
            float kr = r * 0.9f;
            crown.setPoint(0, sf::Vector2f(cx - kr*0.6f, cy + kr*0.5f));
            crown.setPoint(1, sf::Vector2f(cx - kr*0.6f, cy - kr*0.2f));
            crown.setPoint(2, sf::Vector2f(cx - kr*0.4f, cy - kr*0.1f));
            crown.setPoint(3, sf::Vector2f(cx - kr*0.3f, cy - kr*0.5f));
            crown.setPoint(4, sf::Vector2f(cx - kr*0.15f, cy - kr*0.2f));
            crown.setPoint(5, sf::Vector2f(cx, cy - kr*0.6f));
            crown.setPoint(6, sf::Vector2f(cx + kr*0.15f, cy - kr*0.2f));
            crown.setPoint(7, sf::Vector2f(cx + kr*0.3f, cy - kr*0.5f));
            crown.setPoint(8, sf::Vector2f(cx + kr*0.4f, cy - kr*0.1f));
            crown.setPoint(9, sf::Vector2f(cx + kr*0.6f, cy - kr*0.2f));
            crown.setPoint(10, sf::Vector2f(cx + kr*0.6f, cy + kr*0.5f));
            crown.setFillColor(borderColor);
            renderTex.draw(crown);
            sf::RectangleShape crossV(sf::Vector2f(kr*0.1f, kr*0.5f));
            crossV.setFillColor(borderColor);
            crossV.setPosition(sf::Vector2f(cx - kr*0.05f, cy - kr*0.7f));
            renderTex.draw(crossV);
            sf::RectangleShape crossH(sf::Vector2f(kr*0.4f, kr*0.1f));
            crossH.setFillColor(borderColor);
            crossH.setPosition(sf::Vector2f(cx - kr*0.2f, cy - kr*0.55f));
            renderTex.draw(crossH);
            break;
        }
        default: break;
    }

    renderTex.display();
    sf::Image capturedImage = renderTex.getTexture().copyToImage();
    sf::Texture result;
    result.loadFromImage(capturedImage);
    return result;
}

bool AssetManager::loadFont() {
    // Resolve assets path once
    std::string assetsBase = getAssetsPath();
    std::string fontInAssets = assetsBase + "/font.ttf";

    // Try bundled font first
    struct stat st;
    if (stat(fontInAssets.c_str(), &st) == 0 && font.openFromFile(fontInAssets)) {
        fontLoaded = true;
        return true;
    }

    // Try system fonts
    std::vector<std::string> fontPaths = {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\Arial.ttf",
        "C:\\Windows\\Fonts\\Calibri.ttf",
        "C:\\Windows\\Fonts\\SegoeUI.ttf",
#elif __APPLE__
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/SFCompact.ttf",
        "/Library/Fonts/Arial.ttf",
#else
        "/usr/share/fonts/truetype/liberation/LiberanceSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
#endif
    };

    for (const auto& path : fontPaths) {
        if (stat(path.c_str(), &st) == 0 && font.openFromFile(path)) {
            fontLoaded = true;
            return true;
        }
    }

    fontLoaded = false;
    std::cerr << "Warning: Could not load any font. Text will not be rendered." << std::endl;
    return false;
}

bool AssetManager::initialize() {
    texturesLoaded = false;
    fontLoaded = false;

    PieceType types[] = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
    PieceColor colors[] = {WHITE, BLACK};
    int loadedCount = 0;
    int totalCount = 12;

    for (PieceColor c : colors) {
        for (PieceType t : types) {
            if (loadPieceTexture(t, c)) {
                loadedCount++;
            }
        }
    }

    texturesLoaded = (loadedCount > 0);

    if (texturesLoaded) {
        std::cout << "Loaded " << loadedCount << "/" << totalCount << " piece textures." << std::endl;
    } else {
        std::cout << "Creating fallback piece graphics." << std::endl;
        for (PieceColor c : colors) {
            for (PieceType t : types) {
                PieceKey key = {t, c};
                pieceTextures[key] = createFallbackTexture(t, c);
            }
        }
        texturesLoaded = true;
    }

    loadFont();

    return true;
}

const sf::Texture* AssetManager::getPieceTexture(PieceType type, PieceColor color) const {
    PieceKey key = {type, color};
    auto it = pieceTextures.find(key);
    if (it != pieceTextures.end()) {
        return &it->second;
    }
    return nullptr;
}
