#import <boost/circular_buffer.hpp>
#import <boost/random.hpp>
#import <SFML/Window.hpp>

struct Player;

template<class T>
bool boolWithProbability(T& gen, float prob) {
    if (prob <= 0.0) return false;
    if (prob >= 1.0) return true;
    
    boost::random::uniform_01<float> dist;
    return dist(gen) <= prob;
}
template<class T>
float normalRealInRange(T& gen, float a, float b, float tightness) {
    float mu = (a + b) / 2.0;
    float sigma = tightness * (b - a) / 2.0;
    
    boost::random::normal_distribution<float> dist(mu, sigma);
    float val = dist(gen);
    if (val >= a && val <= b)
        return val;
    
    for (int i = 0; i < 10; i++) {
        val = dist(gen);
        if (val >= a && val <= b)
            return val;
    }
    
    return mu;
}

// Laplace distribution?


struct Particle {
    
    Vec2<float> position;
    Vec2<float> velocity;
    Vec2<float> acceleration;
    
    float birthday;
    float lifetime;
    float age;
    
    bool dead;
    
    Particle(float _birthday, float _lifetime, Vec2<float> _position)
      : birthday(_birthday), lifetime(_lifetime), position(_position), age(0.0), dead(false) { }
};
struct Emitter {
    sf::Clock timer;
    boost::random::mt11213b rng;
    
    Vec2<int> position;
    Angle direction;
    Angle arc;
    float averageSpeed;
    
    float lastUpdate;
    
    sf::Clock particleClock;
    float spawnFrequency;
    float lastSpawned;
    
    boost::circular_buffer<Particle> particles;
    
    Emitter() : direction(Angle(0.0)), arc(Angle(0.0)) { }
    Emitter(Angle _direction, Angle _arc, int _spawnFrequency, int capacity)
      : timer(), rng(), direction(_direction), arc(_arc), spawnFrequency(_spawnFrequency), particles(capacity), particleClock(), lastSpawned(0.0) { }
    
    void begin() {
        timer.Reset();
        particleClock.Reset();
    }
    void update(Player& owner);
};
