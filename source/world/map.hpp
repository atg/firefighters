#import <map>
#import <vector>
#import "util/util.hpp"
#import "physics/particle.hpp"

const int TILE_SIZE = 32; // In pixels
const int TILE_SIZE_LOG = 5;

const int CHUNK_SIZE = 32; // In tiles
const int CHUNK_SIZE_LOG = 5;

enum class CharacterClass : uint8_t {
    Flamethrower = 0,
};

struct Player;

struct Weapon {
    enum class Type {
        Flamethrower,
    };
    
    Type type;
    bool isFiring;
    
    // Ideally we would use std::unique_ptr and allocate as needed.
    // That's not an option until Micah can get a new computer that supports Clang 3.1
    Emitter _emitter;
    bool _hasEmitter;
    bool hasEmitter() { return _hasEmitter; }
    
    void start(Player& player);
    void stop() {
        if (!isFiring) return;
        isFiring = false;
        
        _emitter = Emitter();
        _hasEmitter = false;
    }
    
    Weapon() : type(Weapon::Type::Flamethrower), _emitter(), _hasEmitter(false) { }
};
struct Player {
    
    // Each player needs an identifier so we can refer to them over the wire
    typedef int ID;
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
    
    // Weapons
    std::vector<Weapon> weapons;
    
    // Visited chunks (for the server)
    struct VisitedChunk {
        int x;
        int y;
        int version; // Chunks are versioned
        
        static VisitedChunk Make(int _x, int _y, int _v) {
            VisitedChunk xyv; xyv.x = _x; xyv.y = _x; xyv.version = _v; return xyv;
        }
    };
    std::map<std::pair<int, int>, VisitedChunk> visitedChunks;
    
    int viewportX;
    int viewportY;
    int viewportWidth;
    int viewportHeight;
    
    Player(Player::ID _identifier=0)
        : identifier(_identifier),
          position(),
          angle(0.0),
          team(0),
          cclass(CharacterClass::Flamethrower),
          health(Player::MaxHealth),
          viewportX(0), viewportY(0), viewportWidth(0), viewportHeight(0) { }
};

void Weapon::start(Player& player) {
    if (isFiring) {
        _emitter.position = Vec2<int>(round(player.position.x), round(player.position.y));
        _emitter.update();
        return;
    }
    
    isFiring = true;
    
    // TODO: 0.942 radians is just a random angle, change it to something better
    _emitter = Emitter(player.angle, Angle(0.942), 10, 100);
    _emitter.position = Vec2<int>(round(player.position.x), round(player.position.y));
    _hasEmitter = true;
    
    _emitter.begin();
}
// static void updatedPlayerPosition(Player& player) {
    // Not needed now

    // Update the emitter positions for each weapon
    /*
    for (Weapon& weapon : player.weapons) {
        if (weapon.hasEmitter()) {
            weapon._emitter.position.x = round(player.position.x);
            weapon._emitter.position.y = round(player.position.y);
        }
    }
    */
// }

// Some tiles are rotationally symmetric in C2 or C4
// Vertical or horizontal
#define VH(name) name ## V, name ## H
// Rotated
#define NSEW(name) name ## N, name ## S, name ## E, name ## W

enum class Tile : uint8_t {
    Black = 0,
    Dirt,
    Grass,
    Tarmac,
    Pavement,
    RoadCenterLine,
    
    NSEW(Door),
};

struct Chunk {
    // Chunks are versioned. If a chunk changes, then its version is increased (by the server ONLY!) so that other clients know to refresh it
    int version;
    Tile tiles[CHUNK_SIZE][CHUNK_SIZE];
};

struct World {
    Player* me; // Client only
    std::map<Player::ID, Player> players;
    
    std::map<std::pair<int, int>, Chunk> chunks;
};
