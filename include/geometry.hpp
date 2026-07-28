#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <array>
#include <cmath>

struct Vector2D {
    double x;
    double y;
};
template <std::size_t N>
using Polygon = std::array<Vector2D, N>;

using Line     = Polygon<2>;
using Triangle = Polygon<3>;

struct Circle {
    Vector2D center;
    float radius;
};

Circle triangle_get_circumcircle(const Triangle& triangle);
float triangle_sign(Vector2D p1, Vector2D p2, Vector2D p3);

#endif
