#import "main/game.hpp"
#import "world/map.hpp"

// This is for the server only.
// Clients are dumb. We just give them the map and they display it. The server is tasked with generating, maintaining and sending to map to clients.

template<class T, int N>
struct Square {
    T* backing;
    Square(T val) : backing(new T[N * N]) {
        memset(backing, (int)val, N * N);
    }
    ~Square() { delete[] backing; }
    
    T& operator() (int x, int y) {
        return backing[x * N + y];
    }
};

void generate(World& world) {
    
    /* Basic strategy is:
        Take a field of grass
        Add motorways, main roads, side roads and alleyways
        Add buildings alongside the roads
        Add rivers
        Add bridges
        Add subway stations
        Add bus stops
        Add car parks
    */
    
    // It seems to be that it's easier to generate the whole map at once, and keep it in memory, than to try to generate it as we go along. Say we have ~4 bytes per tile, then 512MB of memory gets us a 11585x11585 map
    
    // Start with a field of grass
    const int size = 256;
    Square<Tile, size> tiles(Tile::Grass);
    
    // Make a road
    // 13 pixels of road, 25 pixels of space
    const int roadmodulus = (13 + 25);
    for (int i = 0; i < size / roadmodulus; i++) {
        for (int j = 0; j < size; j++) {
            int x = i * roadmodulus;
            
            tiles(x +  0, j) = Tile::Pavement;
            tiles(x +  1, j) = Tile::Pavement;
            tiles(x +  2, j) = Tile::Tarmac;
            tiles(x +  3, j) = Tile::Tarmac;
            tiles(x +  4, j) = Tile::Tarmac;
            tiles(x +  5, j) = Tile::Tarmac;
            tiles(x +  6, j) = Tile::RoadCenterLine;
            tiles(x +  7, j) = Tile::Tarmac;
            tiles(x +  8, j) = Tile::Tarmac;
            tiles(x +  9, j) = Tile::Tarmac;
            tiles(x + 10, j) = Tile::Tarmac;
            tiles(x + 11, j) = Tile::Pavement;
            tiles(x + 12, j) = Tile::Pavement;
        }
    }
    
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size / roadmodulus; j++) {
            int y = j * roadmodulus;
            
            if (tiles(i, y + 0) != Tile::RoadCenterLine && tiles(i, y + 0) != Tile::Tarmac)
                tiles(i, y +  0) = Tile::Pavement;
            if (tiles(i, y + 1) != Tile::RoadCenterLine && tiles(i, y + 1) != Tile::Tarmac)
                tiles(i, y +  1) = Tile::Pavement;
            
            for (int a = 2; a <= 10; a++) {
                if (a == y) continue;
                if (tiles(i, y + a) == Tile::RoadCenterLine) continue;
                
                tiles(i, y + a) = Tile::Tarmac;
            }
            
            tiles(i, y +  6) = Tile::RoadCenterLine;
            
            if (tiles(i, y + 1) != Tile::RoadCenterLine && tiles(i, y + 11) != Tile::Tarmac)
                tiles(i, y + 11) = Tile::Pavement;
            if (tiles(i, y + 1) != Tile::RoadCenterLine && tiles(i, y + 12) != Tile::Tarmac)
                tiles(i, y + 12) = Tile::Pavement;
        }
    }
    
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            Tile above = j == 0    ? Tile::Black : tiles(i, j - 1);
            Tile below = j == size ? Tile::Black : tiles(i, j + 1);
            Tile  left = i == 0    ? Tile::Black : tiles(i - 1, j);
            Tile right = i == size ? Tile::Black : tiles(i + 1, j);
            
            Tile here = tiles(i, j);
            
            // Walls around pavements
            if (here == Tile::Grass && (above == Tile::Pavement || below == Tile::Pavement || left == Tile::Pavement || right == Tile::Pavement))
                tiles(i, j) = Tile::BrickWall;
        }
    }
    
    // Chunkify
    for (int i = 0; i < size / CHUNK_SIZE; i++) {
        for (int j = 0; j < size / CHUNK_SIZE; j++) {
            Chunk chunk;
            chunk.version = 0;
            for (int x = 0; x < CHUNK_SIZE; x++) {
                // TODO: turn this into a memcpy
                for (int y = 0; y < CHUNK_SIZE; y++) {
                    chunk.tiles[x][y] = tiles(i * CHUNK_SIZE + x, j * CHUNK_SIZE + y);
                }
            }
            
            world.chunks[std::make_pair(i, j)] = chunk;
        }
    }
    
    // Print out the result
    // for (int x = 0; x < size; x++) {
    //     for (int y = 0; y < size; y++) {
    //         char c = '?';
    //         switch (tiles(x, y)) {
    //             case Tile::Dirt: c = 'D'; break;
    //             case Tile::Grass: c = '.'; break;
    //             case Tile::Tarmac: c = '%'; break;
    //             case Tile::Pavement: c = '\''; break;
    //             case Tile::RoadCenterLine: c = '+'; break;
    //
    //             default: ;
    //         }
    //
    //         putchar(c);
    //     }
    //     putchar('\n');
    // }
}
