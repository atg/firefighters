#import <SFML/Window.hpp>
#import <string>
#import <set>

#import "world/map.hpp"
#import "render/render.hpp"

// Global game state
struct Game {
    bool isClient, isServer;
    World world;
    
    int port;
    
    // Client state
    std::string serverIP;
    
    std::set<sf::Key::Code> heldKeys;
    sf::Window* app;
    
    // Server state
    // ...
};
static Game GAME;


static void handleKeyEvent(sf::Event& event, bool isKeyUp) {
    if (isKeyUp)
        GAME.heldKeys.insert(event.Key.Code);
    else if (GAME.heldKeys.find(event.Key.Code) != GAME.heldKeys.end())
        GAME.heldKeys.erase(event.Key.Code);
}


static bool isKeyDown(sf::Key::Code code) {
    return GAME.heldKeys.find(code) != GAME.heldKeys.end();
}

static void processEvents() {
    sf::Event event;
    while (GAME.app->GetEvent(event)) {
        // Close window : exit
        if (event.Type == sf::Event::Closed)
            GAME.app->Close();
        
        // Escape key : exit
        else if ((event.Type == sf::Event::KeyPressed) && (event.Key.Code == sf::Key::Escape))
            GAME.app->Close();
        
        // W A S D left right up down, etc
        else if (event.Type == sf::Event::KeyPressed || event.Type == sf::Event::KeyReleased) {
            handleKeyEvent(event, event.Type == sf::Event::KeyReleased);
        }
        
        // Resize event : adjust viewport
        else if (event.Type == sf::Event::Resized)
            glViewport(0, 0, event.Size.Width, event.Size.Height);
    }
    
    
    // Get the mouse X and Y
    int mouseX = GAME.app->GetInput().GetMouseX();
    int mouseY = GAME.app->GetInput().GetMouseY()
    
    Vec2<double> viewportPosition = Vec2<double>(0.0, 0.0); // TODO: Moving viewports!
    Vec2<double> playerPosition = GAME.world.me.position;
    Vec2<double> mousePosition = viewportPosition + Vec2<double>(mouseX, mouseY);
    
    // Movement keys
    
    // Basically we re-origin the playerPosition in terms of the mouse position
    // Then we turn that into polar form, and decrease the distance
    // Then we turn add the playerPosition back on to it
    playerPosition -= mousePosition;
    double distance = playerPosition.distance();
    Angle angle = playerPosition.angle();
    bool changedPosition = false;
    
    const double walkingSpeed = 4.59; // ft/s
    
    // We should really use some kind of clock instead of having uneven movement speeds
    double movementDistance = 1.0; // For now we just move one foot
    
    if (isKeyDown(sf::Key::Up) || isKeyDown(sf::Key::W)) {
        // We want to move the player TOWARDS the mouse cursor
        distance -= movementDistance;
        if (distance < 0.0)
            distance = 0.0;
    }
    else if (isKeyDown(sf::Key::Down) || isKeyDown(sf::Key::S)) {
        // We want to move the player AWAY from the mouse cursor
        distance += movementDistance;
    }
    else if (isKeyDown(sf::Key::Left) || isKeyDown(sf::Key::A)) {
        // We want to move the player ANTI-CLOCKWISE around the mouse cursor
        // Work out the circumference of the circle
        double circum = M_2_PI * distance;
        if (circum < 1.0)
            circum = 1.0;
        
        angle += movementDistance * M_2_PI / circum;
    }
    else if (isKeyDown(sf::Key::Right) || isKeyDown(sf::Key::D)) {
        // We want to move the player CLOCKWISE around the mouse cursor
        // Work out the circumference of the circle
        double circum = M_2_PI * distance;
        if (circum < 1.0)
            circum = 1.0;
        
        angle += movementDistance * M_2_PI / circum;
    }
    
    if (changedPosition) {
        playerPosition = Vec2<double>::FromPolar(distance, angle);
        playerPosition += mousePosition;
        
        // Remember to tell the server after moving the player!
        GAME.world.me.angle = angle;
        GAME.world.me.position = playerPosition;
        printf("New Position (%lf, %lf) pointing %lf\n", GAME.world.me.position.x, GAME.world.me.position.y, GAME.world.me.angle);
    }
}


static void parseArguments(int argc, char *argv[]) {
    GAME.isClient = true;
    GAME.isServer = false;
    
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--server")) {
            GAME.isClient = false;
            GAME.isServer = true;
        }
        
        if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) {
            if (i + 1 >= argc)
                continue;
            GAME.port = atoi(argv[i + 1]);
        }
        
        if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--connect")) {
            if (i + 1 >= argc)
                continue;
            GAME.serverIP = std::string(argv[i + 1]);
        }
    }
}

void setUpServer() {
    // TODO
}
void setUpClient() {
    // TODO
    
    // FOR NOW just create a fake player (remove this when actual players)
    Player me;
    me.identifier = 42;
    me.team = 0;
    me.cclass = CharacterClass::Flamethrower;
    me.health = Player::MaxHealth;
    
    GAME.world.players.push_back(me);
    GAME.world.me = &(GAME.world.players.back());
}
void connectToServer() {
    // TODO
}

int main(int argc, char *argv[]) {
    
    // Are we a server?
    parseArguments(argc, argv);
    
    // Create the main window
    sf::Window app(sf::VideoMode(32 * TILE_SIZE, 20 * TILE_SIZE, 32), "Firefighters");
    GAME.app = &app;
    GAME.app->ShowMouseCursor(false);
    
    // Create a player for us
    if (isServer) {
        setUpServer();
    }
    else {
        setUpClient();
        connectToServer();
    }
    
    // Set up run loop
    while (app.IsOpened()) {
        
        // Process events
        processEvents();
        
        app.SetActive();
        
        // Clear color buffer
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Your drawing here...
        // render();
        
        // Display rendered frame on screen
        app.Display();
    }
    
    return 0;
}
