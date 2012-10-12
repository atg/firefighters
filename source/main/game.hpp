#import <SFML/Graphics.hpp>
#import <set>
#import "world/map.hpp"

// Global game state
struct Game {
    bool isClient, isServer;
    World world;
    
    int port;
    
    // Client state
    std::string serverIP;
    
    std::set<sf::Key::Code> heldKeys;
    sf::RenderWindow* app;
    
    int mouseX;
    int mouseY;
    
    // Server state
    // ...
};

extern Game GAME;

