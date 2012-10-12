#import <cmath>

struct Angle {
    double angle;
    Angle(double radians) : angle(radians) { }

    static Angle North() { return Angle(0.0); }
    static Angle East() { return Angle(M_PI_2); }
    static Angle South() { return Angle(M_PI); }
    static Angle West() { return Angle(3.0 * M_PI_2); }
    
    void normalize() {
        if (angle > 0 && angle < M_2_PI)
            return;
        
        while (angle < 0 && angle >= - M_2_PI)
            angle += M_2_PI;
        
        while (angle > M_2_PI && angle <= 2 * M_2_PI)
            angle -= M_2_PI;
    }
    
    // Angles are represented as 16-bit ints over the wire
    // uint16_t wireRepr() {
    //    return round((angle * 65536.0) / M_2_PI) % 65536;
    // }
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
