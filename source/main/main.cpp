#define MAIN_UNIT

#import <SFML/Window.hpp>
#import <SFML/Graphics.hpp>
#import <SFML/System/Sleep.hpp>
#import <string>
#import <set>
#import <stdio.h>

#import "world/map.hpp"
#import "render/render.hpp"

#import "main/game.hpp"
#import "main/preferences.hpp"
#import "net/net.hpp"

#import "net/net_client.hpp"
#import "net/net_server.hpp"

#import "world/buildmap.hpp"

#import "physics/collision.hpp"
#import "physics/particle.hpp"

Game GAME;

static void handleKeyEvent(sf::Event& event, bool isKeyUp) {
    if (!isKeyUp)
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
            // printf("event.Key.Code = %d\n", event.Key.Code);
            handleKeyEvent(event, event.Type == sf::Event::KeyReleased);
        }
        
        // Resize event : adjust viewport
        else if (event.Type == sf::Event::Resized)
            glViewport(0, 0, event.Size.Width, event.Size.Height);
    }
    
    
    // Get the mouse X and Y
    int mouseX = GAME.app->GetInput().GetMouseX();
    int mouseY = GAME.app->GetInput().GetMouseY();
    GAME.mouseX = mouseX;
    GAME.mouseY = mouseY;
    
    GAME.viewportWidth = GAME.app->GetWidth();
    GAME.viewportHeight = GAME.app->GetHeight();
    GAME.viewportX = round(GAME.world.me->position.x - GAME.viewportWidth / 2.0);
    GAME.viewportY = round(GAME.world.me->position.y - GAME.viewportHeight / 2.0);
    
    Vec2<double> viewportPosition = Vec2<double>(GAME.viewportX, GAME.viewportY);
    Vec2<double> playerPosition = GAME.world.me->position;
    Vec2<double> mousePosition = viewportPosition + Vec2<double>(mouseX, mouseY);
    
    // Movement keys
    
    // Basically we re-origin the playerPosition in terms of the mouse position
    // Then we turn that into polar form, and decrease the distance
    // Then we turn add the playerPosition back on to it
    playerPosition = playerPosition - mousePosition;
    double distance = playerPosition.distance();
    Angle angle = playerPosition.angle();
    bool changedPosition = false;
    
    const double walkingSpeed = 220.0; //4.59; // ft/s
    
    // We should really use some kind of clock instead of having uneven movement speeds
    double movementDistance = GAME.clock.GetElapsedTime() * walkingSpeed; // For now we just move one foot
    
    if (isKeyDown(sf::Key::Up) || isKeyDown(sf::Key::W)) {
        // We want to move the player TOWARDS the mouse cursor
        distance -= movementDistance;
        if (distance < 0.0)
            distance = 0.0;
        changedPosition = true;
    }
    else if (isKeyDown(sf::Key::Down) || isKeyDown(sf::Key::S)) {
        // We want to move the player AWAY from the mouse cursor
        distance += movementDistance;
        changedPosition = true;
    }
    
    if (isKeyDown(sf::Key::Left) || isKeyDown(sf::Key::A)) {
        // We want to move the player ANTI-CLOCKWISE around the mouse cursor
        // Work out the circumference of the circle
        if (distance < 1.0) {
            angle.angle += GAME.clock.GetElapsedTime() * 2.0 * M_PI / 1.0;
        }
        else {
            double circum = 2.0 * M_PI * distance;
            double radians_per_foot = 2.0 * M_PI / circum;
            angle.angle += movementDistance * radians_per_foot;
        }
        angle.normalize();
        changedPosition = true;
    }
    else if (isKeyDown(sf::Key::Right) || isKeyDown(sf::Key::D)) {
        // We want to move the player CLOCKWISE around the mouse cursor
        // Work out the circumference of the circle
        if (distance < 1.0) {
            angle.angle -= GAME.clock.GetElapsedTime() * 2.0 * M_PI / 1.0;
        }
        else {
            double circum = 2.0 * M_PI * distance;
            double radians_per_foot = 2.0 * M_PI / circum;
            angle.angle -= movementDistance * radians_per_foot;
        }
        angle.normalize();
        changedPosition = true;
    }
    
    if (changedPosition) {
        playerPosition = Vec2<double>::FromPolar(distance, angle);
        playerPosition = playerPosition + mousePosition;
        
        GAME.world.me->position = playerPosition;
        GAME.world.me->angle = angle;
    }
    else if (PREFS.playerAngleFollowsMouse) {
        GAME.world.me->angle = angle;
    }
    
    // Handle weapon firing
    Weapon* weapon = GAME.world.me->activeWeapon();
    if (isKeyDown(sf::Key::Space)) {
        if (weapon)
            weapon->start(*GAME.world.me);
    }
    else {
        if (weapon)
            weapon->stop();
    }
    
    GAME.viewportX = round(GAME.world.me->position.x - GAME.viewportWidth / 2.0);
    GAME.viewportY = round(GAME.world.me->position.y - GAME.viewportHeight / 2.0);
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
    generate(GAME.world);
    startNetworking();
}
void setUpClient() {
    
}
void connectToServer() {
    startNetworking();
}

int main(int argc, char *argv[]) {
    
    // TCP 9456, UDP 9457
    GAME.port = 9456;
    GAME.clock.Reset();
    GAME.clientID = 0;
    
    // Are we a server?
    parseArguments(argc, argv);
    
    // If this is a server, then we just run the server thread
    if (GAME.isServer) {
        setUpServer();
        
        while (true) {
            mainQueue().popAll();
            serverSendGameState();
            sf::Sleep(0.01);
        }
        
        return 0;
    }
    
    // Create the main window
    sf::RenderWindow app(sf::VideoMode(32 * TILE_SIZE, 20 * TILE_SIZE, 32), "Firefighters");
    GAME.app = &app;
    GAME.app->ShowMouseCursor(false);
    
    // Create a player for us
    setUpClient();
    connectToServer();
    
    sf::Clock networkClock;
    
    // Set up run loop
    while (app.IsOpened()) {
        
        mainQueue().popAll();
        
        if (!GAME.clientID) {
            glClear(GL_COLOR_BUFFER_BIT);
            app.Display();
            continue;
        }
        
        // Process events
        processEvents();
        GAME.clock.Reset();
        
        app.SetActive();
        
        // Clear color buffer
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw everything
        render();
        
        // Display rendered frame on screen
        app.Display();
        
        // Sync server state
        if (networkClock.GetElapsedTime() > 0.05) {
            clientSendGameState();
            networkClock.Reset();
        }
        
        //sleep(1);
    }
    
    return 0;
}
