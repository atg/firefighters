#import "net/net.hpp"
#import "net/net_client.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

static void serverReceiveGameState(std::string data, uint32_t clientID) {
    
    ClientQuickUpdate u;
    std::istringstream iss(std::string(&data[0], data.size()));
    u.ParseFromIstream(&iss);
    
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
    
    player->position.x = u.x();
    player->position.y = u.y();
    player->angle.angle = u.angle();
    player->angle.normalize();
    
    // TODO: Smooth movement
    // u.set_velocityx(0.0);
    // u.set_velocityy(0.0);
}
void game_serverQuickUpdate(void* ctx) {
    auto arg = (std::pair<std::string, uint32_t>*)ctx;
    printf("Receive game state\n");
    serverReceiveGameState(arg->first, arg->second);
    delete arg;
}
static sf::Packet messageToPacket(const google::protobuf::Message* msg) {
    
    // Write to a std::string
    std::ostringstream oss;
    if (!msg->SerializeToOstream(&oss)) {
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
    return packet;
}
static void serverSendGameState() {
    
    // For each client
    for (std::pair<uint32_t, Player> kv : GAME.world.players) {
        Player& player = GAME.world.players[kv.first];
        
        std::vector<ServerQuickUpdate_PlayerUpdate> playerUpdates;
        // Determine near players to send
        for (std::pair<uint32_t, Player> otherKV : GAME.world.players) {
            // Ignore this player
            if (otherKV.first == kv.first)
                continue;
            
            Player& otherPlayer = GAME.world.players[otherKV.first];
            
            // If this player is too far away from the player, ignore them
            if (player.position.distance(otherPlayer.position) > TILE_SIZE * CHUNK_SIZE * 2)
                continue;
            
            // Otherwise add them
            ServerQuickUpdate_PlayerUpdate pu;
            pu.set_playerid(otherPlayer.identifier);
            pu.mutable_update()->CopyFrom(clientQuickUpdateFrom(otherPlayer));
            
            playerUpdates.push_back(pu);
        }
        
        
        std::vector<ServerQuickUpdate> sus;
        for (int i = 0; i < playerUpdates.size(); i++) {
            if (sus.empty() || sus.back().ByteSize() > 500) {
                sus.push_back(ServerQuickUpdate());
            }
            
            sus.back().add_updates()->CopyFrom(playerUpdates[i]);
        }
        
        for (const ServerQuickUpdate& su : sus) {
            udpQueue().push(messageToPacket(&su), kv.first);
        }
    }
    
/*
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
*/
}

