#import "net/net.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "main/gamemode.hpp"
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
        
        // Do this at the very end
        if (pu.update().isfiringflamethrower())
            player->flamethrower.start(*player);
        else
            player->flamethrower.stop();
        // player->activeWeapon = &(player->flamethrower);
    }
}
static void clientReceiveFullUpdate(const std::string& data) {
    wire::ServerUpdate u;
    printf("Message is: %s\n", data.c_str());
    std::istringstream iss(data);
    if (!u.ParseFromIstream(&iss)) {
        coma("Could not read quick update.");
        return;
    }

    // For each chunk
    printf("Receiving update where chunks would be set theoretically\n");
    for (const wire::Chunk& chunku : u.chunks()) {
        int x = chunku.x();
        int y = chunku.y();
        int version = chunku.version();
        printf("\t Receiving chunks: %d / %d / %d\n", x, y, version);
        
        // Find a player with this ID
        Player* player = NULL;
        auto worldIter = GAME.world.chunks.find(std::make_pair(x, y));
        if (worldIter == GAME.world.chunks.end()) {
            Chunk chunk;
            GAME.world.chunks[std::make_pair(x, y)] = chunk;
            
            worldIter = GAME.world.chunks.find(std::make_pair(x, y));
        }
        
        worldIter->second.version = version;
        std::copy(chunku.tiles().begin(), chunku.tiles().end(), (char*)&worldIter->second.tiles[0][0]);
    }
    
    // Game state
    if (u.has_score()) {
        
        // Tickets
        GAME.state.red.tickets = u.score().red().tickets();
        GAME.state.blu.tickets = u.score().blu().tickets();
        
        printf("TICKETS! %d / %d", GAME.state.red.tickets, GAME.state.blu.tickets);
        
        // Members
        GAME.state.red.members.clear();
        GAME.state.blu.members.clear();
        
        for (int member : u.score().red().members()) {
            GAME.state.red.members.insert(member);
        }
        for (int member : u.score().red().members()) {
            GAME.state.blu.members.insert(member);
        }
        
        // Player Metadata
        for (const wire::Score_MetaPlayer& metaplayer : u.score().metaplayers()) {
            int id = metaplayer.identifier();
            
            if (metaplayer.has_kills())
                GAME.state.kills[id] = metaplayer.kills();
            if (metaplayer.has_deaths())
                GAME.state.deaths[id] = metaplayer.deaths();
            
            if (metaplayer.has_health()) {
                if (GAME.world.players.find(id) != GAME.world.players.end()) {
                    GAME.world.players[id].health = metaplayer.health();
                }
            }
        }
    }
}
void game_clientQuickUpdate(InvocationMessage ctx) {
    clientReceiveGameState(ctx.data);
}
void game_clientFullUpdate(InvocationMessage ctx) {
    printf("#### FULL UPDATE #### \n");
    clientReceiveFullUpdate(ctx.data);
}

static wire::ClientQuickUpdate clientQuickUpdateFrom(Player& player) {
    
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
    
    if (player.activeWeapon() == &(player.flamethrower))
        u.set_isfiringflamethrower(player.flamethrower.isFiring);
    
    return u;
}
static void clientSendGameState() {
    
    // Construct client game state
    wire::ClientQuickUpdate u = clientQuickUpdateFrom(*(GAME.world.me));
    u.set_viewportx(GAME.viewportX);
    u.set_viewporty(GAME.viewportY);
    u.set_viewportwidth(GAME.viewportWidth);
    u.set_viewportheight(GAME.viewportHeight);
    
    // Write to a std::string
    std::ostringstream oss;
    if (!u.SerializeToOstream(&oss))
        die("Could not serialize quick update to ostream");
    
    std::string s = oss.str();
    if (!s.size())
        die("No data when serializing quick update to string\n");
    
    // printf("Sending %d bytes\n", (int)(s.size()));
    sf::Packet packet;
    packet.Append(&s[0], s.size());
    
    udpQueue().push(packet, GAME.clientID);
}
