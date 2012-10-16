#import "net/net.hpp"
#import "world/map.hpp"
#import "main/game.hpp"
#import "wire.pb.h"
#import <sstream>
#import <string>

/*
Thread 0 Crashed:: Dispatch queue: com.apple.main-thread
0   hat.Firefighters              	0x000000010c216cd4 Angle::normalize() + 20
1   hat.Firefighters              	0x000000010c210c9e _ZL21clientQuickUpdateFromRK6Player + 94
2   hat.Firefighters              	0x000000010c20fd67 _ZL19serverSendGameStatev + 695
3   hat.Firefighters              	0x000000010c20f4d6 main + 150
4   hat.Firefighters              	0x000000010c1ad2e4 start + 52

Thread 2:
0   libsystem_kernel.dylib        	0x00007fff98600df2 __select + 10
1   com.sfml.network              	0x000000010c5ed6ee sf::SelectorBase::Wait(float) + 302
2   hat.Firefighters              	0x000000010c21fa04 sf::Selector<sf::SocketTCP>::Wait(float) + 84
3   hat.Firefighters              	0x000000010c211d7c _ZL19serverNetworkThreadPv + 1516
4   com.sfml.system               	0x000000010c6b4aae sf::Thread::ThreadFunc(void*) + 30
5   libsystem_c.dylib             	0x00007fff8d86c8bf _pthread_start + 335
6   libsystem_c.dylib             	0x00007fff8d86fb75 thread_start + 13
*/

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
void game_clientQuickUpdate(void* ctx) {
    auto arg = (std::vector<char>*)ctx;
    clientReceiveGameState(*arg);
    delete arg;
}

static ClientQuickUpdate clientQuickUpdateFrom(const Player& player) {
    
    ClientQuickUpdate u;
    u.set_x(player.position.x);
    u.set_y(player.position.y);
    
    Angle a = player.angle;
    a.normalize();
    u.set_angle(a.angle);
    
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

