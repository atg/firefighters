#import "main/game.hpp"
#import "world/map.hpp"

struct AABB {
    int x;
    int y;
    int w;
    int h;
    int z;
    
    AABB(int _x, int _y, int _w, int _h, int _z = 0) : x(_x), y(_y), w(_w), h(_h), z(_z) { }
    
    int maxx() { return x + w; }
    int maxy() { return y + h; }
};

static std::bitset<Tile::LAST> solid_init() {    
    std::bitset<Tile::LAST> bits;
    
    #define AFFIRM_SOLIDITY(tilename) \
      bits[(int)Tile::tilename] = 1;
      
    #define AFFIRM_SOLIDITY_VH(tilename) \
      bits[(int)Tile::tilename##V] = 1; \
      bits[(int)Tile::tilename##H] = 1;
    
    #define AFFIRM_SOLIDITY_NSEW(tilename) \
      bits[(int)Tile::tilename##N] = 1; \
      bits[(int)Tile::tilename##S] = 1; \
      bits[(int)Tile::tilename##E] = 1; \
      bits[(int)Tile::tilename##W] = 1;
    
    AFFIRM_SOLIDITY(BrickWall);
    AFFIRM_SOLIDITY_NSEW(DoorClosed);
    
    #undef AFFIRM_SOLIDITY
}
static bool is_solid(Tile tile) {
    static std::bitset<(int)Tile::LAST> bitset = solid_init();
    return bitset[(int)tile];
}


// Pixel <-> Tile <-> Chunk coordinate conversion
static int pixelToChunk(int px) {
    return px >> (TILE_SIZE_LOG + CHUNK_SIZE_LOG);
}
static int pixelToTile(int px) {
    return px >> (CHUNK_SIZE_LOG);
}
static int tileToPixel(int px) {
    return px << (CHUNK_SIZE_LOG);
}
static int tileToChunk(int px) {
    return px >> (TILE_SIZE_LOG);
}
static int chunkToPixel(int px) {
    return px << (TILE_SIZE_LOG + CHUNK_SIZE_LOG);
}
static int chunkToTile(int px) {
    return px << (TILE_SIZE_LOG);
}


static Tile worldTileAtPixel(int px, int py) {
    
    int chX = pixelToChunk(px);
    int chY = pixelToChunk(py);
    
    auto chunkIt = GAME.world.chunks.find(std::make_pair(chX, chY));
    if (chunkIt == GAME.world.chunks.end())
        return Tile::Black;
    
    Chunk& chunk = *chunkIt;
    return chunk.tiles[px - chunkToPixel(chX)][py - chunkToPixel(chY)];
}

// The meat of the operation
static bool collides(AABB first, AABB second) {
    
    // Check z index
    if (first.z != second.z) return false;
    
    // Check coordinates
    if (first.x >= second.maxx()) return false;
    if (first.y >= second.maxy()) return false;
    if (second.x >= first.maxx()) return false;
    if (second.y >= first.maxy()) return false;
    
    return true;
}

static bool collides(AABB first, int x, int y, int z = 0) {

    // Check z index
    if (first.z != z) return false;

    // Check coordinates
    if (x < first.x && x >= first.maxx()) return false;
    if (y < first.y && y >= first.maxy()) return false;

    return true;
}


