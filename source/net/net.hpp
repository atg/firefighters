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

#import "net/serverio.hpp"
#import "net/clientio.hpp"

static void startNetworking() {
    
    static sf::Thread thread(GAME.isServer ? NetServer::mainThread : NetClient::mainThread);
    thread.Launch();
}


