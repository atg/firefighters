#import "net/net.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

static void clientReceiveGameState() {
    
}
static void clientSendGameState() {
    
    // Construct client game state
    ClientQuickUpdate u;
    u.set_x(GAME.world.me->position.x);
    u.set_y(GAME.world.me->position.y);
    
    GAME.world.me->angle.normalize();
    u.set_angle(GAME.world.me->angle.angle);
    
    // TODO: Smooth movement
    u.set_velocityx(0.0);
    u.set_velocityy(0.0);
    
    // Write to a std::string
    std::ostringstream oss;
    if (!u.SerializeToOstream(&oss)) {
        fprintf(stderr, "Could not serialize quick update to ostream\n");
        abort();
    }
    std::string s = oss.str();
    if (!s.size()) {
        fprintf(stderr, "No data when serializing quick update to string\n");
        abort();
    }
    
    printf("Sending %d bytes\n", (int)(s.size()));
    sf::Packet packet;
    packet.Append(&s[0], s.size());
    
    udpQueue().push(packet, GAME.clientID);
}

