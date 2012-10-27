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
    
    return bits;
}
static bool is_solid(Tile tile) {
    static std::bitset<(int)Tile::LAST> bitset = solid_init();
    return bitset[(int)tile];
}
static Tile worldTileAtPixel(int px, int py) {
    if (px < 0 || py < 0) return Tile::Black; // Negative tiles, who needs 'em
    
    int chX = pixelToChunk(px);
    int chY = pixelToChunk(py);
    
    auto chunkIt = GAME.world.chunks.find(std::make_pair(chX, chY));
    if (chunkIt == GAME.world.chunks.end())
        return Tile::Black;

    Chunk& chunk = chunkIt->second;
    int tx = (px & 0b1111100000) >> 5;
    int ty = (py & 0b1111100000) >> 5;
    return chunk.tiles[tx][ty];
}
