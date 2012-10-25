struct NetServer {

    int port;

    // UDP
    sf::SocketUDP udpListener;

    // TCP
    sf::SocketTCP tcpListener;
    sf::SelectorTCP tcpSelector;


    // ------ Clients ------
    struct Client {
        int clientID;
        sf::IPAddress ip;
        sf::SocketTCP tcpSocket;

        Client() : clientID(-1), ip(), tcpSocket() { }
    };

    sf::Mutex clientMutex;
    std::vector<Client> clients;


    // ------ Sending ------
    void sendToTCP() {
        DataQueue& queue = tcpQueue();
        sf::Lock lock(queue.mutex);
        for (; !queue.q.empty(); queue.q.pop()) {
            DataQueue::Member& member = queue.q.back();

            sf::Lock lock(clientMutex);
            for (Client& client : clients) {
                if (client.clientID != member.clientID) continue;

                printf("Sending TCP %s", client.ip.ToString().c_str());
                client.tcpSocket.Send(member.packet);
                break;
            }
        }
    }
    void sendToUDP() {
        DataQueue& queue = udpQueue();
        int port = port + 2;

        sf::Lock lock(queue.mutex);
        for (; !queue.q.empty(); queue.q.pop()) {
            DataQueue::Member& member = queue.q.back();

            sf::Lock lock(clientMutex);
            for (Client& client : clients) {
                if (client.clientID != member.clientID) continue;
                
                printf("Sending UDP %s:%d", client.ip.ToString().c_str(), port);
                udpListener.Send(member.packet, client.ip, port);
                break;
            }
        }
    }

    // ------ Receiving ------
    void serverReadPacket(uint32_t clientID, sf::Packet& packet, bool isTCP) {
        static int i;
        printf("%d\tReceived %d bytes\n", i, (int)(packet.GetDataSize()));
        i++;

        if (!isTCP && clientID > 0) {
            const char* dataptr = packet.GetData();
            std::string data = std::string(dataptr, dataptr + packet.GetDataSize());
            mainQueue().push(game_serverQuickUpdate, InvocationMessage(clientID, data));
        }
    }

    // ------ Main ------
    static NetServer& sharedInstance() {
        static NetServer shared;
        return shared;
    }
    NetServer() : port(0), udpListener(), tcpListener(), tcpSelector() { }

    void setup() {

        // Port
        port = GAME.port;
        if (port == 0) die("No port specified\n");

        // TCP
        if (!tcpListener.Listen(port)) {
            fprintf(stderr, "Could not listen on TCP port %d\n", port);
            abort();
        }
        printf("Server is listening on TCP port %d\n", port);
        tcpSelector.Add(tcpListener);

        // UDP
        if (!udpListener.Bind(port + 1)) {
            fprintf(stderr, "Could not bind on UDP port %d\n", port + 1);
            abort();
        }
        printf("Server bound to UDP port %d\n", port + 1);

        // Threads
        static sf::Thread udpThread(NetServer::readUDPThread);
        udpThread.Launch();

        static sf::Thread tcpThread(NetServer::readTCPThread);
        tcpThread.Launch();

        // Sending
        while (true) {
            sendToUDP();
            sendToTCP();
        }
    }
    static void mainThread(void* unused) {
        sharedInstance().setup();
    }


    // ------ UDP ------
    void readUDP() {
        while (true) {
            sf::Packet packet;
            unsigned short serverInPort = port + 1;
            sf::IPAddress address;

            if (udpListener.Receive(packet, address, serverInPort) != sf::Socket::Done)
                continue;

            sf::Lock lock(clientMutex);
            for (const Client& client : clients) {
                if (client.ip != address)
                    continue;

                serverReadPacket(client.clientID, packet, false);
                break;
            }
        }
    }
    static void readUDPThread(void* unused) { sharedInstance().readUDP(); }


    // ------ TCP ------
    void readTCP() {
        while (true) {
            int n = tcpSelector.Wait(0.01);
            for (unsigned i = 0; i < n; i++) {

                sf::SocketTCP socket = tcpSelector.GetSocketReady(i);

                if (socket == tcpListener) {
                    // It wants us to Listen for a new client
                    sf::IPAddress address;
                    sf::SocketTCP clientSocket;
                    tcpListener.Accept(clientSocket, &address);

                    Client* clientPtr = NULL;
                    
                    {
                        sf::Lock lock(clientMutex);
                        for (Client& c : clients) {
                            if (c.ip == address)
                                clientPtr = &c;
                        }
    
                        if (clientPtr && clientPtr->tcpSocket.IsValid()) {
                            tcpSelector.Remove(clientPtr->tcpSocket);
                            clientPtr->tcpSocket.Close();
                        }
    
                        if (!clientPtr) {
                            Client client = Client();
                            client.ip = address;
    
                            clients.push_back(client);
                            clientPtr = &(clients.back());
                        }
                    }

                    clientPtr->tcpSocket = clientSocket;

                    tcpSelector.Add(clientSocket);


                    // Send a PlayerID packet
                    static uint32_t nextClientPlayerID = 2;
                    sf::Packet playerIDPacket;

                    playerIDPacket << nextClientPlayerID;
                    clientPtr->clientID = nextClientPlayerID;

                    nextClientPlayerID++;
                    printf("Sending player id packet to %d\n", clientPtr->clientID);
                    clientSocket.Send(playerIDPacket);
                }
                else {
                    // It wants us to read from the given socket
                    sf::Packet packet;
                    sf::Socket::Status status = socket.Receive(packet);
                    if (status == sf::Socket::Done) {

                        // Read from the packet!
                        uint32_t clientID = 0;

                        sf::Lock lock(clientMutex);
                        for (Client& client : clients) {
                            if (client.tcpSocket != socket) continue;

                            serverReadPacket(client.clientID, packet, true);
                            break;
                        }
                    }
                    else if (status == sf::Socket::Disconnected || status == sf::Socket::Error) {
                        // Oops, error. Remove the socket.
                        tcpSelector.Remove(socket);
                        // remove_if(server.clients.begin(), server.clients.end(), socket);
                    }
                }
            }
        }
    }
    static void readTCPThread(void* unused) { sharedInstance().readTCP(); }
};
