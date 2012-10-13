#import <SFML/Network.hpp>
#import "main/game.hpp"

/* Networking model
    There are two sockets for each client
        1) A TCP socket, for information that must be received (e.g sending new map tiles)
        2) A UDP socket, for information that must be sent quickly, but may be dropped (e.g sending world updates)
*/

static int nextClientID = 2;

struct NetworkStateClient {
    sf::IPAddress ip;
    uint32_t clientID;
    int port;
    
    sf::SocketUDP udp;
    sf::SocketTCP tcp;
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
        
        bool hasUDPSocket;
        sf::SocketUDP udpSocket;
        
        bool hasTCPSocket;
        sf::SocketTCP tcpSocket;
        
        ClientState() : clientID(-1), ip(), hasUDPSocket(), udpSocket(), hasTCPSocket(), tcpSocket() { }
    };
    std::vector<NetworkStateServer::ClientState> clients;
};

static void serverSendBytes(std::vector<char> bytes, bool isTCP) {
    
}
static void serverReadPacket(sf::Packet& packet, bool isTCP) {
    // Do something with the packet...
}

static void clientReadPacket(sf::Packet& packet, bool isTCP) {
    // Do something with the packet...
}

static void serverNetworkThread(void* userData) {
    
    static NetworkStateServer server;
    server.port = GAME.port;
    
    // Begin listening
    if (!server.tcpListener.Listen(server.port)) {
        fprintf(stderr, "Could not listen on TCP port %d\n", server.port);
        abort();
    }
    
    // printf("Server is listening on TCP port %d\n", server.port);
    server.tcpSelector.Add(server.tcpListener);
    
    
    server.udpListener.SetBlocking(false);
    if (!server.udpListener.Bind(server.port + 1)) {
        fprintf(stderr, "Could not listen on UDP port %d\n", server.port + 1);
        abort();
    }
    
    // printf("Server is listening on UDP port %d\n", server.port + 1);
    // server.udpSelector.Add(server.udpListener);
    
    while (true) {
        printf("Running\n");
        // UDP
        unsigned n = server.clients.size();
        if (n == 0)
            n++;
        for (unsigned i = 0; i < n; i++) {
            sf::Packet packet;
            unsigned short port = server.port + 1;
            sf::IPAddress address;
            if (server.udpListener.Receive(packet, address, port) == sf::Socket::Done) {
                serverReadPacket(packet, false);
            }
        }
        
        // TCP
        n = server.tcpSelector.Wait(0.01);
        printf("  did wait\n");
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
                
                clientState->hasTCPSocket = true;
                clientState->tcpSocket = client;
                
                server.tcpSelector.Add(client);
            }
            else {
                // It wants us to read from the given socket
                sf::Packet packet;
                if (socket.Receive(packet) == sf::Socket::Done) {
                    
                    // Read from the packet!
                    serverReadPacket(packet, true);
                }
                else {
                    // Oops, error. Remove the socket.
                    server.tcpSelector.Remove(socket);
                }
            }
        }

        // Check if we have any messages to send
        // ...
    }
    
    server.tcpListener.Close();
}
static void clientNetworkThread(void* userData) {
    // Connect to the given server
    sf::IPAddress serverAddress((GAME.serverIP));
    
    static NetworkStateClient client;
    client.port = GAME.port;
    client.ip = serverAddress;
    
    client.tcp.SetBlocking(false);
    if (client.tcp.Connect(client.port, client.ip) != sf::Socket::Done) {
        fprintf(stderr, "Could not connect to TCP server %s on port %d\n", GAME.serverIP.c_str(), client.port);
        abort();
    }
    
    client.udp.SetBlocking(false);
    while (true) {
        
        // Read from UDP
        {
            sf::Packet packet;
            unsigned short port = client.port + 1;
            sf::IPAddress address;
            if (client.udp.Receive(packet, address, port) == sf::Socket::Done) {
                clientReadPacket(packet, false);
            }
        }
        
        // Send to UDP
        // ...
        
        // Read from TCP
        {
            sf::Packet packet;
            if (client.tcp.Receive(packet) == sf::Socket::Done) {
                clientReadPacket(packet, true);
            }
        }
        
        // Send to TCP
        // ...
    }
}


static void startNetworking() {
    
    static sf::Thread thread(GAME.isServer ? &serverNetworkThread : &clientNetworkThread);
    thread.Launch();
}

