#include "../include/graph.hpp"


// Predicates

/* GraphList */
// Edge operation
void graph_add_edge_one(GraphList& graph, int node1, int node2) {
    std::vector<uint32_t>& adj_row = graph.adj_list[node1];
    if (std::find(adj_row.begin(), adj_row.end(), node2) != adj_row.end())
        return;
    adj_row.push_back(node2);
}
void graph_add_edge(GraphList& graph, int node1, int node2) {
    if (!graph.active[node1] || !graph.active[node2])
        return;
    graph_add_edge_one(graph, node1, node2);
    graph_add_edge_one(graph, node2, node1);
}
void graph_rm_edge_one(GraphList& graph, int node1, int node2) {
    std::vector<uint32_t>& adj_row = graph.adj_list[node1];
    auto it                        = std::find(adj_row.begin(), adj_row.end(), node2);
    if (it != adj_row.end())
        adj_row.erase(it);
}
void graph_rm_edge(GraphList& graph, int node1, int node2) {
    graph_rm_edge_one(graph, node1, node2);
    graph_rm_edge_one(graph, node2, node1);
}
bool graph_get_edge(GraphList& graph, int node1, int node2) {
    std::vector<uint32_t>& adj_row = graph.adj_list[node1];

    return std::find(adj_row.begin(), adj_row.end(), node2) != adj_row.end();
}
std::vector<GraphListEdge> graph_get_edges(GraphList& graph) {
    std::vector<GraphListEdge> edges;

    for (uint32_t i = 0; i < graph.adj_list.size(); i++) {
        auto const& row = graph.adj_list[i];
        for (auto const& node : row) {
            if (i < node)
                edges.push_back({i, node});
        }
    }

    return edges;
}
// Vertex operation
uint32_t graph_add_vertex(GraphList& graph) {
    for (int i = 0; i < graph.adj_list.size(); i++) {
        if (!graph.active[i]) {
            graph.adj_list[i].clear();
            graph.active[i] = true;
            return i;
        }
    }
    graph.adj_list.push_back({});
    graph.active.push_back(true);

    return graph.adj_list.size() - 1;
}
void graph_rm_vertex(GraphList& graph, int node) {
    for (int neighbor : graph.adj_list[node])
        graph_rm_edge_one(graph, neighbor, node);
    graph.active[node] = false;
}
// TriangulationNaive (that doesn't require position)
void graph_triangulate_fan(GraphList& graph) {
    for (size_t i = 1; i < graph.adj_list.size() - 1; i++) {
        if (!graph.active[i])
            continue;
        graph_add_edge(graph, 0, i);
        graph_add_edge(graph, 0, i + 1);
        graph_add_edge(graph, i, i + 1);
    }
}
/* GraphList2D */
// GraphList2D utils
Vector2D graph2D_get_min_bound(GraphList2D& graph) {
    Vector2D min;
    int i_start;

    i_start = 0;
    while (!graph.active[i_start])
        i_start++;
    min = graph.positions[i_start];
    for (size_t i = 1; i < graph.positions.size(); i++) {
        if (!graph.active[i])
            continue;
        if (graph.positions[i].x < min.x)
            min.x = graph.positions[i].x;
        if (graph.positions[i].y < min.y)
            min.y = graph.positions[i].y;
    }

    return min;
}
Vector2D graph2D_get_max_bound(GraphList2D& graph) {
    Vector2D max;
    int i_start;

    i_start = 0;
    while (!graph.active[i_start])
        i_start++;
    max = graph.positions[i_start];
    for (size_t i = 1; i < graph.positions.size(); i++) {
        if (!graph.active[i])
            continue;
        if (graph.positions[i].x > max.x)
            max.x = graph.positions[i].x;
        if (graph.positions[i].y > max.y)
            max.y = graph.positions[i].y;
    }
    return max;
}
// From: https://fr.wikipedia.org/wiki/Algorithme_de_Bowyer-Watson
Triangle graph2D_get_super_triangle(GraphList2D& graph, double epsilon) {
    Vector2D pos_min;
    Vector2D pos_max;
    Triangle super_triangle;

    pos_min           = graph2D_get_min_bound(graph);
    pos_max           = graph2D_get_max_bound(graph);
    super_triangle[0] = {pos_min.x - epsilon, pos_min.y - epsilon};
    super_triangle[1] = {
        pos_min.x + 2 * (pos_max.x - pos_min.x) + 3 * epsilon,
        pos_min.y - epsilon,
    };
    super_triangle[2] = {
        pos_min.x - epsilon,
        pos_min.y + 2 * (pos_max.y - pos_min.y) + 3 * epsilon};

    return super_triangle;
}
double graph2D_get_average_distance(GraphList2D& graph) {
    if (graph.positions.size() < 2)
        return 0.0f;
    Vector2D min = graph2D_get_min_bound(graph);
    Vector2D max = graph2D_get_max_bound(graph);
    double area =
        (max.x - min.x) *
        (max.y - min.y);
    double density = graph.positions.size() / area;
    return sqrtf(1.0f / density);
}
std::vector<uint32_t> graph2D_sort_grid_indices(GraphList2D& graph, Vector2D cell_dims) {
    Vector2D min_bound;
    Vector2D max_bound;
    int grid_x;
    int grid_y;
    int cell_count;
    std::vector<uint32_t> cell_heads;
    std::vector<GridEntry> entries;

    min_bound  = graph2D_get_min_bound(graph);
    max_bound  = graph2D_get_max_bound(graph);
    grid_x     = (int) ((max_bound.x - min_bound.x) / cell_dims.x) + 1;
    grid_y     = (int) ((max_bound.y - min_bound.y) / cell_dims.y) + 1;
    cell_count = grid_x * grid_y;
    cell_heads = std::vector<uint32_t>(cell_count, UINT32_MAX);
    entries.resize(graph.positions.size());

    // Fill buckets
    for (uint32_t i = 0; i < graph.positions.size(); i++) {
        if(!graph.active[i])
            continue;
        const Vector2D& p = graph.positions[i];
        int x            = (int) ((p.x - min_bound.x) / cell_dims.x);
        int y            = (int) ((p.y - min_bound.y) / cell_dims.y);
        x                = std::clamp(x, 0, grid_x - 1);
        y                = std::clamp(y, 0, grid_y - 1);
        int cell         = y * grid_x + x;
        entries[i].point = i;
        entries[i].next  = cell_heads[cell];
        cell_heads[cell] = i;
    }
    std::vector<uint32_t> order;
    order.reserve(graph.positions.size());
    for (int y = 0; y < grid_y; y++) {
        for (int i = 0; i < grid_x; i++) {
            int x          = (y & 1) ? grid_x - 1 - i : i;
            uint32_t entry = cell_heads[y * grid_x + x];
            while (entry != UINT32_MAX) {
                order.push_back(entries[entry].point);
                entry = entries[entry].next;
            }
        }
    }

    return order;
}
// GraphList2D vertex
uint32_t graph2D_add_vertex(GraphList2D& graph, Vector2D pos) {
    uint32_t node;

    node = graph_add_vertex(graph);
    if (node < graph.positions.size())
        graph.positions[node] = pos;
    else
        graph.positions.push_back(pos);

    return node;
}
