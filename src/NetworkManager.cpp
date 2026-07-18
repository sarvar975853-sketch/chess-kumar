#include "NetworkManager.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cstring>

const float NetworkManager::BROADCAST_INTERVAL = 2.0f;

NetworkManager::NetworkManager() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    playerName = "Player" + std::to_string(std::rand() % 9000 + 1000);
}

NetworkManager::~NetworkManager() {
    disconnect();
    udpSocket.unbind();
    tcpListener.close();
}

void NetworkManager::setNonBlocking(sf::TcpSocket& sock) {
    sock.setBlocking(false);
}

void NetworkManager::setState(NetState s) {
    state = s;
}

void NetworkManager::startDiscovery(unsigned short port) {
    discoveryPort = port;
    if (udpSocket.bind(discoveryPort) != sf::Socket::Status::Done) {
        if (onStatusMessage) onStatusMessage("Could not bind discovery port");
        return;
    }
    udpSocket.setBlocking(false);
    setState(NetState::DISCOVERING);
    broadcastClock.restart();

    if (tcpListener.listen(discoveryPort) != sf::Socket::Status::Done) {
        if (onStatusMessage) onStatusMessage("Could not listen on port");
    } else {
        tcpListener.setBlocking(false);
    }

    sendBroadcast();
}

void NetworkManager::stopDiscovery() {
    udpSocket.unbind();
    tcpListener.close();
    if (state != NetState::CONNECTED)
        setState(NetState::IDLE);
}

void NetworkManager::clearPeers() {
    peers.clear();
}

void NetworkManager::updateDiscovery() {
    if (state != NetState::DISCOVERING && state != NetState::CONNECTED && state != NetState::CONNECTING)
        return;

    receiveDiscoveryReplies();

    if (broadcastClock.getElapsedTime().asSeconds() >= BROADCAST_INTERVAL) {
        sendBroadcast();
        broadcastClock.restart();
    }

    cleanupStalePeers();
}

void NetworkManager::sendBroadcast() {
    std::string msg = "CHESS_LAN_DISCOVER";
    udpSocket.send(msg.data(), msg.size(), sf::IpAddress::Broadcast, discoveryPort);
}

void NetworkManager::receiveDiscoveryReplies() {
    char buf[512];
    std::size_t received;
    std::optional<sf::IpAddress> sender;
    unsigned short senderPort;

    while (udpSocket.receive(buf, sizeof(buf) - 1, received, sender, senderPort) == sf::Socket::Status::Done) {
        if (received > 0 && sender.has_value()) {
            buf[received] = '\0';
            processUDPMessage(std::string(buf), *sender);
        }
    }
}

void NetworkManager::processUDPMessage(const std::string& msg, const sf::IpAddress& sender) {
    if (msg == "CHESS_LAN_DISCOVER") {
        std::string reply = "CHESS_LAN_HERE:" + playerName;
        udpSocket.send(reply.data(), reply.size(), sender, discoveryPort);
    } else if (msg.find("CHESS_LAN_HERE:") == 0) {
        std::string name = msg.substr(14);
        if (name == playerName) return;

        std::string ip = sender.toString();
        bool found = false;
        for (auto& p : peers) {
            if (p.ip == ip) {
                p.name = name;
                p.lastSeen.restart();
                found = true;
                break;
            }
        }
        if (!found) {
            DiscoveredPeer peer;
            peer.name = name;
            peer.ip = ip;
            peer.lastSeen.restart();
            peers.push_back(peer);
            if (onStatusMessage) onStatusMessage("Found: " + name);
        }
    }
}

void NetworkManager::cleanupStalePeers() {
    auto it = peers.begin();
    while (it != peers.end()) {
        if (it->lastSeen.getElapsedTime().asSeconds() > 5.0f) {
            if (onStatusMessage) onStatusMessage("Lost: " + it->name);
            it = peers.erase(it);
        } else {
            ++it;
        }
    }
}

bool NetworkManager::connectToPeer(const std::string& ip, unsigned short port) {
    if (state == NetState::CONNECTED || state == NetState::CONNECTING) return false;

    auto addrOpt = sf::IpAddress::fromString(ip);
    if (!addrOpt.has_value()) return false;

    setState(NetState::CONNECTING);
    setNonBlocking(tcpSocket);
    tcpSocket.connect(*addrOpt, port);
    role = NetRole::CLIENT;
    if (onStatusMessage) onStatusMessage("Connecting to " + ip + "...");
    return true;
}

void NetworkManager::disconnect() {
    if (state == NetState::CONNECTED)
        sendTCPString("BYE");
    tcpSocket.disconnect();
    tcpListener.close();
    opponentName.clear();
    role = NetRole::NONE;
    pendingMove.reset();
    receivedDrawOffer = false;
    recvBuffer.clear();
    if (state != NetState::IDLE)
        setState(NetState::DISCONNECTED);
}

void NetworkManager::updateConnection() {
    if (state == NetState::CONNECTING) {
        auto remote = tcpSocket.getRemoteAddress();
        if (remote.has_value()) {
            sendTCPString("HELLO:" + playerName);
            setState(NetState::CONNECTED);
            if (onStatusMessage) onStatusMessage("Connected!");
            pingClock.restart();
        }
        return;
    }

    if (state == NetState::DISCOVERING || state == NetState::CONNECTED) {
        sf::TcpSocket incoming;
        if (tcpListener.accept(incoming) == sf::Socket::Status::Done) {
            if (state != NetState::CONNECTED) {
                tcpSocket.disconnect();
                tcpSocket = std::move(incoming);
                setNonBlocking(tcpSocket);
                role = NetRole::HOST;
                setState(NetState::CONNECTED);
                pingClock.restart();
                if (onStatusMessage) onStatusMessage("Player connected!");
            }
        }
    }

    if (state == NetState::CONNECTED)
        receiveTCPData();
}

void NetworkManager::receiveTCPData() {
    char buf[4096];
    std::size_t received;
    sf::Socket::Status status = tcpSocket.receive(buf, sizeof(buf) - 1, received);

    if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
        if (onStatusMessage) onStatusMessage("Disconnected");
        disconnect();
        setState(NetState::DISCONNECTED);
        return;
    }

    if (status == sf::Socket::Status::NotReady || received == 0) return;

    buf[received] = '\0';
    recvBuffer += buf;

    size_t pos;
    while ((pos = recvBuffer.find('\n')) != std::string::npos) {
        std::string msg = recvBuffer.substr(0, pos);
        recvBuffer.erase(0, pos + 1);
        if (!msg.empty()) processTCPMessage(msg);
    }
}

void NetworkManager::processTCPMessage(const std::string& msg) {
    if (msg.find("HELLO:") == 0) {
        opponentName = msg.substr(6);
        if (onStatusMessage) onStatusMessage("Playing vs " + opponentName);
    } else if (msg.find("MOVE:") == 0) {
        std::string data = msg.substr(5);
        int fr, fc, tr, tc, promo = 0;
        if (std::sscanf(data.c_str(), "%d:%d:%d:%d:%d", &fr, &fc, &tr, &tc, &promo) >= 4)
            pendingMove = Move(fr, fc, tr, tc, static_cast<PieceType>(promo));
    } else if (msg == "RESIGN") {
        pendingMove = Move(-2, -2, -2, -2);
    } else if (msg == "DRAW_OFFER") {
        receivedDrawOffer = true;
    } else if (msg == "DRAW_ACCEPT") {
        pendingMove = Move(-3, -3, -3, -3);
    } else if (msg == "DRAW_DECLINE") {
        if (onStatusMessage) onStatusMessage("Draw declined");
    } else if (msg == "BYE") {
        if (onStatusMessage) onStatusMessage("Opponent disconnected");
        disconnect();
        setState(NetState::DISCONNECTED);
    }
}

void NetworkManager::sendTCPString(const std::string& msg) {
    std::string data = msg + "\n";
    tcpSocket.send(data.data(), data.size());
}

void NetworkManager::sendMove(const Move& move) {
    std::string msg = "MOVE:" + std::to_string(move.fromRow) + ":"
                     + std::to_string(move.fromCol) + ":"
                     + std::to_string(move.toRow) + ":"
                     + std::to_string(move.toCol) + ":"
                     + std::to_string(move.promotion);
    sendTCPString(msg);
}

void NetworkManager::sendResign()   { sendTCPString("RESIGN"); }
void NetworkManager::sendDrawOffer()  { sendTCPString("DRAW_OFFER"); }
void NetworkManager::sendDrawAccept() { sendTCPString("DRAW_ACCEPT"); }
void NetworkManager::sendDrawDecline() { sendTCPString("DRAW_DECLINE"); }

Move NetworkManager::consumePendingMove() {
    Move m = pendingMove.value();
    pendingMove.reset();
    return m;
}

std::string NetworkManager::getStatusText() const {
    switch (state) {
        case NetState::IDLE: return "Not connected";
        case NetState::DISCOVERING: return "Searching for players...";
        case NetState::CONNECTING: return "Connecting...";
        case NetState::CONNECTED:
            if (role == NetRole::HOST) return "Hosting vs " + opponentName;
            return "Connected to " + opponentName;
        case NetState::DISCONNECTED: return "Disconnected";
    }
    return "";
}
