#include "../include/graph.hpp"


// Predicates
static double robust_orient(Vector2D a, Vector2D b, Vector2D p) {
    double pa[2] = { (double)a.x, (double)a.y };
    double pb[2] = { (double)b.x, (double)b.y };
    double pp[2] = { (double)p.x, (double)p.y };
    return orient2d(pa, pb, pp);
}
static double robust_incircle(Vector2D a, Vector2D b, Vector2D c, Vector2D d) {
    double pa[2] = { (double)a.x, (double)a.y };
    double pb[2] = { (double)b.x, (double)b.y };
    double pc[2] = { (double)c.x, (double)c.y };
    double pd[2] = { (double)d.x, (double)d.y };
    return incircle(pa, pb, pc, pd);
}

/* Graph */
// Edge operation
void graph_add_edge_one(Graph& graph, int node1, int node2) {
    std::vector<uint32_t>& adj_row = graph.adj_list[node1];
    if (std::find(adj_row.begin(), adj_row.end(), node2) != adj_row.end())
        return;
    adj_row.push_back(node2);
}
void graph_add_edge(Graph& graph, int node1, int node2) {
    if (!graph.active[node1] || !graph.active[node2])
        return;
    graph_add_edge_one(graph, node1, node2);
    graph_add_edge_one(graph, node2, node1);
}
void graph_rm_edge_one(Graph& graph, int node1, int node2) {
    std::vector<uint32_t>& adj_row = graph.adj_list[node1];
    auto it                        = std::find(adj_row.begin(), adj_row.end(), node2);
    if (it != adj_row.end())
        adj_row.erase(it);
}
void graph_rm_edge(Graph& graph, int node1, int node2) {
    graph_rm_edge_one(graph, node1, node2);
    graph_rm_edge_one(graph, node2, node1);
}
bool graph_get_edge(Graph& graph, int node1, int node2) {
    std::vector<uint32_t>& adj_row = graph.adj_list[node1];

    return std::find(adj_row.begin(), adj_row.end(), node2) != adj_row.end();
}
std::vector<GraphEdge> graph_get_edges(Graph& graph) {
    std::vector<GraphEdge> edges;

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
uint32_t graph_add_vertex(Graph& graph) {
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
void graph_rm_vertex(Graph& graph, int node) {
    for (int neighbor : graph.adj_list[node])
        graph_rm_edge_one(graph, neighbor, node);
    graph.active[node] = false;
}
// TriangulationNaive (that doesn't require position)
void graph_triangulate_fan(Graph& graph) {
    for (size_t i = 1; i < graph.adj_list.size() - 1; i++) {
        if (!graph.active[i])
            continue;
        graph_add_edge(graph, 0, i);
        graph_add_edge(graph, 0, i + 1);
        graph_add_edge(graph, i, i + 1);
    }
}
/* Graph2D */
// Graph2D utils
Vector2D graph2D_get_min_bound(Graph2D& graph) {
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
Vector2D graph2D_get_max_bound(Graph2D& graph) {
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
Triangle graph2D_get_super_triangle(Graph2D& graph, double epsilon) {
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
double graph2D_get_average_distance(Graph2D& graph) {
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
std::vector<uint32_t> graph2D_sort_grid_indices(Graph2D& graph, Vector2D cell_dims) {
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
// Graph2D vertex
uint32_t graph2D_add_vertex(Graph2D& graph, Vector2D pos) {
    uint32_t node;

    node = graph_add_vertex(graph);
    if (node < graph.positions.size())
        graph.positions[node] = pos;
    else
        graph.positions.push_back(pos);

    return node;
}

// Graph2D triangulation
// Helpers
//static double orient(Vector2D a, Vector2D b, Vector2D p) {
//    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
//}
static double orient_canonical(Graph2D& graph, uint32_t u, uint32_t v, const Vector2D& p) {
    bool swapped = u > v;
    if (swapped)
        std::swap(u, v);
    double s = robust_orient(graph.positions[u], graph.positions[v], p);
    return swapped ? -s : s;
}
static bool check_sign_equal(double val1, double val2, double val3) {
    return (val1 >= 0 && val2 >= 0 && val3 >= 0) || (val1 < 0 && val2 < 0 && val3 < 0);
}
static bool check_sign_different(double val1, double val2, double val3) {
    return (val1 < 0 && val2 >= 0 && val3 >= 0) || (val1 >= 0 && val2 < 0 && val3 < 0);
}
GraphTriangle make_oriented_triangle(Graph2D& graph, uint32_t a, uint32_t b, uint32_t c) {
    double s = robust_orient(graph.positions[a], graph.positions[b], graph.positions[c]);
    if (s < 0)
        std::swap(b, c);
    return {a, b, c};
}
int check_point_position(const std::array<double, 3>& signs) {
    if (signs[0] < 0)
        return 0;
    else if (signs[1] < 0)
        return 1;
    else if (signs[2] < 0)
        return 2;
    else
        return 3;
    return -1;
}
int trigtriangle_get_edge_index(const TrigTriangle& trig_triangle, uint32_t node1, uint32_t node2) {
    for (size_t edge = 0; edge < 3; edge++) {
        size_t edge2 = (edge + 1) % 3;
        if ((trig_triangle.triangle[edge] == node1 && trig_triangle.triangle[edge2] == node2) ||
            (trig_triangle.triangle[edge2] == node1 && trig_triangle.triangle[edge] == node2))
            return edge;
    }
    printf("Not normal\n");
    assert(false);
    return -1;
}
// WALK_TO_CONTAINING,
void graph2D_triangulate_walk_state_init(Triangulation& triangulation, StateWalkToContaining& state) {
    state.walk_current_triangle_id = triangulation.last_inserted;
}
uint32_t graph2D_triangulate_walk_state_it(Graph2D& graph, Triangulation& triangulation, StateWalkToContaining& state) {
    const Vector2D& point = triangulation.current_point;
    std::array<double, 3> signs;
    TrigTriangle& graph_triangle = *(TrigTriangle*) SlotArrayGet(&triangulation.triangles, state.walk_current_triangle_id);
    Triangle triangle            = {
        graph.positions[graph_triangle.triangle[0]],
        graph.positions[graph_triangle.triangle[1]],
        graph.positions[graph_triangle.triangle[2]]};
    for (size_t i = 0; i < 3; i++)
        signs[i] = orient_canonical(graph, graph_triangle.triangle[i], graph_triangle.triangle[(i + 1) % 3], point);

    int offset   = (int) (state.walk_current_triangle_id % 3);
    int position = -1;
    for (int k = 0; k < 3; k++) {
        int i = (offset + k) % 3;
        if (signs[i] < 0) {
            position = i;
            break;
        }
    }
    if (position == -1)
        return state.walk_current_triangle_id;

    return graph_triangle.neighbors[position];
}
// FIND_BAD_TRIANGLES
void graph2D_triangulate_find_bad_state_init(Triangulation& triangulation, StateFindBadTriangles& state, uint32_t start_triangle_id) {
    state.boundary_edges.clear();
    state.bad_triangles.clear();
    state.to_visit          = {};
    state.visited           = std::vector<bool>(triangulation.triangles.dataLen, false);
    state.bad_triangles_map = std::vector<bool>(triangulation.triangles.dataLen, false);
    if (start_triangle_id == UINT32_MAX)
        return;
    state.to_visit.push(start_triangle_id);
    state.visited[start_triangle_id] = true;
}
void graph2D_triangulate_find_bad_state_it(Graph2D& graph, Triangulation& triangulation, StateFindBadTriangles& state) {
    state.bad_current_triangle_id = state.to_visit.front();
    TrigTriangle& graph_triangle  = *(TrigTriangle*) SlotArrayGet(&triangulation.triangles, state.bad_current_triangle_id);
    state.to_visit.pop();

    state.bad_current_triangle_circle = graph_triangle.circle;
    double res = robust_incircle(
            graph.positions[graph_triangle.triangle[0]],
            graph.positions[graph_triangle.triangle[1]],
            graph.positions[graph_triangle.triangle[2]],
            triangulation.current_point
            );
    if (res > 0) {
        state.bad_triangles.push_back(state.bad_current_triangle_id);
        state.bad_triangles_map[state.bad_current_triangle_id] = true;
        for (auto neighbor : graph_triangle.neighbors) {
            if (neighbor != UINT32_MAX && !state.visited[neighbor]) {
                state.to_visit.push(neighbor);
                state.visited[neighbor] = true;
            }
        }
    }
}

void graph2D_triangulate_bowyer_watson_init(Graph2D& graph, Triangulation& triangulation) {
    exactinit();
    Triangle super_triangle_pts;
    double avg_dist;

    avg_dist                    = graph2D_get_average_distance(graph);
    triangulation.process_order = graph2D_sort_grid_indices(graph, {avg_dist * 4.0f, avg_dist * 4.0f});
    triangulation.process_index = 0;
    super_triangle_pts          = graph2D_get_super_triangle(graph);
    for (int i = 0; i < 3; i++) {
        uint32_t node_id                = graph2D_add_vertex(graph, super_triangle_pts[i]);
        triangulation.super_triangle[i] = node_id;
    }
    triangulation.super_triangle = make_oriented_triangle(
        graph,
        triangulation.super_triangle[0],
        triangulation.super_triangle[1],
        triangulation.super_triangle[2]);

    for (size_t i = 0; i < 3; i++)
        graph.active[triangulation.super_triangle[i]] = false;

    SlotArrayInit(&triangulation.triangles, graph.active.size() * 2, sizeof(TrigTriangle));
    TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayInsertGetIndex(&triangulation.triangles, &triangulation.last_inserted);
    trig_triangle->triangle     = triangulation.super_triangle;
    trig_triangle->circle       = triangle_get_circumcircle(super_triangle_pts);
    trig_triangle->neighbors    = {UINT32_MAX, UINT32_MAX, UINT32_MAX};

    triangulation.state.type = NodeProcessState::INIT;
    triangulation.state.data = StateInit{};
}
void graph2D_triangulate_bowyer_watson_finalize(Graph2D& graph, Triangulation& triangulation) {
    std::vector<size_t> indexes_to_delete;
    const GraphTriangle& super_triangle = triangulation.super_triangle;

    for (size_t i = 0; i < 3; i++)
        graph_rm_vertex(graph, triangulation.super_triangle[i]);

    for (size_t i = 0; i < triangulation.triangles.dataLen; i++) {
        if (!triangulation.triangles.active[i])
            continue;
        const TrigTriangle& graph_triangle = *(TrigTriangle*) SlotArrayGet(&triangulation.triangles, i);
        const GraphTriangle& triangle      = graph_triangle.triangle;
        for (size_t j = 0; j < 3; j++) {
            if (triangle[j] == super_triangle[0] || triangle[j] == super_triangle[1] || triangle[j] == super_triangle[2]) {
                indexes_to_delete.push_back(i);
                break;
            }
        }
    }
    for (size_t i = 0; i < triangulation.triangles.dataLen; i++) {
        if (!triangulation.triangles.active[i])
            continue;
        const TrigTriangle& graph_triangle = *(TrigTriangle*) SlotArrayGet(&triangulation.triangles, i);
        const GraphTriangle& triangle      = graph_triangle.triangle;
        graph_add_edge(graph, triangle[0], triangle[1]);
        graph_add_edge(graph, triangle[0], triangle[2]);
        graph_add_edge(graph, triangle[1], triangle[2]);
    }
}
bool graph2D_triangulate_bowyer_watson_it(Graph2D& graph, Triangulation& triangulation) {
    switch (triangulation.state.type) {
        case NodeProcessState::INIT: {
            triangulation.current_node = triangulation.process_order[triangulation.process_index];
            triangulation.current_point = graph.positions[triangulation.current_node];
            triangulation.state.type    = NodeProcessState::WALK_TO_CONTAINING;
            auto& walk_state            = triangulation.state.data.emplace<StateWalkToContaining>();
            graph2D_triangulate_walk_state_init(triangulation, walk_state);
            return false;
        }
        case NodeProcessState::WALK_TO_CONTAINING: {
            auto& walk_state = std::get<StateWalkToContaining>(triangulation.state.data);
            uint32_t next_id = graph2D_triangulate_walk_state_it(graph, triangulation, walk_state);
            if (next_id == walk_state.walk_current_triangle_id || next_id == UINT32_MAX) {
                uint32_t start_triangle_id = (next_id == UINT32_MAX) ? UINT32_MAX : walk_state.walk_current_triangle_id;
                triangulation.state.type   = NodeProcessState::FIND_BAD_TRIANGLES;
                auto& find_state           = triangulation.state.data.emplace<StateFindBadTriangles>();
                graph2D_triangulate_find_bad_state_init(triangulation, find_state, start_triangle_id);
            } else
                walk_state.walk_current_triangle_id = next_id;
            return false;
        }
        case NodeProcessState::FIND_BAD_TRIANGLES: {
            auto& find_state = std::get<StateFindBadTriangles>(triangulation.state.data);
            if (!find_state.to_visit.empty()) {
                graph2D_triangulate_find_bad_state_it(graph, triangulation, find_state);
                return false;
            }
            triangulation.state.type = NodeProcessState::BUILD_POLYGON;
            return false;
        }
        case NodeProcessState::BUILD_POLYGON: {
            auto& find_state = std::get<StateFindBadTriangles>(triangulation.state.data);
            for (size_t i = 0; i < find_state.bad_triangles.size(); i++) {
                TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayGet(&triangulation.triangles, find_state.bad_triangles[i]);
                for (size_t edge = 0; edge < 3; edge++) {
                    uint32_t neighbor = trig_triangle->neighbors[edge];
                    if (neighbor == UINT32_MAX || !find_state.bad_triangles_map[neighbor])
                        find_state.boundary_edges.push_back((BoundaryEdge) {
                            trig_triangle->triangle[edge],
                            trig_triangle->triangle[(edge + 1) % 3],
                            neighbor});
                }
            }
            triangulation.state.type = NodeProcessState::DELETE_TRIANGLES;
            return false;
        }
        case NodeProcessState::DELETE_TRIANGLES: {
            auto& find_state = std::get<StateFindBadTriangles>(triangulation.state.data);
            for (auto const& triangle_id : find_state.bad_triangles)
                SlotArrayDel(&triangulation.triangles, triangle_id);
            triangulation.state.type = NodeProcessState::ADD_NEW_TRIANGLES;
            return false;
        }
        case NodeProcessState::ADD_NEW_TRIANGLES: {
            auto& find_state = std::get<StateFindBadTriangles>(triangulation.state.data);
            std::unordered_map<uint32_t, uint32_t> node_to_triangle;
            for (auto const& boundary_edge : find_state.boundary_edges) {
                TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayInsertGetIndex(&triangulation.triangles, &triangulation.last_inserted);
                trig_triangle->triangle     = make_oriented_triangle(graph, triangulation.current_node, boundary_edge.a, boundary_edge.b);

                Triangle triangle = {
                    graph.positions[trig_triangle->triangle[0]],
                    graph.positions[trig_triangle->triangle[1]],
                    graph.positions[trig_triangle->triangle[2]]};
                trig_triangle->circle = triangle_get_circumcircle(triangle);

                size_t edge = trigtriangle_get_edge_index(*trig_triangle, boundary_edge.a, boundary_edge.b);
                for (size_t i = 0; i < 3; i++) {
                    if (i == edge)
                        trig_triangle->neighbors[i] = boundary_edge.triangle_outside;
                    else
                        trig_triangle->neighbors[i] = UINT32_MAX;
                }
                if (boundary_edge.triangle_outside != UINT32_MAX && triangulation.triangles.active[boundary_edge.triangle_outside]) {
                    TrigTriangle* trig_triangle_neighbor             = (TrigTriangle*) SlotArrayGet(&triangulation.triangles, boundary_edge.triangle_outside);
                    size_t edge_neighbor                             = trigtriangle_get_edge_index(*trig_triangle_neighbor, boundary_edge.a, boundary_edge.b);
                    trig_triangle_neighbor->neighbors[edge_neighbor] = triangulation.last_inserted;
                }

                for (auto const v : {boundary_edge.a, boundary_edge.b}) {
                    auto it = node_to_triangle.find(v);
                    if (it == node_to_triangle.end())
                        node_to_triangle[v] = triangulation.last_inserted;
                    else {
                        TrigTriangle* trig_triangle_neighbor             = (TrigTriangle*) SlotArrayGet(&triangulation.triangles, node_to_triangle[v]);
                        size_t edge                                      = trigtriangle_get_edge_index(*trig_triangle, triangulation.current_node, v);
                        size_t edge_neighbor                             = trigtriangle_get_edge_index(*trig_triangle_neighbor, triangulation.current_node, v);
                        trig_triangle->neighbors[edge]                   = node_to_triangle[v];
                        trig_triangle_neighbor->neighbors[edge_neighbor] = triangulation.last_inserted;
                    }
                }
            }
            triangulation.process_index++;
            triangulation.state.type = NodeProcessState::INIT;
            triangulation.state.data = StateInit{};
            return true;
        }
    }
    return true; // unreachable
}
void graph2D_triangulate_bowyer_watson(Graph2D& graph) {
    Triangulation triangulation;

    graph2D_triangulate_bowyer_watson_init(graph, triangulation);
    while (triangulation.process_index < triangulation.process_order.size())
        while (!graph2D_triangulate_bowyer_watson_it(graph, triangulation))
            ;
    graph2D_triangulate_bowyer_watson_finalize(graph, triangulation);
    SlotArrayTerminate(&triangulation.triangles);
}
