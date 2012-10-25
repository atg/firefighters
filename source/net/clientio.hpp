// #import <boost/asio.hpp>

// using UDP = boost::asio::ip::udp;

struct NetClient {
    sf::IPAddress serverIP;
    int port;
    
    int clientID;
    
    // std::unique_ptr<boost::asio::ip::udp::socket> udp;
    // std::unique_ptr<boost::asio::ip::udp::socket> tcp;
    
    sf::SocketUDP udp;
    sf::SocketUDP udpSender;
    sf::SocketTCP tcp;
    
    bool hasHandledFirstTCPPacket;
    
    
    // ------ Receiving ------
    void clientReadPacket(sf::Packet& packet, bool isTCP) {
        
        // Do something with the packet...
        printf("CLIENT READ PACKET: is TCP? %d, has handled first? %d\n", isTCP, hasHandledFirstTCPPacket);
        if (isTCP && !hasHandledFirstTCPPacket) {
            // This is our client ID, hopefully
            hasHandledFirstTCPPacket = true;
            
            uint32_t cid;
            packet >> cid;
            
            mainQueue().push(game_setClientID, InvocationMessage(cid, ""));
            return;
        }
        
        if (!isTCP) {
            const char* dataptr = packet.GetData();
            std::string data = std::string(dataptr, dataptr + packet.GetDataSize());
            mainQueue().push(game_clientQuickUpdate, InvocationMessage(0, data));
        }
    }
    
    
    // ------ Main ------
    static NetClient& sharedInstance() {
        static NetClient shared;
        return shared;
    }
    NetClient() : serverIP(), port(0), clientID(), udp(), udpSender(), tcp(), hasHandledFirstTCPPacket(false) { }
    
    void setup() {
        
        // IP and Port
        serverIP = GAME.serverIP;
        port = GAME.port;
        if (port == 0) die("No port specified");
        
        // TCP
        if (tcp.Connect(port, serverIP) != sf::Socket::Done) {
            fprintf(stderr, "Could not connect to TCP server %s on port %d\n", serverIP.ToString().c_str(), port);
            abort();
        }
        
        // UDP
        // boost::asio::io_service io_service;
        // boost::asio::ip::udp::resolver resolver(io_service);
        // boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4, serverIP.ToString(), "fffastpath");
        // boost::asio::ip::udp::endpoint endpoint = *resolver.resolve(query);
        // UDP::endpoint endpoint(UDP::v4(), port + 2);
        // UDP::socket socket(io_service, endpoint);
        
        // char data[2048] = { 0 };
        // socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint, 0, error);
        // socket.
        
        // socket.bind(endpoint);
        // socket.open();
        
        
        // udp.reset(new boost::asio::ip::udp::socket(io_service));
        // udp->open()
        // boost::system::error_code ec;
        
        
        if (!udp.Bind(port + 2)) {
            fprintf(stderr, "Could not bind on UDP port %d\n", port + 2);
            abort();
        }
        
        if (!udpSender.Bind(0)) {
            die("Could not bind UDP sender");
        }
        
        printf("Client bound to UDP port %d", port + 2);
        
        // Threads
        static sf::Thread udpThread(NetClient::readUDPThread);
        udpThread.Launch();
        
        static sf::Thread tcpThread(NetClient::readTCPThread);
        tcpThread.Launch();
        
        // Sending
        while (true) {
            sendUDP();
            sendTCP();
        }
    }
    static void mainThread(void* unused) {
        sharedInstance().setup();
    }
    
    
    // ------ Reading ------
    void sendTCP() {
        DataQueue& queue = tcpQueue();
        sf::Lock lock(queue.mutex);
        for (; !queue.q.empty(); queue.q.pop()) {
            DataQueue::Member& member = queue.q.back();
            printf("Send TCP\n");
            tcp.Send(member.packet);
        }
    }
    void sendUDP() {
        DataQueue& queue = udpQueue();
        sf::Lock lock(queue.mutex);
        for (; !queue.q.empty(); queue.q.pop()) {
            DataQueue::Member& member = queue.q.back();
            printf("Send UDP\n");
            udpSender.Send(member.packet.GetData(), member.packet.GetDataSize(), serverIP, port + 1);
        }
    }
    
    
    // ------ UDP ------
    void readUDP() {
        while (true) {
            sf::Packet packet;
            unsigned short port = port + 2;
            sf::IPAddress address;
            
            const size_t PACKET_SIZE = 4096;
            static char data[PACKET_SIZE + 1];
            memset(data, 0, PACKET_SIZE);
            
            size_t srec = 0;
            
            if (udp.Receive(data, PACKET_SIZE, srec, address, port) == sf::Socket::Done && srec > 0)
            // if (udp.Receive(packet, address, port) == sf::Socket::Done)
            {
                packet.Append(data, srec);
                clientReadPacket(packet, false);
            }
        }
    }
    static void readUDPThread(void* unused) { sharedInstance().readUDP(); }
    
    
    // ------ TCP ------
    void readTCP() {
        while (true) {
            sf::Packet packet;
            if (tcp.Receive(packet) == sf::Socket::Done) {
                clientReadPacket(packet, true);
            }
        }
    }
    static void readTCPThread(void* unused) { sharedInstance().readTCP(); }
};
