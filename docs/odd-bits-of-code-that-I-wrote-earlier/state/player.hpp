#import <map>
#import "util/util.hpp"
#import "maps/map.hpp"

enum class CharacterClass : uint8_t {
    Flamethrower = 0,
};

struct Player {
    
    // Each player needs an identifier so we can refer to them over the wire
    typedef ID uint16_t;
    Player::ID identifier;
    
    // Location
    Vec2<int> position;
    Angle angle;
    
    // Game
    uint8_t team; // In team games
    CharacterClass cclass;
    
    // Health
    uint32_t health; // Use use a big fat int for health. Flamethrowers do little bits of damage, slowly.
    const uint32_t MaxHealth = (uint32_t)-1;
    bool isAlive() { return health > 0; }
};

struct World {
    std::map<Player::ID, Player> players;
};
