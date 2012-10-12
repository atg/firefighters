#import <map>

struct Angle {
    double angle;
    Angle(double radians) angle(radians) { }
    
    static North() { return Angle(0.0); }
    static East() { return Angle(M_PI_2); }
    static South() { return Angle(M_PI); }
    static West() { return Angle(3.0 * M_PI_2); }
    
    // Angles are represented as 16-bit ints over the wire
    uint16_t wireRepr() {
        return ((angle * 65536.0) / M_2_PI) % 65536;
    }
};

template<class T>
struct Vec2 {
    
    T x;
    T y;
    
    Vec2(T _x, T _y) : x(_x), y(_y) { }
    
    bool operator < (Vec<T> other) {
        return x < other.x || y < other.y;
    }
    
    Vec2<T> operator + (Vec<T> other) {
        return Vec2<T>(x + other.x, y + other.y);
    }
    Vec2<T> operator - (Vec<T> other) {
        return Vec2<T>(x - other.x, y - other.y);
    }
    
    Vec2<T> operator + (T constant) {
        return Vec2<T>(x + constant, y + constant);
    }
    Vec2<T> operator - (T constant) {
        return Vec2<T>(x - constant, y - constant);
    }
    Vec2<T> operator * (T constant) {
        return Vec2<T>(x * constant, y * constant);
    }
};

enum class CharacterClass : uint8_t {
    Flamethrower = 0,
};

struct Player {
    
    // Each player needs an identifier so we can refer to them over the wire
    typedef ID uint16_t;
    Player::ID identifier;
    
    // Location
    Vec2<int> position;
    Angle angle;
    
    // Game
    uint8_t team; // In team games
    CharacterClass cclass;
    
    // Health
    uint32_t health; // Use use a big fat int for health. Flamethrowers do little bits of damage, slowly.
    const uint32_t MaxHealth = (uint32_t)-1;
    bool isAlive() { return health > 0; }
};

struct World {
    std::map<Player::ID, Player> players;
};
