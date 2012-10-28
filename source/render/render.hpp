#import "main/game.hpp"
#import "physics/collision.hpp"
#import "physics/particle.hpp"
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

static void drawEmitter(Emitter& emitter) {
    // Draw each particle
    // printf("emitter.particles = %d\n", (int)(emitter.particles.size()));
    
    /*
    // Loop through particles, find minX, minY, maxX, maxY
    // Create array of that size, memset to 0
    // Set colors
    // Create image
    sf::Image img = sf::Image::LoadFromPixels(width, height, data);
    // Draw
    */
    
    for (Particle& p : emitter.particles) {
        if (p.dead) continue;
        
        // White  255 255 255
        // Yellow 255 255 0
        // Orange 255 128 0
        // Red    255 0   0
        // Brown  128 0   0
        
        int red = 255;
        int green = 255;
        int blue = 255;
        
        float percentage = p.age;
        if (percentage > 0.995)
            percentage = 0.995;
        
        float intpart = floorf(percentage * 4.0);
        float fracpart = percentage * 4.0 - intpart;
        
        if (intpart == 0) blue *= 1.0 - fracpart;
        else blue = 0;
        
        if (intpart == 1) green *= (1.0 - fracpart) * 0.5 + 0.5;
        else if (intpart == 2) green *= (1.0 - fracpart) * 0.5;
        else if (intpart >= 3) green = 0;
        
        if (intpart == 3) red *= (1.0 - fracpart) * 0.5 + 0.5;
        
        // printf("Draw particle: %f, %f\n", p.position.x, p.position.y);
        sf::Shape shape = sf::Shape::Line(p.position.x, p.position.y, p.position.x + 1, p.position.y + 1, 1.0, sf::Color(red, green, blue, 255));
        GAME.app->Draw(shape);
    }
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
    
    // Draw emitters on top
    for (std::pair<Player::ID, Player> pair : GAME.world.players) {
        Player& player = GAME.world.players[pair.first];
        if (player.activeWeapon()) {
            if (player.activeWeapon()->isFiring) {
                drawEmitter(player.activeWeapon()->_emitter);
            }
        }
    }
}

static sf::Color colorForTile(Tile tile) {
    switch (tile) {
        case Tile::LAST: case Tile::Black: return sf::Color(0, 0, 0, 255);
        
        case Tile::Dirt: return sf::Color(128, 64, 0, 255);
        case Tile::Grass: return sf::Color(0, 255, 0, 255);
        case Tile::Tarmac: return sf::Color(102, 102, 102, 255);
        case Tile::Pavement: return sf::Color(192, 192, 192, 255);
        case Tile::RoadCenterLine: return sf::Color(255, 255, 0, 255);
        case Tile::BrickWall: return sf::Color(255, 128, 0, 255);
        case Tile::DoorClosedN:
        case Tile::DoorClosedS:
        case Tile::DoorClosedE:
        case Tile::DoorClosedW:
          return sf::Color(128, 128, 0, 255);
    }
}
struct TileNeighbourhood {
    Tile neigh[3][3];
};

// We can classify tiles like so: std::bitset<int(Tile::Last)> indoorTiles;
static TileNeighbourhood neighbourhood(int tilex, int tiley, Tile centre) {
    TileNeighbourhood horse;
    horse.neigh[0][0] = Tile::Black;
    horse.neigh[0][1] = Tile::Black;
    horse.neigh[0][2] = Tile::Black;
    horse.neigh[1][0] = Tile::Black;
    horse.neigh[1][1] = centre;
    horse.neigh[1][2] = Tile::Black;
    horse.neigh[2][0] = Tile::Black;
    horse.neigh[2][1] = Tile::Black;
    horse.neigh[2][2] = Tile::Black;
    
    return horse; // TODO
}
static void drawTile(int xpx, int ypx, Tile tile) {
    sf::Shape box = sf::Shape::Rectangle(xpx, ypx, xpx + TILE_SIZE, ypx + TILE_SIZE, colorForTile(tile));
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
        if (xcoord + TILE_SIZE < vr.minx) { /* printf("  %d + T < %d CONT\n", xcoord, vr.minx); */ continue; }
        if (xcoord > TILE_SIZE + vr.maxx) { /* printf("  %d > T + %d BREAK\n", xcoord, vr.maxx); */ break; }
        
        for (int j = 0; j < CHUNK_SIZE; j++) {
            int ycoord = (cy * CHUNK_SIZE + j) * TILE_SIZE;
            
            if (ycoord + TILE_SIZE < vr.miny) { /* printf("%d + T < %d CONT\n", ycoord, vr.miny); */ continue; }
            if (ycoord > TILE_SIZE + vr.maxy) { /* printf("%d > T + %d BREAK\n", ycoord, vr.maxy); */ break; }
            
            // printf("Drawing tile at %d, %d :: %d, %d -> %d\n", xcoord, ycoord, i, j, chunk.tiles[i][j]);
            drawTile(xcoord, ycoord, chunk.tiles[i][j]);
        }
    }
}
static void drawChunks(ViewportRect vr) {
    // ViewportRect vr = { meX - vW/2, meX + vW - vW/2, meY - vH/2, meY + vH - vH/2 };
    
    int chunk_tile_lg = CHUNK_SIZE_LOG + TILE_SIZE_LOG; // used to have a -1, wtf?
    int chunkMinX = vr.minx >> chunk_tile_lg;
    int chunkMaxX = vr.maxx >> chunk_tile_lg;
    
    int chunkMinY = vr.miny >> chunk_tile_lg;
    int chunkMaxY = vr.maxy >> chunk_tile_lg;
    
    --chunkMinX;
    --chunkMinY;
    ++chunkMaxX;
    ++chunkMaxY;
    
    auto& chunks = GAME.world.chunks;
    for (int x = chunkMinX; x <= chunkMaxX; x++) {
        for (int y = chunkMinX; y <= chunkMaxX; y++) {
            // printf("Attempting to draw chunk (%d, %d) = %d\n", x, y, int(chunks.count(std::make_pair(x, y))));
            if (chunks.count(std::make_pair(x, y))) {
                drawChunk(x, y, vr, chunks[std::make_pair(x, y)]);
            }
        }
    }
}

void drawScores() {
    // Draw two rectangles
    double viewportWidth = GAME.viewportWidth;
    double width = viewportWidth / 6.0;
    double height = width * 0.35;
    double centreMargin = viewportWidth / 100.0;
    bool playerIsRed = true;
    sf::Color red = sf::Color(217, 129, 128, 255);
    sf::Color blue = sf::Color(126, 174, 217, 255);
    
    // static sf::Font scoreFont = sf::Font::LoadFromFile("Helvetica")
    
    sf::Color leftColor = playerIsRed ? red : blue;
    sf::Shape leftbox = sf::Shape::Rectangle(0, 0, width, height, leftColor);
    leftbox.SetPosition(viewportWidth / 2.0 - centreMargin - width, centreMargin * 2.0);
    GAME.app->Draw(leftbox);

    sf::Color rightColor = playerIsRed ? blue : red;
    sf::Shape rightbox = sf::Shape::Rectangle(0, 0, width, height, rightColor);
    rightbox.SetPosition(viewportWidth / 2.0 + centreMargin, centreMargin * 2.0);
    GAME.app->Draw(rightbox);

    // box.SetCenter(width / 2, height / 2);
    // box.SetRotation(360.0 - player.angle.angle * 180.0 / M_PI + 90.0);
    
    
}

static void render() {
    // *** Draw Game ***
    
    // Draw chunks
    ViewportRect vr = { GAME.viewportX, GAME.viewportX + GAME.viewportWidth, GAME.viewportY, GAME.viewportY + GAME.viewportHeight };
    // int meX = round(GAME.world.me->position.x);
    // int meY = round(GAME.world.me->position.y);
    // int vW = GAME.viewportWidth;
    // int vH = GAME.viewportHeight;
    
    // Center the view on the player
    sf::View view(sf::FloatRect(GAME.viewportX, GAME.viewportY, GAME.viewportX + GAME.viewportWidth, GAME.viewportY + GAME.viewportHeight));
    GAME.app->SetView(view);
    
    drawChunks(vr);
    
    // Draw the players
    drawAllPlayers();
    
    
    // *** Draw Interface ***
    GAME.app->SetView(GAME.app->GetDefaultView());
    
    // Draw the scores at the top
    drawScores();
    
    // Get the mouse position, and draw a crosshair
    drawCrosshair();
}

