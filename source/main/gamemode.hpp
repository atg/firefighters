#import <set>
#import <map>
#import "util/util.hpp"

struct Player;

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

struct GameState {
    bool hasChanged;
    GameMode mode;
    
    Team red;
    Team blu;
    
    std::map<int, int> kills;
    std::map<int, int> deaths;
    
    GameState() : hasChanged(false), mode(GameMode::TeamDM), red(), blu() { }
};

enum class CauseOfDeath {
    Stupidity = -10, // They killed themselves. What a plonker.
    OtherTeam = -9, // They were killed by the other team, but we don't know who did it
    PastPlayer = -8, // They were killed by someone who has now quit
};

bool isRedPlayer(int id);
bool isBluPlayer(int id);

void playerDamaged(int assaulter, int victim, int amount);
void playerDied(int killer, Player& dead);

void gameOver();
