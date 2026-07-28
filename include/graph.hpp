#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "geometry.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

/* ==== GraphList ==== */
struct GraphList {
    std::vector<std::vector<uint32_t>> adj_list;
    std::vector<bool> active;
    GraphList(int n) {
        adj_list.resize(n);
        active.resize(n, true);
    }
};
template <std::size_t N>
using GraphListPolygon  = std::array<uint32_t, N>;
using GraphListEdge     = GraphListPolygon<2>;
using GraphListTriangle = GraphListPolygon<3>;
// GraphList Edge
void graph_add_edge_one(GraphList& graph, int node1, int node2);
void graph_add_edge(GraphList& graph, int node1, int node2);
void graph_rm_edge_one(GraphList& graph, int node1, int node2);
void graph_rm_edge(GraphList& graph, int node1, int node2);
bool graph_get_edge(GraphList& graph, int node1, int node2);
std::vector<GraphListEdge> graph_get_edges(GraphList& graph);
// GraphList Vertex
uint32_t graph_add_vertex(GraphList& graph);
void graph_rm_vertex(GraphList& graph, int node);
// GraphList TriangulationNaive (that doesn't require position)
void graph_triangulate_fan(GraphList& graph);

/* ==== GraphList2D ==== */
struct GraphList2D : GraphList {
    std::vector<Vector2D> positions;
    GraphList2D(int n) : GraphList(n) {
        positions.resize(n);
    };
};
// GraphList2D utils
Vector2D graph2D_get_min_bound(GraphList2D& graph);
Vector2D graph2D_get_max_bound(GraphList2D& graph);
Triangle graph2D_get_super_triangle(GraphList2D& graph, double epsilon = 10.f);
double graph2D_get_average_distance(GraphList2D& graph);
struct GridEntry {
    uint32_t point;
    uint32_t next;
};
std::vector<uint32_t> graph2D_sort_grid_indices(GraphList2D& graph, Vector2D cell_dims);
// GraphList2D vertex
uint32_t graph2D_add_vertex(GraphList2D& graph, Vector2D pos);

#endif
