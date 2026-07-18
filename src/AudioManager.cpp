#include "AudioManager.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <random>
#include <sys/stat.h>
#include <dirent.h>
#include <climits>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const int SR = 44100;

using Sample = std::int16_t;

static std::vector<Sample> genTone(float freq, float durSec, float volume,
                                   float attack = 0.008f, float release = 0.012f) {
    int ns = static_cast<int>(SR * durSec);
    std::vector<Sample> buf(ns, 0);
    int aSamps = std::min(ns, std::max(1, static_cast<int>(SR * attack)));
    int rSamps = std::min(ns, std::max(1, static_cast<int>(SR * release)));
    for (int i = 0; i < ns; ++i) {
        float t = static_cast<float>(i) / SR;
        float env = 1.0f;
        if (i < aSamps) env = static_cast<float>(i) / aSamps;
        if (i > ns - rSamps) env = static_cast<float>(ns - i) / rSamps;
        env = env * env * (3.0f - 2.0f * env);
        float sample = volume * env * std::sin(2.0f * static_cast<float>(M_PI) * freq * t);
        buf[i] = static_cast<Sample>(std::clamp(sample, -32768.0f, 32767.0f));
    }
    return buf;
}

static std::vector<Sample> mix(const std::vector<std::vector<Sample>>& layers) {
    size_t maxLen = 0;
    for (auto& l : layers) if (l.size() > maxLen) maxLen = l.size();
    std::vector<Sample> out(maxLen, 0);
    for (auto& l : layers) {
        for (size_t i = 0; i < l.size(); ++i) {
            int s = out[i] + l[i];
            out[i] = static_cast<Sample>(std::clamp(s, -32768, 32767));
        }
    }
    return out;
}

static std::vector<std::string> scanMusicFolder() {
    std::vector<std::string> files;
    struct stat info;

    // Build list of candidate directories (absolute paths preferred)
    std::vector<std::string> candidates;

#ifdef __APPLE__
    char exeBuf[4096];
    uint32_t exeSize = sizeof(exeBuf);
    if (_NSGetExecutablePath(exeBuf, &exeSize) == 0) {
        // Use realpath to resolve .. and symlinks
        char resolved[4096];
        if (realpath(exeBuf, resolved)) {
            std::string exeDir = dirname(resolved);
            candidates.push_back(exeDir + "/assets/music");
            candidates.push_back(exeDir + "/../Resources/assets/music");
        }
    }
#endif
    candidates.push_back("assets/music");
    candidates.push_back("../assets/music");
    candidates.push_back("../Resources/assets/music");

    for (auto& dir : candidates) {
        // Resolve to absolute path
        std::string absDir;
        char resolved[PATH_MAX];
        if (realpath(dir.c_str(), resolved)) {
            absDir = resolved;
        } else {
            // Try stat to see if it exists (handles relative paths from CWD)
            if (stat(dir.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) {
                absDir = dir;
            } else {
                continue;
            }
        }

        DIR* d = opendir(absDir.c_str());
        if (!d) continue;

        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == ".." || name == ".DS_Store") continue;

            // Check extension
            size_t dot = name.rfind('.');
            if (dot == std::string::npos) continue;
            std::string ext = name.substr(dot);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".ogg" && ext != ".wav" && ext != ".flac") continue;

            std::string fullPath = absDir + "/" + name;
            if (stat(fullPath.c_str(), &info) == 0 && S_ISREG(info.st_mode)) {
                files.push_back(fullPath);
            }
        }
        closedir(d);

        if (!files.empty()) break;
    }

    std::sort(files.begin(), files.end());
    return files;
}

AudioManager::AudioManager() {
    genMoveBuf();
    genCaptureBuf();
    genCheckBuf();
    genCheckmateBuf();
    genStalemateBuf();
    genPieceSelectBuf();

    moveSnd = std::make_unique<sf::Sound>(moveBuf);
    captureSnd = std::make_unique<sf::Sound>(captureBuf);
    checkSnd = std::make_unique<sf::Sound>(checkBuf);
    checkmateSnd = std::make_unique<sf::Sound>(checkmateBuf);
    stalemateSnd = std::make_unique<sf::Sound>(stalemateBuf);
    pieceSelSnd = std::make_unique<sf::Sound>(pieceSelBuf);

    try {
        playlist = scanMusicFolder();
    } catch (std::exception& e) {
        std::cerr << "Warning: could not scan music folder: " << e.what() << std::endl;
    }

    if (!playlist.empty()) {
        // Shuffle playlist
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(playlist.begin(), playlist.end(), g);
        advanceTrack();
    }
}

AudioManager::~AudioManager() {
    shutdown();
}

void AudioManager::shutdown() {
    if (musicLoaded) {
        music->stop();
        musicLoaded = false;
    }
    if (moveSnd) moveSnd->stop();
    if (captureSnd) captureSnd->stop();
    if (checkSnd) checkSnd->stop();
    if (checkmateSnd) checkmateSnd->stop();
    if (stalemateSnd) stalemateSnd->stop();
    if (pieceSelSnd) pieceSelSnd->stop();
}

void AudioManager::applySettings(const Settings& s) {
    sndOn = s.soundEnabled;
    musOn = s.musicEnabled;
    if (!musOn && musicLoaded) music->stop();
}

void AudioManager::genMoveBuf() {
    auto buf = genTone(110.0f, 0.07f, 8000.0f, 0.01f, 0.015f);
    moveBuf.loadFromSamples(buf.data(), buf.size(), 1, SR, {sf::SoundChannel::Mono});
}

void AudioManager::genCaptureBuf() {
    auto a = genTone(160.0f, 0.10f, 10000.0f, 0.005f, 0.02f);
    auto b = genTone(120.0f, 0.10f, 8000.0f, 0.005f, 0.02f);
    auto buf = mix({a, b});
    captureBuf.loadFromSamples(buf.data(), buf.size(), 1, SR, {sf::SoundChannel::Mono});
}

void AudioManager::genCheckBuf() {
    auto a = genTone(440.0f, 0.06f, 7000.0f, 0.005f, 0.01f);
    auto b = genTone(554.0f, 0.06f, 7000.0f, 0.005f, 0.01f);
    std::vector<Sample> buf(a.size() + b.size(), 0);
    for (size_t i = 0; i < a.size(); ++i) buf[i] = a[i];
    for (size_t i = 0; i < b.size(); ++i) buf[a.size() + i] = b[i];
    checkBuf.loadFromSamples(buf.data(), buf.size(), 1, SR, {sf::SoundChannel::Mono});
}

void AudioManager::genCheckmateBuf() {
    auto a = genTone(523.0f, 0.15f, 9000.0f, 0.01f, 0.03f);
    auto b = genTone(659.0f, 0.15f, 9000.0f, 0.01f, 0.03f);
    auto c = genTone(784.0f, 0.20f, 10000.0f, 0.01f, 0.04f);
    std::vector<Sample> buf(a.size() + b.size() + c.size(), 0);
    for (size_t i = 0; i < a.size(); ++i) buf[i] = a[i];
    for (size_t i = 0; i < b.size(); ++i) buf[a.size() + i] = b[i];
    for (size_t i = 0; i < c.size(); ++i) buf[a.size() + b.size() + i] = c[i];
    checkmateBuf.loadFromSamples(buf.data(), buf.size(), 1, SR, {sf::SoundChannel::Mono});
}

void AudioManager::genStalemateBuf() {
    auto a = genTone(262.0f, 0.12f, 7000.0f, 0.01f, 0.03f);
    auto b = genTone(311.0f, 0.12f, 7000.0f, 0.01f, 0.03f);
    auto buf = mix({a, b});
    stalemateBuf.loadFromSamples(buf.data(), buf.size(), 1, SR, {sf::SoundChannel::Mono});
}

void AudioManager::genPieceSelectBuf() {
    auto buf = genTone(200.0f, 0.035f, 5000.0f, 0.003f, 0.008f);
    pieceSelBuf.loadFromSamples(buf.data(), buf.size(), 1, SR, {sf::SoundChannel::Mono});
}



void AudioManager::advanceTrack() {
    if (playlist.empty()) return;

    if (playlist.size() == 1) {
        currentTrack = 0;
    } else {
        int next = currentTrack;
        while (next == currentTrack) {
            next = std::rand() % playlist.size();
        }
        currentTrack = next;
    }

    if (!music) music = std::make_unique<sf::Music>();

    try {
        if (!music->openFromFile(playlist[currentTrack])) {
            playlist.erase(playlist.begin() + currentTrack);
            if (playlist.empty()) { musicLoaded = false; return; }
            currentTrack = (currentTrack >= (int)playlist.size()) ? 0 : currentTrack;
            advanceTrack();
            return;
        }
    } catch (...) {
        playlist.erase(playlist.begin() + currentTrack);
        if (playlist.empty()) { musicLoaded = false; return; }
        currentTrack = (currentTrack >= (int)playlist.size()) ? 0 : currentTrack;
        advanceTrack();
        return;
    }

    musicLoaded = true;
    music->setVolume(70.0f);
    if (musOn) music->play();
}

void AudioManager::updateMusic() {
    if (!musOn || playlist.empty() || !musicLoaded || !music) return;
    if (music->getStatus() == sf::SoundSource::Status::Stopped) {
        advanceTrack();
    }
}

void AudioManager::playMove() { if (sndOn) moveSnd->play(); }
void AudioManager::playCapture() { if (sndOn) captureSnd->play(); }
void AudioManager::playCheck() { if (sndOn) checkSnd->play(); }
void AudioManager::playCheckmate() { if (sndOn) checkmateSnd->play(); }
void AudioManager::playStalemate() { if (sndOn) stalemateSnd->play(); }
void AudioManager::playPieceSelect() { if (sndOn) pieceSelSnd->play(); }
void AudioManager::startMusic() { if (!playlist.empty() && musOn && currentTrack >= 0 && music) music->play(); }
void AudioManager::stopMusic() { if (music) music->stop(); }
