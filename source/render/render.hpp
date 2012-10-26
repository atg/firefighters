#import "main/game.hpp"
#import <SFML/Graphics.hpp>

static void drawCrosshair() {
    const int size = 4;
    int mouseX = GAME.mouseX;
    int mouseY = GAME.mouseY;
    
    sf::Shape crosshairV = sf::Shape::Line(mouseX, mouseY - size - 1, mouseX, mouseY + size, 1, sf::Color(255, 255, 255, 255));
    GAME.app->Draw(crosshairV);
    
    sf::Shape crosshairH = sf::Shape::Line(mouseX - size - 1, mouseY, mouseX + size, mouseY, 1, sf::Color(255, 255, 255, 255));
    GAME.app->Draw(crosshairH);
}

static void drawPlayer(Player& player, bool isUser) {
    // First of all, is the player IN our viewport?
    // TODO: Check its axis-aligned bounding box
    
    double width = TILE_SIZE * 5.0 / 3.0;
    double height = TILE_SIZE * 2.0 / 3.0;
    sf::Color color = isUser ? sf::Color(255, 255, 124, 255) : sf::Color(255, 124, 124, 255);
    sf::Shape box = sf::Shape::Rectangle(0, 0, width, height, color);
    box.SetPosition(player.position.x, player.position.y);
    box.SetCenter(width / 2, height / 2);
    box.SetRotation(360.0 - player.angle.angle * 180.0 / M_PI + 90.0);
    
    GAME.app->Draw(box);
}
static void drawAllPlayers() {
    for (std::pair<Player::ID, Player> pair : GAME.world.players) {
        // Don't draw THE player
        Player& player = GAME.world.players[pair.first];
        if (&player == GAME.world.me)
            continue;
        
        drawPlayer(player, false);
    }
    
    drawPlayer(*GAME.world.me, true);
}

static sf::Color colorForTile(Tile tile) {
    switch (tile){
        case Tile::Black: return sf::Color(0, 0, 0, 255);
        case Tile::Dirt: return sf::Color(128, 64, 0, 255);
        case Tile::Grass: return sf::Color(0, 255, 0, 255);
        case Tile::Tarmac: return sf::Color(102, 102, 102, 255);
        case Tile::Pavement: return sf::Color(192, 192, 192, 255);
        case Tile::RoadCenterLine: return sf::Color(255, 255, 0, 255);
        case Tile::DoorN:
        case Tile::DoorS:
        case Tile::DoorE:
        case Tile::DoorW:
          return sf::Color(128, 128, 0, 255);
    }
}
static void drawTile(int xpx, int ypx, Tile tile) {
    sf::Shape box = sf::Shape::Rectangle(xpx, ypx, TILE_SIZE, TILE_SIZE, colorForTile(tile));
    GAME.app->Draw(box);
}

struct ViewportRect {
    int minx;
    int maxx;
    int miny;
    int maxy;
};
static void drawChunk(int cx, int cy, ViewportRect vr, Chunk& chunk) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
        int xcoord = (cx * CHUNK_SIZE + i) * TILE_SIZE;
        if (xcoord + TILE_SIZE < vr.minx) continue;
        if (xcoord > vr.maxx) break;
        
        for (int j = 0; j < CHUNK_SIZE; j++) {
            int ycoord = (cy * CHUNK_SIZE + j) * TILE_SIZE;
            
            if (ycoord + TILE_SIZE < vr.miny) continue;
            if (ycoord > vr.maxy) break;
            
            drawTile(xcoord, ycoord, chunk.tiles[i][j]);
        }
    }
}

static void render() {
    // Get the mouse position, and draw a crosshair
    drawCrosshair();
    
    // Draw the players
    drawAllPlayers();
    
    // Draw chunks
    ViewportRect vr = { GAME.viewportX, GAME.viewportX + GAME.viewportWidth, GAME.viewportY, GAME.viewportY + GAME.viewportHeight };
    
    int chunkMinX = vr.minx >> (CHUNK_SIZE_LOG - 1);
    int chunkMaxX = (vr.maxx >> (CHUNK_SIZE_LOG - 1)) + 1;
    
    int chunkMinY = vr.miny >> (CHUNK_SIZE_LOG - 1);
    int chunkMaxY = (vr.maxy >> (CHUNK_SIZE_LOG - 1)) + 1;
    
    auto& chunks = GAME.world.chunks;
    for (int x = chunkMinX; x <= chunkMaxX; x++) {
        for (int y = chunkMinX; y <= chunkMaxX; y++) {
            if (chunks.count(std::make_pair(x, y))) {
                drawChunk(x, y, vr, chunks[std::make_pair(x, y)]);
            }
        }
    }
}

