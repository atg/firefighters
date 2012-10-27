#import <boost/circular_buffer.hpp>
#import <boost/random.hpp>
#import "physics/collision.hpp"

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
    void update() {
        
        float t = timer.GetElapsedTime();
        float dt = t - lastUpdate;
        if (dt < 1.0 / 100.0)
            return;
        // printf("dt = %f\n", dt);
        lastUpdate = t;
        
        // Delete expired particles
        while (!particles.empty()) {
            const Particle& p = particles.front();
            if (t - p.birthday > p.lifetime || p.dead)
                particles.pop_front();
            else
                break;
        }
        
        // Update particles
        for (Particle& p : particles) {
            Vec2<float> v = p.acceleration * dt + p.velocity;
            Vec2<float> r = p.position + (v + p.velocity) * 0.5 * dt;
            
            // p.position.x += 1.0;
            // p.position.y += 1.0;
            
            p.velocity = v;
            p.position = r;
            p.age = t - p.birthday;
            
            // Check for collisions
            Tile tile = worldTileAtPixel(p.position.x, p.position.y)
            if (is_solid(tile))
                p.dead = true;
        }
        
        // Work out how many particles we need to create
        // double zpart = 0.0;
        // double qpart = std::modf(dt * spawnFrequency, &zpart);
        
        int n = 0;
        if (particleClock.GetElapsedTime() + lastSpawned > 1.0 / spawnFrequency) {
            lastSpawned += particleClock.GetElapsedTime();
            particleClock.Reset();
        
            while (lastSpawned >= 1.0 / spawnFrequency) {
                n++;
                lastSpawned -= 1.0 / spawnFrequency;
            }
        }
        
        // We need to take a new approach to spawning.
        // Have a "last spawned" timestamp
        // When the time exceeds 1/spawnFrequency seconds, then spawn by an appropriate amount
        
        // int n = zpart;
        // if (boolWithProbability(rng, qpart))
        //     n++;
        
        if (n == 0)
            return;
        
        for (int i = 0; i < n; i++) {
            Particle p(t, 1.0, Vec2<float>(position.x, position.y));
            
            float theta = normalRealInRange(rng, direction.angle - arc.angle / 2.0, direction.angle + arc.angle / 2.0, 2.0);
            float speed = normalRealInRange(rng, averageSpeed / 2.0, averageSpeed * 3.0 / 2.0, 2.0);
            
            // printf("Random test: %f\n", normalRealInRange(rng, 10.0, 20.0, 2.0));
            // printf("Theta angle: %f | %f => %f\n", direction.angle, arc.angle, theta);
            // printf("Speed: %f => %f\n", averageSpeed, speed);
            
            p.velocity = Vec2<float>::FromPolar(speed, Angle(theta));
            
            // float theta2 = normalRealInRange(rng, 2.0 * theta - direction.angle, direction.angle, 2.0);
            // float accel = normalRealInRange(rng, 2.0 * theta - direction.angle, direction.angle, 2.0);
            p.acceleration = Vec2<float>(0.0, 0.0); //Vec2<float>::FromPolar(- speed, Angle(theta2));
            
            particles.push_back(p);
        }
    }
};
