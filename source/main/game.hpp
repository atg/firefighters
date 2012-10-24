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

struct InvocationMessage {
    uint32_t sender;
    std::string data;
    
    InvocationMessage(uint32_t _sender, std::string _data) : sender(_sender), data(_data) { }
};
struct Invocation {
    void (*function)(InvocationMessage);
    InvocationMessage context;
    
    Invocation(void (*_function)(InvocationMessage), InvocationMessage _context) : function(_function), context(_context) { }
    
    void invoke() {
        function(context);
    }
};
struct InvocationQueue {
    sf::Mutex mutex;
    std::queue<Invocation> q;

    void push(void (*function)(InvocationMessage), InvocationMessage arg) {
        sf::Lock lock(mutex);
        q.push(Invocation(function, arg));
    }
    void popAll() {
        
        std::vector<Invocation> others;
        mutex.Lock();
        others.reserve(q.size());
        for (; !q.empty(); q.pop()) {
            others.push_back(q.back());
        }
        mutex.Unlock();
        
        for (Invocation& invok : others) {
            invok.invoke();
        }
    }
};
static InvocationQueue& mainQueue() {
    static InvocationQueue _mainQueue;
    return _mainQueue;
}


static void game_setClientID(InvocationMessage ctx) {
    uint32_t cid = ctx.sender;
    printf("Got ClientID: %u\n", cid);
    GAME.clientID = cid;
    
    // Create the player
    Player me(GAME.clientID);
    GAME.world.players[me.identifier] = me;
    GAME.world.me = &(GAME.world.players[me.identifier]);
}
void game_clientQuickUpdate(InvocationMessage ctx);
void game_serverQuickUpdate(InvocationMessage ctx);
