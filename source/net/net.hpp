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
        int clientID;
    };
    std::queue<DataQueue::Member> q;

    void push(sf::Packet packet, int clientID) {
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

#import "wire.pb.h"
#import <sstream>
#import <string>
static sf::Packet messageToPacket(const google::protobuf::Message* msg) {

    // Write to a std::string
    std::ostringstream oss;
    if (!msg->SerializeToOstream(&oss))
        die("Could not serialize quick update to ostream");

    std::string s = oss.str();
    if (!s.size())
        die("No data when serializing quick update to string");

    // printf("Sending %d bytes\n", (int)(s.size()));
    sf::Packet packet;
    packet.Append(&s[0], s.size());
    return packet;
}
