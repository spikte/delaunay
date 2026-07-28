#include "../include/bowyer_watson.hpp"

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

// GraphList2D triangulation
// Helpers
//static double orient(Vector2D a, Vector2D b, Vector2D p) {
//    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
//}
static double orient_canonical(GraphList2D& graph, uint32_t u, uint32_t v, const Vector2D& p) {
    bool swapped = u > v;
    if (swapped)
        std::swap(u, v);
    double s = robust_orient(graph.positions[u], graph.positions[v], p);
    return swapped ? -s : s;
}
GraphListTriangle make_oriented_triangle(GraphList2D& graph, uint32_t a, uint32_t b, uint32_t c) {
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
uint32_t graph2D_triangulate_walk_state_it(GraphList2D& graph, Triangulation& triangulation, StateWalkToContaining& state) {
    const Vector2D& point = triangulation.current_point;
    std::array<double, 3> signs;
    TrigTriangle& graph_triangle = *(TrigTriangle*) SlotArrayGet(&triangulation.triangles, state.walk_current_triangle_id);
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
void graph2D_triangulate_find_bad_state_it(GraphList2D& graph, Triangulation& triangulation, StateFindBadTriangles& state) {
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

void graph2D_triangulate_bowyer_watson_init(GraphList2D& graph, Triangulation& triangulation) {
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
void graph2D_triangulate_bowyer_watson_finalize(GraphList2D& graph, Triangulation& triangulation) {
    std::vector<size_t> indexes_to_delete;
    const GraphListTriangle& super_triangle = triangulation.super_triangle;

    for (size_t i = 0; i < 3; i++)
        graph_rm_vertex(graph, triangulation.super_triangle[i]);

    for (size_t i = 0; i < triangulation.triangles.dataLen; i++) {
        if (!triangulation.triangles.active[i])
            continue;
        const TrigTriangle& graph_triangle = *(TrigTriangle*) SlotArrayGet(&triangulation.triangles, i);
        const GraphListTriangle& triangle      = graph_triangle.triangle;
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
        const GraphListTriangle& triangle      = graph_triangle.triangle;
        graph_add_edge(graph, triangle[0], triangle[1]);
        graph_add_edge(graph, triangle[0], triangle[2]);
        graph_add_edge(graph, triangle[1], triangle[2]);
    }
}
bool graph2D_triangulate_bowyer_watson_it(GraphList2D& graph, Triangulation& triangulation) {
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
void graph2D_triangulate_bowyer_watson(GraphList2D& graph) {
    Triangulation triangulation;

    graph2D_triangulate_bowyer_watson_init(graph, triangulation);
    while (triangulation.process_index < triangulation.process_order.size())
        while (!graph2D_triangulate_bowyer_watson_it(graph, triangulation))
            ;
    graph2D_triangulate_bowyer_watson_finalize(graph, triangulation);
    SlotArrayTerminate(&triangulation.triangles);
}
