struct NetworkStateClient {
    sf::IPAddress ip;
    uint32_t clientID;
    int port;

    sf::SocketUDP udp;
    sf::SocketTCP tcp;

    bool hasHandledFirstTCPPacket;
};

static void sendTCP(DataQueue& q, sf::SocketTCP& socket) {
    sf::Lock lock(q.mutex);
    for (; !q.q.empty(); q.q.pop()) {
        DataQueue::Member& member = q.q.back();
        printf("Send TCP\n");
        socket.Send(member.packet);
    }
}
static void sendUDP(DataQueue& q, sf::SocketUDP& socket, sf::IPAddress& ip, int port) {
    sf::Lock lock(q.mutex);
    for (; !q.q.empty(); q.q.pop()) {
        DataQueue::Member& member = q.q.back();
        printf("Send UDP\n");
        socket.Send(member.packet, ip, port);
    }
}

static void clientReadPacket(NetworkStateClient& client, sf::Packet& packet, bool isTCP) {

    // Do something with the packet...
    printf("CLIENT READ PACKET: is TCP? %d, has handled first? %d\n", isTCP, client.hasHandledFirstTCPPacket);
    if (isTCP && !client.hasHandledFirstTCPPacket) {
        // This is our client ID, hopefully
        client.hasHandledFirstTCPPacket = true;

        uint32_t cid;
        packet >> cid;

        mainQueue().push(game_setClientID, InvocationMessage(cid, ""));
        return;
    }

    if (!isTCP) {
        const char* dataptr = packet.GetData();
        mainQueue().push(game_clientQuickUpdate, InvocationMessage(0, std::string(dataptr, dataptr + packet.GetDataSize())));
    }
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
