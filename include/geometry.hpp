#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <array>
#include <cmath>
#include <raylib.h>

template <std::size_t N>
using Polygon = std::array<Vector2, N>;

using Line     = Polygon<2>;
using Triangle = Polygon<3>;

struct Circle {
    Vector2 center;
    float radius;
};

Circle triangle_get_circumcircle(const Triangle& triangle);
float triangle_sign(Vector2 p1, Vector2 p2, Vector2 p3);

#endif
