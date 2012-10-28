#import <cmath>
#import <string>

#import <stdio.h>
#import <stdlib.h>

template<class T>
static T propermod(T a, T n) {
    return ((a % n) + n) % n;
}
template<class T>
static T fpropermod(T a, T n) {
    return fmod(fmod(a, n) + n, n);
}

struct Angle {
    double angle;
    Angle(double radians) : angle(radians) { }

    static Angle North() { return Angle(0.0); }
    static Angle East() { return Angle(M_PI_2); }
    static Angle South() { return Angle(M_PI); }
    static Angle West() { return Angle(3.0 * M_PI_2); }
    
    void normalize() {
        if (angle > - M_PI && angle < M_PI)
            return;
        
        // angle = fmod(fmod(angle, M_2_PI) + angle, M_2_PI);
        // if (angle > M_PI)
            // angle -= M_2_PI;
        
        while (angle < M_PI)
            angle += 2.0 * M_PI;
        
        while (angle > M_PI)
            angle -= 2.0 * M_PI;
    }
    
    // Angles are represented as 16-bit ints over the wire
    static Angle FromWire(unsigned v) {
        double theta = v;
        // [0, 65536) -> [0, 2 pi)
        theta /= 65536.0;
        theta *= 2 * M_PI;
        
        Angle a = Angle(theta);
        a.normalize();
        return a;
    }
    uint16_t wireRepr() {
       return propermod((int)round((angle * 65536.0) / (2.0 * M_PI)), 65536);
    }
    int degrees() {
        return round(angle * 180.0 / (2.0 * M_PI)) + 180;
    }
};

template<class T>
struct Vec2 {
    
    T x;
    T y;
    
    Vec2() : x(0), y(0) { }
    Vec2(T _x, T _y) : x(_x), y(_y) { }
    
    static Vec2<T> FromPolar(T dist, Angle theta) {
        return
          Vec2<T>(dist * std::cos(theta.angle),
                  dist * std::sin(theta.angle));
    }
    
    // Relations
    bool operator == (Vec2<T> other) {
        return x == other.x && y == other.y;
    }
    bool operator != (Vec2<T> other) {
        return x != other.x || y != other.y;
    }
    bool operator < (Vec2<T> other) {
        return x < other.x || (x == other.x && y < other.y); // Lexicographical ordering
    }
    
    // Polar
    Angle angle() {
        return Angle(atan2((double)y, (double)x));
    }
    
    template<class R = double>
    R distance() {
        return sqrt(double(x * x + y * y));
    }
    
    template<class R = double>
    R distance(Vec2<T> other) {
        T dx = x - other.x;
        T dy = y - other.y;
        return sqrt(double(dx * dx + dy * dy));
    }
    
    
    // Vector operations
    Vec2<T> operator + (Vec2<T> other) {
        return Vec2<T>(x + other.x, y + other.y);
    }
    Vec2<T> operator - (Vec2<T> other) {
        return Vec2<T>(x - other.x, y - other.y);
    }
    Vec2<T> dot(Vec2<T> other) {
        return x * other.x  +  y * other.y;
    }
    
    // Elementwise operations
    template<class R = T>
    Vec2<R> round() {
        return Vec2<R>((R)round(x), (R)round(y));
    }
    template<class R = T>
    Vec2<R> floor() {
        return Vec2<R>((R)std::floor(x), (R)std::floor(y));
    }
    
    // Constant operations
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


static void die(const char* str) {
    fprintf(stderr, "%s\n", str);
    abort();
}
static void coma(const char* str) {
    fprintf(stderr, "%s\n", str);
}

static std::string to_string(int x) {
    char buffer[100] = {0};
    sprintf(buffer, "%d", x);
    return std::string((const char*)buffer);
}

