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

/* Graph */
struct Graph {
    std::vector<std::vector<uint16_t>> adj_list;
    std::vector<bool> active;
    Graph(int n) {
        adj_list.resize(n);
        active.resize(n, true);
    }
};

template <std::size_t N>
using GraphPolygon  = std::array<uint16_t, N>;
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
uint16_t graph_add_vertex(Graph& graph);
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
// Graph2D vertex
uint16_t graph2D_add_vertex(Graph2D& graph, Vector2 pos);
// Graph2D triangulation
uint32_t hash_two_u16(uint16_t n1, uint16_t n2);
// Naive
struct TriangulationNaive {
    std::vector<GraphTriangle> triangles;
    // Used to delete the super triangle node at the end
    GraphTriangle super_triangle;
};
void graph2D_triangulate_bowyer_watson_naive_init(Graph2D& graph, TriangulationNaive& triangulation);
void graph2D_triangulate_bowyer_watson_naive_finalize(Graph2D& graph, TriangulationNaive& triangulation);
void graph2D_triangulate_bowyer_watson_naive_it(Graph2D& graph, TriangulationNaive& triangulation, uint16_t node);
void graph2D_triangulate_bowyer_watson_naive(Graph2D& graph);

// -- Optimized --
struct GraphTriangulation {
    std::vector<GraphTriangle> triangles;
    //std::unordered_map<uint32_t, std::array<uint16_t, 2>> edge_map;
    std::vector<bool> active;
    std::vector<uint16_t> freeTriangle;
};
uint16_t graphtrig_add_vertex(GraphTriangulation& triangulation, GraphTriangle triangle);
void graphtrig_rm_vertex(GraphTriangulation& triangulation, uint16_t triangle_id);
std::array<uint16_t, 3> graphtrig_get_neighbors(GraphTriangulation& triangulation, uint16_t triangle);
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
    uint16_t walk_current_triangle_id = UINT16_MAX;
};
struct BoundaryEdge {
    uint16_t a;
    uint16_t b;
    uint16_t triangle_outside;
};
struct StateFindBadTriangles {
    uint16_t bad_current_triangle_id = UINT16_MAX;
    Circle bad_current_triangle_circle{};
    std::queue<uint16_t> to_visit;
    std::vector<bool> visited;
    std::vector<uint16_t> bad_triangles;
    std::vector<bool> bad_triangles_map; // O(1) access to if a triangles is visited
    std::vector<BoundaryEdge> boundary_edges;
};
struct State {
    NodeProcessState type                                                      = NodeProcessState::INIT;
    std::variant<StateInit, StateWalkToContaining, StateFindBadTriangles> data = StateInit{};
};
struct TrigTriangle {
    GraphTriangle triangle;
    std::array<uint16_t, 3> neighbors;
    Circle circle;
};
int trigtriangle_get_edge_index(const TrigTriangle& trig_triangle, uint16_t node1, uint16_t node2);
struct Triangulation {
    SlotArray triangles;
    GraphTriangle super_triangle{};
    Vector2 current_point{};            // point currently being inserted
    uint16_t current_node = UINT16_MAX; // node currently being inserted
    size_t last_inserted  = UINT16_MAX;
    State state;
};

// WALK_TO_CONTAINING
void graph2D_triangulate_walk_state_init(Triangulation& triangulation);
uint16_t graph2D_triangulate_walk_state_it(Graph2D& graph, Triangulation& triangulation, StateWalkToContaining& state);
uint16_t graph2D_triangulate_walk_state_run(Graph2D& graph, Triangulation& triangulation, StateWalkToContaining& state);
// FIND_BAD_TRIANGLES
void graph2D_triangulate_find_bad_state_init(Triangulation& triangulation, StateFindBadTriangles& state, uint16_t start_triangle_id);
void graph2D_triangulate_find_bad_state_it(Graph2D& graph, Triangulation& triangulation, StateFindBadTriangles& state);
void graph2D_triangulate_find_bad_state_run(Graph2D& graph, Triangulation& triangulation, StateFindBadTriangles& state);

void graph2D_triangulate_bowyer_watson_init(Graph2D& graph, Triangulation& triangulation);
void graph2D_triangulate_bowyer_watson_finalize(Graph2D& graph, Triangulation& triangulation);
bool graph2D_triangulate_bowyer_watson_it(Graph2D& graph, Triangulation& triangulation, uint16_t node);
void graph2D_triangulate_bowyer_watson(Graph2D& graph);

void graph2D_triangulate_find_bad_state_draw(Triangulation& triangulation, StateFindBadTriangles& state);

#endif
