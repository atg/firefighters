#import "net/net.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

static void clientReceiveGameState(const std::string& data) {
    wire::ServerQuickUpdate u;
    // printf("Message is: %s\n", data.c_str());
    std::istringstream iss(data);
    if (!u.ParseFromIstream(&iss)) {
        coma("Could not read quick update.");
        return;
    }
    
    // For each player
    for (const wire::ServerQuickUpdate_PlayerUpdate& pu : u.updates()) {
        
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
        // player->angle = Angle::FromWire(pu.update().angle());
        player->angle.angle = pu.update().angle();
        
        // TODO: Smooth movement
        // u.set_velocityx(0.0);
        // u.set_velocityy(0.0);
    }
}
void game_clientQuickUpdate(InvocationMessage ctx) {
    clientReceiveGameState(ctx.data);
}

static wire::ClientQuickUpdate clientQuickUpdateFrom(const Player& player) {
    
    wire::ClientQuickUpdate u;
    u.set_x(round(player.position.x));
    u.set_y(round(player.position.y));
    
    Angle a = player.angle;
    a.normalize();
    // u.set_angle(a.wireRepr());
    u.set_angle(a.angle);
    
    // TODO: Smooth movement
    u.set_velocityx(0);
    u.set_velocityy(0);
    return u;
}
static void clientSendGameState() {
    
    // Construct client game state
    wire::ClientQuickUpdate u = clientQuickUpdateFrom(*(GAME.world.me));
    
    // Write to a std::string
    std::ostringstream oss;
    if (!u.SerializeToOstream(&oss))
        die("Could not serialize quick update to ostream");
    
    std::string s = oss.str();
    if (!s.size())
        die("No data when serializing quick update to string\n");
    
    printf("Sending %d bytes\n", (int)(s.size()));
    sf::Packet packet;
    packet.Append(&s[0], s.size());
    
    udpQueue().push(packet, GAME.clientID);
}
