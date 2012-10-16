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

static void game_setClientID(uint32_t* cid) {
    printf("Got ClientID: %u\n", *cid);
    GAME.clientID = *cid;
    
    // Create the player
    Player me(GAME.clientID);
    GAME.world.players[me.identifier] = me;
    GAME.world.me = &(GAME.world.players[me.identifier]);
}
void game_clientQuickUpdate(std::vector<char>* packet);
void game_serverQuickUpdate(std::pair<std::string, uint32_t>** ctx);

struct Invocation {
    void (*function)(void*);
    void* context;
    Invocation(void (*_function)(void*), void* _context) : function(_function), context(_context) { }
    void invoke() {
        function(context);
        if (context)
            free(context);
    }
};
struct InvocationQueue {
    sf::Mutex mutex;
    std::queue<Invocation> q;

    template<typename T>
    void push(void (*function)(T*), T arg) {
        sf::Lock lock(mutex);
        T* x = (T*)malloc(sizeof(T));
        *x = arg;
        q.push(Invocation((void (*)(void*))function, (void*)x));
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
