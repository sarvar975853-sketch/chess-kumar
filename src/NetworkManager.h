#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <SFML/Network.hpp>
#include <string>
#include <vector>
#include <functional>
#include "ChessBoard.h"

struct DiscoveredPeer {
    std::string name;
    std::string ip;
    sf::Clock lastSeen;
};

enum class NetState { IDLE, DISCOVERING, CONNECTING, CONNECTED, DISCONNECTED };
enum class NetRole { NONE, HOST, CLIENT };

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    void setPlayerName(const std::string& name) { playerName = name; }
    std::string getPlayerName() const { return playerName; }

    void startDiscovery(unsigned short port = 42069);
    void stopDiscovery();
    void updateDiscovery();
    const std::vector<DiscoveredPeer>& getPeers() const { return peers; }
    void clearPeers();

    bool connectToPeer(const std::string& ip, unsigned short port = 42069);
    void disconnect();
    bool isConnected() const { return state == NetState::CONNECTED; }
    NetState getState() const { return state; }
    NetRole getRole() const { return role; }
    std::string getOpponentName() const { return opponentName; }

    void sendMove(const Move& move);
    void sendResign();
    void sendDrawOffer();
    void sendDrawAccept();
    void sendDrawDecline();
    bool hasReceivedDrawOffer() const { return receivedDrawOffer; }
    void clearDrawOffer() { receivedDrawOffer = false; }

    void updateConnection();
    bool hasPendingMove() const { return pendingMove.has_value(); }
    Move consumePendingMove();

    std::string getStatusText() const;
    int getPingMs() const { return pingMs; }

    std::function<void(const std::string&)> onStatusMessage;

private:
    static const unsigned short DEFAULT_PORT = 42069;

    std::string playerName = "Player";
    NetState state = NetState::IDLE;
    NetRole role = NetRole::NONE;
    std::string opponentName;

    sf::UdpSocket udpSocket;
    sf::TcpListener tcpListener;
    sf::TcpSocket tcpSocket;
    bool socketBlocking = false;

    unsigned short discoveryPort = DEFAULT_PORT;
    sf::Clock broadcastClock;
    static const float BROADCAST_INTERVAL;

    std::vector<DiscoveredPeer> peers;
    std::string recvBuffer;
    std::optional<Move> pendingMove;
    bool receivedDrawOffer = false;
    int pingMs = 0;
    sf::Clock pingClock;

    void sendBroadcast();
    void receiveDiscoveryReplies();
    void processUDPMessage(const std::string& msg, const sf::IpAddress& sender);
    void processTCPMessage(const std::string& msg);
    void sendTCPString(const std::string& msg);
    void receiveTCPData();
    void cleanupStalePeers();
    void setState(NetState s);
    void setNonBlocking(sf::TcpSocket& sock);
};

#endif
