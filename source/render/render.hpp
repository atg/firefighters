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
    
    sf::Color color = isUser ? sf::Color(255, 255, 124, 255) : sf::Color(255, 124, 124, 255);
    sf::Shape box = sf::Shape::Rectangle(0, 0, TILE_SIZE * 5.0 / 3.0, TILE_SIZE * 2.0 / 3.0, color);
    box.SetCenter(player.position.x * TILE_SIZE, player.position.y * TILE_SIZE);
    box.SetRotation(player.angle.angle * 360.0 / M_2_PI);
    
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

static void render() {
    // Get the mouse position, and draw a crosshair
    drawCrosshair();
    
    // Draw the players
    drawAllPlayers();
}

