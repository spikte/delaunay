#ifndef BOWYER_WATSON_HPP
#define BOWYER_WATSON_HPP

#include "../lib/predicates.h"
#include "graph.hpp"
#include "slot_array.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

struct GraphListTriangulation {
    std::vector<GraphListTriangle> triangles;
    //std::unordered_map<uint32_t, std::array<uint32_t, 2>> edge_map;
    std::vector<bool> active;
    std::vector<uint32_t> freeTriangle;
};
uint32_t graphtrig_add_vertex(GraphListTriangulation& triangulation, GraphListTriangle triangle);
void graphtrig_rm_vertex(GraphListTriangulation& triangulation, uint32_t triangle_id);
std::array<uint32_t, 3> graphtrig_get_neighbors(GraphListTriangulation& triangulation, uint32_t triangle);
enum class NodeProcessState {
    INIT,
    WALK_TO_CONTAINING,
    FIND_BAD_TRIANGLES,
    BUILD_POLYGON,
    DELETE_TRIANGLES,
    ADD_NEW_TRIANGLES,
};

struct StateInit {};
struct StateWalkToContaining {
    uint32_t walk_current_triangle_id = UINT32_MAX;
};
struct BoundaryEdge {
    uint32_t a;
    uint32_t b;
    uint32_t triangle_outside;
};
struct StateFindBadTriangles {
    uint32_t bad_current_triangle_id = UINT32_MAX;
    Circle bad_current_triangle_circle{};
    std::queue<uint32_t> to_visit;
    std::vector<bool> visited;
    std::vector<uint32_t> bad_triangles;
    std::vector<bool> bad_triangles_map; // O(1) access to if a triangles is visited
    std::vector<BoundaryEdge> boundary_edges;
};
struct State {
    NodeProcessState type                                                      = NodeProcessState::INIT;
    std::variant<StateInit, StateWalkToContaining, StateFindBadTriangles> data = StateInit{};
};
struct TrigTriangle {
    GraphListTriangle triangle;
    std::array<uint32_t, 3> neighbors;
    Circle circle;
};
int trigtriangle_get_edge_index(const TrigTriangle& trig_triangle, uint32_t node1, uint32_t node2);
struct Triangulation {
    std::vector<uint32_t> process_order;
    size_t process_index  = 0;
    uint32_t current_node = UINT32_MAX;
    Vector2D current_point{};
    SlotArray triangles;
    GraphListTriangle super_triangle{};
    uint32_t last_inserted = UINT32_MAX;
    State state;
};

// WALK_TO_CONTAINING
void graph2D_triangulate_walk_state_init(Triangulation& triangulation);
uint32_t graph2D_triangulate_walk_state_it(GraphList2D& graph, Triangulation& triangulation, StateWalkToContaining& state);
// FIND_BAD_TRIANGLES
void graph2D_triangulate_find_bad_state_init(Triangulation& triangulation, StateFindBadTriangles& state, uint32_t start_triangle_id);
void graph2D_triangulate_find_bad_state_it(GraphList2D& graph, Triangulation& triangulation, StateFindBadTriangles& state);

void graph2D_triangulate_bowyer_watson_init(GraphList2D& graph, Triangulation& triangulation);
void graph2D_triangulate_bowyer_watson_finalize(GraphList2D& graph, Triangulation& triangulation);
bool graph2D_triangulate_bowyer_watson_it(GraphList2D& graph, Triangulation& triangulation);
void graph2D_triangulate_bowyer_watson(GraphList2D& graph);

void graph2D_triangulate_find_bad_state_draw(Triangulation& triangulation, StateFindBadTriangles& state);

#endif
