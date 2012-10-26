#import <map>
#import "util/util.hpp"

const int TILE_SIZE = 32; // In pixels
const int TILE_SIZE_LOG = 5;

const int CHUNK_SIZE = 32; // In tiles
const int CHUNK_SIZE_LOG = 5;

enum class CharacterClass : uint8_t {
    Flamethrower = 0,
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
    
    // Visited chunks (for the server)
    struct VisitedChunk {
        int x;
        int y;
        int version; // Chunks are versioned
        
        static VisitedChunk MakeVisitedChunk(int _x, int _y, int _v) {
            VisitedChunk xyv; xyv.x = _x; xyv.y = _x; xyv.version = _v; return xyv;
        }
    };
    std::map<std::pair<int, int>, VisitedChunk> visitedChunks;

    Player(Player::ID _identifier=0)
        : identifier(_identifier),
          position(),
          angle(0.0),
          team(0),
          cclass(CharacterClass::Flamethrower),
          health(Player::MaxHealth) { }
};

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
