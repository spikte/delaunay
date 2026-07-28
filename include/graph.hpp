#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "geometry.hpp"
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
#include "../lib/predicates.h"

/* Graph */
struct Graph {
    std::vector<std::vector<uint32_t>> adj_list;
    std::vector<bool> active;
    Graph(int n) {
        adj_list.resize(n);
        active.resize(n, true);
    }
};

template <std::size_t N>
using GraphPolygon  = std::array<uint32_t, N>;
using GraphEdge     = GraphPolygon<2>;
using GraphTriangle = GraphPolygon<3>;

// Graph Edge
void graph_add_edge_one(Graph& graph, int node1, int node2);
void graph_add_edge(Graph& graph, int node1, int node2);
void graph_rm_edge_one(Graph& graph, int node1, int node2);
void graph_rm_edge(Graph& graph, int node1, int node2);
bool graph_get_edge(Graph& graph, int node1, int node2);
std::vector<GraphEdge> graph_get_edges(Graph& graph);
// Graph Vertex
uint32_t graph_add_vertex(Graph& graph);
void graph_rm_vertex(Graph& graph, int node);
// Graph TriangulationNaive (that doesn't require position)
void graph_triangulate_fan(Graph& graph);

/* Graph2D */
// TODO: I could make a general graph with vertex data struct using void and data type size
struct Graph2D : Graph {
    std::vector<Vector2> positions;
    Graph2D(int n) : Graph(n) {
        positions.resize(n);
    };
};
// Graph2D utils
Vector2 graph2D_get_min_bound(Graph2D& graph);
Vector2 graph2D_get_max_bound(Graph2D& graph);
Triangle graph2D_get_super_triangle(Graph2D& graph, float epsilon = 10.f);
float graph2D_get_average_distance(Graph2D& graph);
struct GridEntry {
    uint32_t point;
    uint32_t next;
};
std::vector<uint32_t> graph2D_sort_grid_indices(Graph2D& graph, Vector2 cell_dims);
// Graph2D vertex
uint32_t graph2D_add_vertex(Graph2D& graph, Vector2 pos);
// Graph2D triangulation
struct GraphTriangulation {
    std::vector<GraphTriangle> triangles;
    //std::unordered_map<uint32_t, std::array<uint32_t, 2>> edge_map;
    std::vector<bool> active;
    std::vector<uint32_t> freeTriangle;
};
uint32_t graphtrig_add_vertex(GraphTriangulation& triangulation, GraphTriangle triangle);
void graphtrig_rm_vertex(GraphTriangulation& triangulation, uint32_t triangle_id);
std::array<uint32_t, 3> graphtrig_get_neighbors(GraphTriangulation& triangulation, uint32_t triangle);
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
    GraphTriangle triangle;
    std::array<uint32_t, 3> neighbors;
    Circle circle;
};
int trigtriangle_get_edge_index(const TrigTriangle& trig_triangle, uint32_t node1, uint32_t node2);
struct Triangulation {
    std::vector<uint32_t> process_order;
    size_t process_index = 0;
    uint32_t current_node = UINT32_MAX;
    Vector2 current_point{};
    SlotArray triangles;
    GraphTriangle super_triangle{};
    uint32_t last_inserted  = UINT32_MAX;
    State state;
};

// WALK_TO_CONTAINING
void graph2D_triangulate_walk_state_init(Triangulation& triangulation);
uint32_t graph2D_triangulate_walk_state_it(Graph2D& graph, Triangulation& triangulation, StateWalkToContaining& state);
// FIND_BAD_TRIANGLES
void graph2D_triangulate_find_bad_state_init(Triangulation& triangulation, StateFindBadTriangles& state, uint32_t start_triangle_id);
void graph2D_triangulate_find_bad_state_it(Graph2D& graph, Triangulation& triangulation, StateFindBadTriangles& state);

void graph2D_triangulate_bowyer_watson_init(Graph2D& graph, Triangulation& triangulation);
void graph2D_triangulate_bowyer_watson_finalize(Graph2D& graph, Triangulation& triangulation);
bool graph2D_triangulate_bowyer_watson_it(Graph2D& graph, Triangulation& triangulation);
void graph2D_triangulate_bowyer_watson(Graph2D& graph);

void graph2D_triangulate_find_bad_state_draw(Triangulation& triangulation, StateFindBadTriangles& state);

#endif
