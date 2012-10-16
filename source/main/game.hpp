#import <SFML/Graphics.hpp>
#import <set>
#import "world/map.hpp"

// Global game state
struct Game {
    bool isClient, isServer;
    World world;
    
    int port;
    
    sf::Clock clock;
    
    // Client state
    uint32_t clientID;
    
    std::string serverIP;
    
    std::set<sf::Key::Code> heldKeys;
    sf::RenderWindow* app;
    
    int mouseX;
    int mouseY;
    
    // Server state
    // ...
};

extern Game GAME;

static void game_setClientID(void* ctx) {
    uint32_t cid = *(uint32_t*)ctx;
    delete (uint32_t*)ctx;
    
    printf("Got ClientID: %u\n", cid);
    GAME.clientID = cid;
    
    // Create the player
    Player me(GAME.clientID);
    GAME.world.players[me.identifier] = me;
    GAME.world.me = &(GAME.world.players[me.identifier]);
}
void game_clientQuickUpdate(void* ctx);
void game_serverQuickUpdate(void* ctx);

struct Invocation {
    void (*function)(void*);
    void* context;
    
    Invocation(void (*_function)(void*), void* _context) : function(_function), context(_context) { }
    
    void invoke() {
        function(context);
    }
};
struct InvocationQueue {
    sf::Mutex mutex;
    std::queue<Invocation> q;

    void push(void (*function)(void*), void* arg) {
        sf::Lock lock(mutex);
        q.push(Invocation(function, arg));
    }
    void popAll() {
        sf::Lock lock(mutex);
        for (; !q.empty(); q.pop()) {
            q.back().invoke();
        }
    }
};

static InvocationQueue& mainQueue() {
    static InvocationQueue _mainQueue;
    return _mainQueue;
}
