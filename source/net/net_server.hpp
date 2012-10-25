#import "net/net.hpp"
#import "net/net_client.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

static void serverReceiveGameState(std::string data, uint32_t clientID) {
    wire::ClientQuickUpdate u;
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
    // player->angle = Angle::FromWire(u.angle());
    player->angle.angle = u.angle();

    printf("Player %d\n", (int)clientID);
    printf("  x = %d\n", (int)(u.x()));
    printf("  y = %d\n", (int)(u.y()));
    printf(" th = %d\n", player->angle.degrees());
    
    // TODO: Smooth movement
    // u.set_velocityx(0.0);
    // u.set_velocityy(0.0);
}
void game_serverQuickUpdate(InvocationMessage ctx) {
    printf("Receive game state from %d\n", ctx.sender);
    serverReceiveGameState(ctx.data, ctx.sender);
}
static sf::Packet messageToPacket(const google::protobuf::Message* msg) {
    
    // Write to a std::string
    std::ostringstream oss;
    if (!msg->SerializeToOstream(&oss))
        die("Could not serialize quick update to ostream");
    
    std::string s = oss.str();
    if (!s.size())
        die("No data when serializing quick update to string");
    
    printf("Sending %d bytes\n", (int)(s.size()));
    sf::Packet packet;
    packet.Append(&s[0], s.size());
    return packet;
}
static void serverSendGameState() {
    
    // For each client
    for (std::pair<uint32_t, Player> kv : GAME.world.players) {
        Player& player = GAME.world.players[kv.first];
        
        std::vector<wire::ServerQuickUpdate_PlayerUpdate> playerUpdates;
        // Determine near players to send
        for (std::pair<uint32_t, Player> otherKV : GAME.world.players) {
            // Ignore this player
            if (otherKV.first == kv.first)
                continue;
            
            Player& otherPlayer = GAME.world.players[otherKV.first];
            
            // If this player is too far away from the player, ignore them
            // TODO: Replace this with an actual maximal chunk distance calculation
            if (player.position.distance(otherPlayer.position) > TILE_SIZE * CHUNK_SIZE * 2)
                continue;
            
            // Otherwise add them
            wire::ServerQuickUpdate_PlayerUpdate pu;
            pu.set_playerid(otherPlayer.identifier);
            pu.mutable_update()->CopyFrom(clientQuickUpdateFrom(otherPlayer));
            
            playerUpdates.push_back(pu);
        }
        
        
        std::vector<wire::ServerQuickUpdate> sus;
        for (int i = 0; i < playerUpdates.size(); i++) {
            if (sus.empty() || sus.back().ByteSize() > 500) {
                sus.push_back(wire::ServerQuickUpdate());
            }
            
            sus.back().add_updates()->CopyFrom(playerUpdates[i]);
        }
        
        for (const wire::ServerQuickUpdate& su : sus) {
            udpQueue().push(messageToPacket(&su), kv.first);
        }
    }
}
