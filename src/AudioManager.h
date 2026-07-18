#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SFML/Audio.hpp>
#include <memory>
#include "Settings.h"

class AudioManager {
public:
    AudioManager();
    ~AudioManager();
    void shutdown();
    void applySettings(const Settings& s);

    void playMove();
    void playCapture();
    void playCheck();
    void playCheckmate();
    void playStalemate();
    void playPieceSelect();

    void startMusic();
    void stopMusic();
    void updateMusic();

private:
    void genMoveBuf();
    void genCaptureBuf();
    void genCheckBuf();
    void genCheckmateBuf();
    void genStalemateBuf();
    void genPieceSelectBuf();
    void advanceTrack();

    sf::SoundBuffer moveBuf, captureBuf, checkBuf, checkmateBuf, stalemateBuf, pieceSelBuf;
    std::unique_ptr<sf::Sound> moveSnd, captureSnd, checkSnd, checkmateSnd, stalemateSnd, pieceSelSnd;
    std::unique_ptr<sf::Music> music;

    std::vector<std::string> playlist;
    int currentTrack = -1;

    bool sndOn = true, musOn = true;
    bool musicLoaded = false;
};

#endif
