#import <map>
#import "util/util.hpp"

const int TILE_SIZE = 24; // In pixels

enum class CharacterClass : uint8_t {
    Flamethrower = 0,
};

struct Player {
    
    // Each player needs an identifier so we can refer to them over the wire
    typedef uint16_t ID;
    Player::ID identifier;
    
    // Location
    Vec2<double> position;
    Angle angle;
    
    // Game
    uint8_t team; // In team games
    CharacterClass cclass;
    
    // Health
    uint32_t health; // Use use a big fat int for health. Flamethrowers do little bits of damage, slowly.
    static const uint32_t MaxHealth = (uint32_t)-1;
    bool isAlive() { return health > 0; }
    
    Player(Player::ID _identifier=0)
        : identifier(_identifier),
          position(),
          angle(0.0),
          team(0),
          cclass(CharacterClass::Flamethrower),
          health(Player::MaxHealth) { }
};

struct World {
    Player* me; // Client only
    std::map<Player::ID, Player> players;
};
