#include "../include/geometry.hpp"

// From: https://en.wikipedia.org/wiki/Circumcircle#Circumcenter_coordinates
Circle triangle_get_circumcircle(const Triangle& triangle) {
    float D;
    Circle circle;

    Vector2 A = {0, 0};
    Vector2 B = {triangle[1].x - triangle[0].x, triangle[1].y - triangle[0].y};
    Vector2 C = {triangle[2].x - triangle[0].x, triangle[2].y - triangle[0].y};

    D               = 2 * (B.x * C.y - B.y * C.x);
    circle.center.x = 1 / D * (C.y * (B.x * B.x + B.y * B.y) - B.y * (C.x * C.x + C.y * C.y));
    circle.center.y = 1 / D * (B.x * (C.x * C.x + C.y * C.y) - C.x * (B.x * B.x + B.y * B.y));
    circle.radius   = std::sqrt(circle.center.x * circle.center.x + circle.center.y * circle.center.y);

    circle.center.x += triangle[0].x;
    circle.center.y += triangle[0].y;

    return circle;
}
float triangle_sign(Vector2 p1, Vector2 p2, Vector2 p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}
