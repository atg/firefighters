#import "net/net.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

static void clientReceiveGameState(std::vector<char>& data) {
    ServerQuickUpdate u;
    std::istringstream iss(std::string(&data[0], data.size()));
    if (!u.ParseFromIstream(&iss)) {
        fprintf(stderr, "Could not read quick update\n");
        abort();
    }
    
    // For each player
    for (const ServerQuickUpdate_PlayerUpdate& pu : u.updates()) {
        
        uint32_t clientID = pu.playerid();
        
        // Find a player with this ID
        Player* player = NULL;
        if (GAME.world.players.count(clientID)) {
            player = &(GAME.world.players[clientID]);
        }
        if (!player) {
            Player newPlayer;
            newPlayer.identifier = clientID;
            GAME.world.players[clientID] = newPlayer;
            player = &(GAME.world.players[clientID]);
        }
        
        player->position.x = pu.update().x();
        player->position.y = pu.update().y();
        player->angle.angle = pu.update().angle();
        player->angle.normalize();
        
        // TODO: Smooth movement
        // u.set_velocityx(0.0);
        // u.set_velocityy(0.0);
    }
}
void game_clientQuickUpdate(std::vector<char>* packet) {
    clientReceiveGameState(*packet);
}

static ClientQuickUpdate clientQuickUpdateFrom(const Player& player) {
    
    ClientQuickUpdate u;
    u.set_x(player.position.x);
    u.set_y(player.position.y);
    
    GAME.world.me->angle.normalize();
    u.set_angle(player.angle.angle);
    
    // TODO: Smooth movement
    u.set_velocityx(0.0);
    u.set_velocityy(0.0);
    return u;
}
static void clientSendGameState() {
    
    // Construct client game state
    ClientQuickUpdate u = clientQuickUpdateFrom(*(GAME.world.me));
    
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

