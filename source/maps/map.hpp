/* Maps are built from *chunks* of different *tiles*. Think Minecraft.
   This makes procedural generation much easier, than a vector based approach.
   
   But the tiles just form the floor of the map. Maps can be elaborated by adding *objects* on top of the tiles.
   For example, there might be a piece of Road tile, and on top of it a Car tile.
*/
struct Tile {
    enum class Type {
        Dirt,
        Road,
    };
};

// struct Object {
//     Vec2<uint8_t> position;
//
//     enum class Type {
//         Hamster,
//     };
// };

// No idea what to make the chunk size, will need some experimentation
// Should be a power of 2 at least
// If we make the chunk size bigger than the screen size then it means the player can only see 2 chunk boundaries at one time
const int CHUNK_SIZE = 64;
struct Chunk {
    Tile tiles[CHUNK_SIZE][CHUNK_SIZE];
    
    // TODO: Maybe a quadtree would be better?
    // std::vector<Object> objects;
};

struct Map {
    
    // TODO: Maybe a quadtree would be better?
    std::map<Vec2<uint16_t>, Chunk> chunks;
};
