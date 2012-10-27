#import "net/net.hpp"
#import "net/net_client.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

static void serverReceiveGameState(std::string data, int clientID) {
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
    
    if (!u.has_viewportx() || !u.has_viewporty() || !u.has_viewportwidth() || !u.has_viewportheight())
        die("Player did not send viewport information"); // Should probably kick the player instead of crashing the server
    
    player->position.x = u.x();
    player->position.y = u.y();
    // player->angle = Angle::FromWire(u.angle());
    player->angle.angle = u.angle();
    
    player->viewportX = u.viewportx();
    player->viewportY = u.viewporty();
    player->viewportWidth = u.viewportwidth();
    player->viewportHeight = u.viewportheight();
    
    // printf("Player %d\n", (int)clientID);
    // printf("  x = %d\n", (int)(u.x()));
    // printf("  y = %d\n", (int)(u.y()));
    // printf(" th = %d\n", player->angle.degrees());
    
    // TODO: Smooth movement
    // u.set_velocityx(0.0);
    // u.set_velocityy(0.0);
}
void game_serverQuickUpdate(InvocationMessage ctx) {
    // printf("Receive game state from %d\n", ctx.sender);
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
    
    // printf("Sending %d bytes\n", (int)(s.size()));
    sf::Packet packet;
    packet.Append(&s[0], s.size());
    return packet;
}
static void serverSendGameState() {
    
    // For each client
    for (std::pair<const int, Player>& kv : GAME.world.players) {
        Player& player = GAME.world.players[kv.first];
        
        std::vector<wire::ServerQuickUpdate_PlayerUpdate> playerUpdates;
        // Determine near players to send
        for (std::pair<const int, Player>& otherKV : GAME.world.players) {
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
        
        
        // Split the update into bitesized packets under 500 bytes
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
        
        bool modifiedSu = false;
        wire::ServerUpdate serveru;
        
        // Find out if they need new chunks around their neighbourhood
        
        // Do a simple neighbourhood calculation
        int chunk_tile_lg = CHUNK_SIZE_LOG + TILE_SIZE_LOG;
        int neigh_min_x = (player.viewportX) >> chunk_tile_lg;
        int neigh_min_y = (player.viewportY) >> chunk_tile_lg;
        int neigh_max_x = (player.viewportX + player.viewportWidth) >> chunk_tile_lg;
        int neigh_max_y = (player.viewportY + player.viewportHeight) >> chunk_tile_lg;
        
        --neigh_min_x;
        --neigh_min_y;
        ++neigh_max_x;
        ++neigh_max_y;
        
        for (int vc_x = neigh_min_x; vc_x <= neigh_max_x; vc_x++) {
            for (int vc_y = neigh_min_y; vc_y <= neigh_max_y; vc_y++) {
                
                auto worldIter = GAME.world.chunks.find(std::make_pair(vc_x, vc_y));
                if (worldIter == GAME.world.chunks.end()) continue;
                
                auto visitedIter = player.visitedChunks.find(std::make_pair(vc_x, vc_y));
                if (visitedIter == player.visitedChunks.end() || visitedIter->second.version < worldIter->second.version) {
                    
                    Tile* tilesPtr = &(worldIter->second.tiles[0][0]);
                    
                    wire::Chunk chunku;
                    chunku.set_x(vc_x);
                    chunku.set_y(vc_y);
                    chunku.set_version(worldIter->second.version);
                    chunku.set_tiles(std::string(
                        reinterpret_cast<char*>(tilesPtr),
                        reinterpret_cast<char*>(tilesPtr + CHUNK_SIZE * CHUNK_SIZE)
                    )); // TODO: Endianness! Should convert to little endian if this is big endian
                    chunku.set_metadata(std::string());
                    
                    printf("Will copy (%d, %d)\n", vc_x, vc_y);
                    wire::Chunk* chunkPtr = serveru.add_chunks();
                    printf("  did add\n");
                    chunkPtr->CopyFrom(chunku);
                    printf("  copy fromed\n");
                    
                    modifiedSu = true;
                    
                    if (visitedIter != player.visitedChunks.end())
                        visitedIter->second.version = worldIter->second.version;
                    else
                        player.visitedChunks[std::make_pair(vc_x, vc_y)] = Player::VisitedChunk::Make(vc_x, vc_y, worldIter->second.version);
                }
            }
        }
        
        // Send the server update
        if (modifiedSu) {
            tcpQueue().push(messageToPacket(&serveru), kv.first);
        }
    }
}
