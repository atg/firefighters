#import <SFML/Network.hpp>
#import <algorithm>
#import "main/game.hpp"

/* Networking model
    There are two sockets for each client
        1) A TCP socket, for information that must be received (e.g sending new map tiles)
        2) A UDP socket, for information that must be sent quickly, but may be dropped (e.g sending world updates)
*/

struct DataQueue {
    sf::Mutex mutex;
    struct Member {
        sf::Packet packet;
        uint32_t clientID;
    };
    std::queue<DataQueue::Member> q;

    void push(sf::Packet packet, uint32_t clientID) {
        sf::Lock lock(mutex);
        DataQueue::Member member;
        member.packet = packet;
        member.clientID = clientID;
        q.push(member);
    }
};

static DataQueue& udpQueue() {
    static DataQueue _udpQueue;
    return _udpQueue;
}
static DataQueue& tcpQueue() {
    static DataQueue _tcpQueue;
    return _tcpQueue;
}

struct NetworkStateClient {
    sf::IPAddress ip;
    uint32_t clientID;
    int port;
    
    sf::SocketUDP udp;
    sf::SocketTCP tcp;
    
    bool hasHandledFirstTCPPacket;
};
struct NetworkStateServer {
    sf::IPAddress ip;
    int port;
    
    sf::SocketUDP udpListener;
    sf::SelectorUDP udpSelector;
    
    sf::SocketTCP tcpListener;
    sf::SelectorTCP tcpSelector;
    
    struct ClientState {
        uint32_t clientID;
        sf::IPAddress ip;
        
        // bool hasUDPSocket;
        // sf::SocketUDP udpSocket;
        
        bool hasTCPSocket;
        sf::SocketTCP tcpSocket;
        
        ClientState() : clientID(-1), ip(), /*hasUDPSocket(), udpSocket(),*/ hasTCPSocket(), tcpSocket() { }
    };
    std::vector<NetworkStateServer::ClientState> clients;
};

static void sendTCP(DataQueue& q, std::vector<NetworkStateServer::ClientState>& clients) {
    sf::Lock lock(q.mutex);
    for (; !q.q.empty(); q.q.pop()) {
        DataQueue::Member& member = q.q.back();
        
        // Find a socket that we can send to
        for (NetworkStateServer::ClientState& client : clients) {
            if (client.clientID != member.clientID)
                continue;
            if (!client.hasTCPSocket)
                continue;
            
            client.tcpSocket.Send(member.packet);
            break;
        }
    }
}
static void sendTCP(DataQueue& q, sf::SocketTCP& socket) {
    sf::Lock lock(q.mutex);
    for (; !q.q.empty(); q.q.pop()) {
        DataQueue::Member& member = q.q.back();
        socket.Send(member.packet);
    }
}
static void sendUDP(DataQueue& q, sf::SocketUDP& socket, const std::vector<NetworkStateServer::ClientState>& clients, int port) {
    sf::Lock lock(q.mutex);
    for (; !q.q.empty(); q.q.pop()) {
        DataQueue::Member& member = q.q.back();
        
        // Find a socket that we can send to
        for (const NetworkStateServer::ClientState& client : clients) {
            if (client.clientID != member.clientID)
                continue;
            // It needs to have a TCP socket before it has an IP address
            if (!client.hasTCPSocket)
                continue;
            
            socket.Send(member.packet, client.ip, port);
            break;
        }
    }
}
static void sendUDP(DataQueue& q, sf::SocketUDP& socket, sf::IPAddress& ip, int port) {
    sf::Lock lock(q.mutex);
    for (; !q.q.empty(); q.q.pop()) {
        DataQueue::Member& member = q.q.back();
        socket.Send(member.packet, ip, port);
    }
}

static void serverReadPacket(NetworkStateServer& server, sf::Packet& packet, bool isTCP) {
    // Do something with the packet...
}
static void clientReadPacket(NetworkStateClient& client, sf::Packet& packet, bool isTCP) {
    
    // Do something with the packet...
    if (isTCP && !client.hasHandledFirstTCPPacket) {
        // This is our client ID, hopefully
        client.hasHandledFirstTCPPacket = true;
        
        uint32_t cid;
        packet >> cid;
        
        mainQueue().push(game_setClientID, cid);
    }
}


static void serverNetworkThread(void* userData) {
    
    static NetworkStateServer server;
    server.port = GAME.port;
    
    // Begin listening
    if (!server.tcpListener.Listen(server.port)) {
        fprintf(stderr, "Could not listen on TCP port %d\n", server.port);
        abort();
    }
    
    printf("Server is listening on TCP port %d\n", server.port);
    server.tcpSelector.Add(server.tcpListener);
    
    
    server.udpListener.SetBlocking(false);
    if (!server.udpListener.Bind(server.port + 1)) {
        fprintf(stderr, "Could not bind on UDP port %d\n", server.port + 1);
        abort();
    }
    
    printf("Server bound to UDP port %d\n", server.port + 1);
    
    while (true) {
        // UDP
        unsigned n = server.clients.size();
        if (n == 0)
            n++;
        for (unsigned i = 0; i < n; i++) {
            sf::Packet packet;
            unsigned short port = server.port + 1;
            sf::IPAddress address;
            if (server.udpListener.Receive(packet, address, port) == sf::Socket::Done) {
                serverReadPacket(server, packet, false);
            }
        }
        
        // TCP
        n = server.tcpSelector.Wait(0.01);
        for (unsigned i = 0; i < n; i++) {
            sf::SocketTCP socket = server.tcpSelector.GetSocketReady(i);
            
            if (socket == server.tcpListener) {
                // It wants us to Listen for a new client
                sf::IPAddress address;
                sf::SocketTCP client;
                server.tcpListener.Accept(client, &address);
                
                NetworkStateServer::ClientState* clientState;
                for (NetworkStateServer::ClientState& otherClient : server.clients) {
                    if (otherClient.ip == address)
                        clientState = &otherClient;
                }
                if (!clientState) {
                    NetworkStateServer::ClientState _clientState = NetworkStateServer::ClientState();
                    _clientState.ip = address;
                    server.clients.push_back(_clientState);
                    clientState = &(server.clients.back());
                }
                else if (clientState->hasTCPSocket) {
                    clientState->tcpSocket.Close();
                }
                
                clientState->hasTCPSocket = true;
                clientState->tcpSocket = client;
                
                server.tcpSelector.Add(client);
                
                // Send a PlayerID packet
                static uint32_t nextClientPlayerID = 2;
                sf::Packet playerIDPacket;
                playerIDPacket << nextClientPlayerID;
                nextClientPlayerID++;
                client.Send(playerIDPacket);
            }
            else {
                // It wants us to read from the given socket
                sf::Packet packet;
                if (socket.Receive(packet) == sf::Socket::Done) {
                    
                    // Read from the packet!
                    serverReadPacket(server, packet, true);
                }
                else {
                    // Oops, error. Remove the socket.
                    server.tcpSelector.Remove(socket);
                    // remove_if(server.clients.begin(), server.clients.end(), socket);
                }
            }
        }

        // Check if we have any messages to send
        sendUDP(udpQueue(), server.udpListener, server.clients, server.port + 2);
        sendTCP(tcpQueue(), server.clients);
    }
    
    server.tcpListener.Close();
}
static void clientNetworkThread(void* userData) {
    // Connect to the given server
    sf::IPAddress serverAddress((GAME.serverIP));
    
    static NetworkStateClient client;
    client.port = GAME.port;
    client.ip = serverAddress;
    client.hasHandledFirstTCPPacket = false;
    
    if (client.tcp.Connect(client.port, client.ip) != sf::Socket::Done) {
        fprintf(stderr, "Could not connect to TCP server %s on port %d\n", GAME.serverIP.c_str(), client.port);
        abort();
    }
    client.tcp.SetBlocking(false);
    
    if (!client.udp.Bind(client.port + 2)) {
        fprintf(stderr, "Could not bind on UDP port %d\n", client.port + 2);
        abort();
    }
    
    printf("Server bound to UDP port %d\n", client.port + 2);
    client.udp.SetBlocking(false);
    
    while (true) {
        
        // Read from UDP
        {
            sf::Packet packet;
            unsigned short port = client.port + 2;
            sf::IPAddress address;
            if (client.udp.Receive(packet, address, port) == sf::Socket::Done) {
                clientReadPacket(client, packet, false);
            }
        }
        
        // Send to UDP
        sendUDP(udpQueue(), client.udp, client.ip, client.port + 1);
        
        // Read from TCP
        {
            sf::Packet packet;
            if (client.tcp.Receive(packet) == sf::Socket::Done) {
                clientReadPacket(client, packet, true);
            }
        }
        
        // Send to TCP
        sendTCP(tcpQueue(), client.tcp);
    }
}


static void startNetworking() {
    
    static sf::Thread thread(GAME.isServer ? &serverNetworkThread : &clientNetworkThread);
    thread.Launch();
}
