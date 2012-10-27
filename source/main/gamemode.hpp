#import <set>
#import <map>
#import "util/util.hpp"

enum class GameMode {
    TeamDM,
    CTF,
};

struct Team {
    int tickets;
    std::set<int> members; // Player ids
    
    std::set<int, int> killCounts;
    std::set<int, int> deathCounts;
    
    /*
    struct CTFState {
        // A flag holder of 0 indicates the flag is in the base
        int flagHolder;
        Vec2<int, int> flagPosition; // If the flag was dropped
        
        // Need some kind of timeout too
        
        CTFState() : flagHolder(0) { }
    };
    Team::CTFState ctfState;
    */
    
    Team() : tickets(500) { }
};

enum GameState {
    GameMode mode;
    
    Team red;
    Team blu;
};


enum class CauseOfDeath {
    Stupidity = -10, // They killed themselves. What a plonker.
    OtherTeam = -9, // They were killed by the other team, but we don't know who did it
};

void playerDied_server(int killer, int dead);
// void playerDied_client(int killer, int dead);


