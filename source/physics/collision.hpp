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


