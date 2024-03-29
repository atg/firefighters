#import "net/net.hpp"
#import "net/net_client.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "main/gamemode.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

// TODO: Unify this and clientReceiveGameState()
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
    
    if (player->isRespawning) {
        return; // Ignore this update if we're respawning but the client hasn't realized yet
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
    
    // Do this at the very end
    if (u.isfiringflamethrower())
        player->flamethrower.start(*player);
    else
        player->flamethrower.stop();
    // player->activeWeapon = &(player->flamethrower);
}
static void serverReceiveFullUpdate(std::string data, int clientID) {
    wire::ClientUpdate u;
    std::istringstream iss(std::string(&data[0], data.size()));
    u.ParseFromIstream(&iss);

    // Find a player with this ID
    Player* player = NULL;
    if (GAME.world.players.count(clientID)) {
        player = &(GAME.world.players[clientID]);
    }
    else {
        return;
    }
    
    if (u.has_confirmrespawned() && u.confirmrespawned()) {
        player->isRespawning = false;
    }
}

void game_serverQuickUpdate(InvocationMessage ctx) {
    // printf("Receive game state from %d\n", ctx.sender);
    serverReceiveGameState(ctx.data, ctx.sender);
}
void game_serverFullUpdate(InvocationMessage ctx) {
    serverReceiveFullUpdate(ctx.data, ctx.sender);
}

static void serializeTeam(wire::Team& out, Team& in) {
    out.set_tickets(in.tickets);
    for (int member : in.members) {
        out.add_members(member);
    }
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
            if (sus.empty() || sus.back().ByteSize() > 490) {
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
                    
                    printf("Will copy (%d, %d, %d) -> %d\n", vc_x, vc_y, worldIter->second.version, player.identifier);
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
        
        // Send game state
        // printf("NETSERV WILL SEND STATE? %d\n", GAME.state.hasChanged);
        // Keep a record of when we last sent this to this particular player
        if (GAME.state.hasChanged) {
            wire::Score score;
            wire::Team red;
            wire::Team blu;
            
            // printf("red state = %d %d\n", GAME.state.red.tickets, GAME.state.blu.tickets);
            serializeTeam(red, GAME.state.red);
            serializeTeam(blu, GAME.state.blu);
            // printf("team [state] = %d; %d\n", red.tickets(), blu.tickets());
            
            score.mutable_red()->CopyFrom(red);
            score.mutable_blu()->CopyFrom(blu);
            // printf("score [state] = %d; %d\n", score.red().tickets(), score.blu().tickets());
            
            std::map<int, wire::Score_MetaPlayer> metaplayers;
            
            for (auto pair : GAME.state.kills) {
                metaplayers[pair.first].set_kills(pair.second);
            }
            
            for (auto pair : GAME.state.deaths) {
                metaplayers[pair.first].set_deaths(pair.second);
            }
            
            for (auto& pair : GAME.world.players) {
                metaplayers[pair.first].set_health(pair.second.health);
                // This is where you would set other information such as weapon
            }
            
            for (auto& pair : metaplayers) {
                pair.second.set_identifier(pair.first);
                score.add_metaplayers()->CopyFrom(pair.second);
            }
            
            serveru.mutable_score()->CopyFrom(score);
            // printf("serveru [state] %d? = %d; %d\n", serveru.has_score(), serveru.score().red().tickets(), serveru.score().blu().tickets());
            modifiedSu = true;
            // GAME.state.hasChanged = false;
        }
        
        if (player.requiresNeedsRespawnNotification) {
            serveru.set_needsrespawn(true);
            player.requiresNeedsRespawnNotification = false;
            modifiedSu = true;
        }
        else {
            serveru.set_needsrespawn(false);
        }
        
        // Send the server update
        if (modifiedSu) {
            // printf("  PUSHING\n");
            tcpQueue().push(messageToPacket(&serveru), kv.first);
        }
    }
}
