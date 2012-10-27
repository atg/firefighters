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
static void playerDied(int killer, int dead) {
    
}


