#include "../include/graph.hpp"
#include <cstdio>
#include <random>
#include <raylib.h>
#include <raymath.h>
#define RAYGUI_IMPLEMENTATION
#include "../lib/raygui.h"
#include <chrono>
#include <cmath>
#include <malloc.h>
#include <vector>

const Color color_background            = GetColor(0xF7F4EEFF); // warm paper
const Color color_grid_dot              = GetColor(0xE4DFD3FF); // faint background dots
const Color color_mesh_edge             = GetColor(0x2B2E3AFF); // final mesh lines, near-black navy
const Color color_outer_triangle        = Fade(GetColor(0x2B2E3AFF), 0.55f);
const Color color_current_point         = GetColor(0xE0574AFF);
const Color color_walk_current_triangle = Fade(GetColor(0x3EA8DEFF), 0.55f); // sky blue wash
const Color color_bad_id                = Fade(GetColor(0x3EA8DEFF), 0.75f);
const Color color_bad_circle            = GetColor(0x2B2E3AFF);
const Color color_bad_visited           = Fade(GetColor(0x8A93A8FF), 0.45f); // muted slate
const Color color_bad_triangles         = Fade(GetColor(0xE0574AFF), 0.55f); // coral red
const Color color_boundary_edge         = GetColor(0xE0574AFF);
const Color color_panel_bg              = Fade(WHITE, 0.92f);
const Color color_panel_border          = GetColor(0xE4DFD3FF);
const Color color_text_primary          = GetColor(0x2B2E3AFF);
const Color color_text_muted            = GetColor(0x8A93A8FF);

constexpr int minNodes = 0;
constexpr int maxNodes = 100000;
int nNodes             = 1000;

constexpr float minSpeed = 0;       // 1 step per second
constexpr float maxSpeed = 1000.0f; // 1000 steps per second

constexpr int screen_width  = 1920;
constexpr int screen_height = 1080;
Camera2D camera             = {0};

struct SceneState {
    Graph2D graph;
    Triangulation triangulation;

    SceneState() : graph(0) {};
};
static void generate_points(SceneState& state, int count) {
    int old_size = (int) state.graph.adj_list.size();

    if (count < old_size) {
        for (int i = count; i < old_size; i++) {
            state.graph.adj_list[i].clear();
            state.graph.active[i] = false;
        }
    } else if (count > old_size) {
        state.graph.adj_list.resize(count);
        state.graph.active.resize(count, true);
        state.graph.positions.resize(count);
    }

    for (int i = 0; i < count; i++) {
        state.graph.active[i] = true;
        state.graph.adj_list[i].clear();
    }
    state.graph.positions.resize(count);

    static std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    for (int i = 0; i < count; i++) {
        state.graph.positions[i].x = dist01(rng) * screen_width;
        state.graph.positions[i].y = dist01(rng) * screen_height;
    }
}

void update(SceneState& state) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

        camera.offset = GetMousePosition();
        camera.target = mouseWorldPos;

        const float zoomFactor = 1.2f;

        if (wheel > 0)
            camera.zoom *= zoomFactor;
        else
            camera.zoom /= zoomFactor;

        camera.zoom = Clamp(camera.zoom, 0.05f, 100.0f);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        delta         = Vector2Scale(delta, -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
    }
}

static const char* state_name(NodeProcessState type) {
    switch (type) {
        case NodeProcessState::INIT:
            return "Init";
        case NodeProcessState::WALK_TO_CONTAINING:
            return "Walking to containing triangle";
        case NodeProcessState::FIND_BAD_TRIANGLES:
            return "Finding bad triangles";
        case NodeProcessState::BUILD_POLYGON:
            return "Building boundary polygon";
        case NodeProcessState::DELETE_TRIANGLES:
            return "Deleting bad triangles";
        case NodeProcessState::ADD_NEW_TRIANGLES:
            return "Adding new triangles";
    }
    return "";
}

// A soft glowing marker for the point currently being inserted.
static void draw_current_point(Vector2 p) {
    float r = 6.0f / camera.zoom;
    DrawCircleV(p, r, color_current_point);
}

static void draw_filled_triangle(Graph2D& graph, const GraphTriangle& t, Color c) {
    DrawTriangle(
        graph.positions[t[0]],
        graph.positions[t[2]],
        graph.positions[t[1]],
        c);
}

void draw_state_init(SceneState& state) {
    draw_current_point(state.triangulation.current_point);
}
void draw_state_walk_to_containing(SceneState& state) {
    auto& walk_state            = std::get<StateWalkToContaining>(state.triangulation.state.data);
    TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayGet(&state.triangulation.triangles, walk_state.walk_current_triangle_id);
    draw_filled_triangle(state.graph, trig_triangle->triangle, color_walk_current_triangle);
    draw_current_point(state.triangulation.current_point);
}
void draw_state_find_bad_triangles(SceneState& state) {
    auto& bad_state = std::get<StateFindBadTriangles>(state.triangulation.state.data);

    // Visited triangles
    for (uint32_t i = 0; i < state.triangulation.triangles.dataLen; i++) {
        if (!state.triangulation.triangles.active[i] || !bad_state.visited[i])
            continue;
        TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayGet(&state.triangulation.triangles, i);
        draw_filled_triangle(state.graph, trig_triangle->triangle, color_bad_visited);
    }
    // Bad triangles
    for (uint32_t triangle_id : bad_state.bad_triangles) {
        TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayGet(&state.triangulation.triangles, triangle_id);
        draw_filled_triangle(state.graph, trig_triangle->triangle, color_bad_triangles);
    }

    if (bad_state.bad_current_triangle_id == UINT32_MAX) {
        draw_current_point(state.triangulation.current_point);
        return;
    }
    TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayGet(&state.triangulation.triangles, bad_state.bad_current_triangle_id);
    draw_filled_triangle(state.graph, trig_triangle->triangle, color_bad_id);
    DrawCircleLinesV(
        bad_state.bad_current_triangle_circle.center,
        bad_state.bad_current_triangle_circle.radius,
        color_bad_circle);
    draw_current_point(state.triangulation.current_point);
}
void draw_boundary_edges(SceneState& state) {
    auto& bad_state = std::get<StateFindBadTriangles>(state.triangulation.state.data);
    for (auto const& boundary_edge : bad_state.boundary_edges) {
        float lineWidth = 5.0f / camera.zoom;
        DrawLineEx(
            state.graph.positions[boundary_edge.a],
            state.graph.positions[boundary_edge.b],
            lineWidth,
            color_boundary_edge);
    }
    draw_current_point(state.triangulation.current_point);
}
void draw_state_build_polygon(SceneState& state) {
    draw_boundary_edges(state);
}
void draw_state_delete_triangles(SceneState& state) {
    draw_boundary_edges(state);
}
void draw_state_add_new_triangles(SceneState& state) {
    draw_boundary_edges(state);
}

void draw_hud(SceneState& state, double triangulation_time_ms, bool init, float* point_slider, float* speed_slider, bool* points_edit, bool* speed_edit) {
    // Stats card, top-left
    const int pad    = 18;
    const int card_w = 420;
    const int card_h = init ? 110 : 90;
    DrawRectangleRounded({10, 10, (float) card_w, (float) card_h}, 0.18f, 8, color_panel_bg);
    DrawRectangleRoundedLines({10, 10, (float) card_w, (float) card_h}, 0.18f, 8, color_panel_border);

    DrawText("Delaunay Triangulation", 10 + pad, 10 + 14, 20, color_text_primary);
    DrawText(TextFormat("Vertices: %d", (int) state.graph.adj_list.size()), 10 + pad, 10 + 42, 20, color_text_muted);
    DrawText(TextFormat("Build time: %.3f ms", triangulation_time_ms), 10 + pad, 10 + 64, 20, color_text_muted);
    if (init)
        DrawText(TextFormat("Phase: %s", state_name(state.triangulation.state.type)), 10 + pad, 10 + 84, 20, color_text_muted);

    // Points controls
    DrawText("Points:", 30, 120, 20, color_text_primary);
    GuiSlider({30, 150, 230, 20}, NULL, NULL, point_slider, minNodes, maxNodes);

    int pts = (int) *point_slider;
    // We pass 0 as min to GuiValueBox so the user can backspace cleanly
    if (GuiValueBox({270, 150, 60, 20}, NULL, &pts, 0, maxNodes, *points_edit)) {
        *points_edit = !*points_edit;
    }
    *point_slider = (float) pts;

    // Clamp to minimum when NOT in edit mode
    if (!*points_edit && *point_slider < minNodes) {
        *point_slider = minNodes;
    }

    // Speed controls (steps per second)
    DrawText("Auto-run Speed (steps/s):", 30, 190, 20, color_text_primary);
    GuiSlider({30, 220, 230, 20}, NULL, NULL, speed_slider, minSpeed, maxSpeed);

    int spd = (int) *speed_slider;
    // We pass 0 as min to GuiValueBox so the user can backspace cleanly
    if (GuiValueBox({270, 220, 60, 20}, NULL, &spd, 0, (int) maxSpeed, *speed_edit)) {
        *speed_edit = !*speed_edit;
    }
    *speed_slider = (float) spd;

    // Clamp to minimum when NOT in edit mode
    if (!*speed_edit && *speed_slider < minSpeed) {
        *speed_slider = minSpeed;
    }

    // Legend, bottom-left
    struct LegendItem {
        Color color;
        const char* label;
    };
    const LegendItem items[] = {
        {color_walk_current_triangle, "Walking triangle"},
        {color_bad_visited, "Visited"},
        {color_bad_triangles, "Bad (in circumcircle)"},
        {color_bad_id, "Current bad triangle"},
        {color_boundary_edge, "Boundary edge"},
        {color_current_point, "Point being inserted"},
    };
    const int n_items  = sizeof(items) / sizeof(items[0]);
    const int row_h    = 26;
    const int legend_w = 260;
    const int legend_h = n_items * row_h + 24;
    const int legend_x = 10;
    const int legend_y = screen_height - legend_h - 20;

    DrawRectangleRounded({(float) legend_x, (float) legend_y, (float) legend_w, (float) legend_h}, 0.1f, 8, color_panel_bg);
    DrawRectangleRoundedLines({(float) legend_x, (float) legend_y, (float) legend_w, (float) legend_h}, 0.1f, 8, color_panel_border);
    DrawText("Legend", legend_x + 16, legend_y + 10, 20, color_text_primary);
    for (int i = 0; i < n_items; i++) {
        int y = legend_y + 34 + i * row_h;
        DrawRectangleRounded({(float) (legend_x + 16), (float) y, 18, 18}, 0.3f, 6, items[i].color);
        DrawText(items[i].label, legend_x + 44, y + 1, 10, color_text_primary);
    }

    // Controls hint, bottom-right
    const char* hint = "TAB to auto-run  |  SPACE/ENTER to step  |  ESC to quit";
    int hint_w       = MeasureText(hint, 20);
    DrawText(hint, screen_width - hint_w - 24, screen_height - 30, 20, color_text_muted);
}

void draw(SceneState& state, bool it = false, bool init = false) {
    if (it && init) {
        switch (state.triangulation.state.type) {
            case NodeProcessState::INIT:
                draw_state_init(state);
                break;
            case NodeProcessState::WALK_TO_CONTAINING:
                draw_state_walk_to_containing(state);
                break;
            case NodeProcessState::FIND_BAD_TRIANGLES:
                draw_state_find_bad_triangles(state);
                break;
            case NodeProcessState::BUILD_POLYGON:
                draw_state_build_polygon(state);
                break;
            case NodeProcessState::DELETE_TRIANGLES:
                draw_state_delete_triangles(state);
                break;
            case NodeProcessState::ADD_NEW_TRIANGLES:
                draw_state_add_new_triangles(state);
                break;
        }
        for (size_t i = 0; i < state.triangulation.triangles.dataLen; i++) {
            if (!state.triangulation.triangles.active[i])
                continue;
            TrigTriangle* trig_triangle = (TrigTriangle*) SlotArrayGet(&state.triangulation.triangles, i);
            DrawTriangleLines(
                state.graph.positions[trig_triangle->triangle[0]],
                state.graph.positions[trig_triangle->triangle[1]],
                state.graph.positions[trig_triangle->triangle[2]],
                color_outer_triangle);
        }
    }

    for (auto const& edge : graph_get_edges(state.graph)) {
        float lineWidth = 2.0f / camera.zoom;

        DrawLineEx(
            state.graph.positions[edge[0]],
            state.graph.positions[edge[1]],
            lineWidth,
            color_mesh_edge);
    }
    float pointRadius = 2.0f / camera.zoom;
    if (pointRadius >= 1.0f) {
        for (auto const& point : state.graph.positions)
            DrawCircleV(point, pointRadius, BLACK);
    } else {
        for (auto const& point : state.graph.positions)
            DrawPixelV(point, color_mesh_edge);
    }
}

static void step_bowyer_watson(SceneState& state, bool* init, bool* finished, size_t& n) {
    if (!*init) {
        graph2D_triangulate_bowyer_watson_init(
            state.graph,
            state.triangulation);
        *init = true;
        return;
    }
    if (*finished)
        return;
    size_t size;
    size = state.graph.adj_list.size();
    // Process current vertex
    if (graph2D_triangulate_bowyer_watson_it(state.graph, state.triangulation)) {
        if (state.triangulation.process_index >= state.triangulation.process_order.size()) {
            graph2D_triangulate_bowyer_watson_finalize(
                state.graph,
                state.triangulation);
            *finished = true;
        }
    }
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(screen_width, screen_height, "Delaunay Triangulation — Bowyer-Watson");
    SetTargetFPS(120);

    // Camera setup
    camera.target = {
        screen_width * 0.5f,
        screen_height * 0.5f};
    camera.offset = {
        screen_width * 0.5f,
        screen_height * 0.5f};
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f;

    SceneState state;
    size_t n               = 0;
    bool init              = false;
    bool finished          = false;
    bool auto_run          = false;
    float point_slider     = 1000;
    float speed_slider     = 60.0f; // Default 60 steps per second
    float step_accumulator = 0.0f;

    // UI states for inputs
    bool hud         = true;
    bool points_edit = false;
    bool speed_edit  = false;

    int previous_points = 1000;

    generate_points(state, previous_points);
    double triangulation_time_ms = 0.0;

    while (!WindowShouldClose()) {
        update(state);
        if(IsKeyPressed(KEY_H))
            hud = !hud;

        // Only update points if the user isn't typing in the box, to prevent resetting mid-edit
        int requested_points = (int) point_slider;
        if (!points_edit && requested_points != previous_points) {
            previous_points  = requested_points;
            n                = 0;
            init             = false;
            finished         = false;
            auto_run         = false;
            step_accumulator = 0.0f;
            generate_points(state, previous_points);
        }

        // Only process hotkeys if we aren't currently typing in an input box
        if (!points_edit && !speed_edit) {
            if (IsKeyPressed(KEY_ENTER)) {
                double start = GetTime();
                while (!finished)
                    step_bowyer_watson(state, &init, &finished, n);
                double end            = GetTime();
                triangulation_time_ms = (end - start) * 1000.0;
            }

            if (IsKeyPressed(KEY_TAB)) {
                auto_run         = !auto_run;
                step_accumulator = 0.0f; // Reset execution timer on toggle
            }

            if (!auto_run && IsKeyPressed(KEY_SPACE)) {
                step_bowyer_watson(state, &init, &finished, n);
            }

            if (IsKeyPressed(KEY_R)) {
                n                = 0;
                init             = false;
                finished         = false;
                auto_run         = false;
                step_accumulator = 0.0f;
                state            = SceneState();
                generate_points(state, previous_points);
            }
        }

        if (auto_run) {
            // Decouple speed from frame rate using an accumulator (steps per second)
            step_accumulator += speed_slider * GetFrameTime();
            int steps = (int) step_accumulator;
            step_accumulator -= steps;

            for (int i = 0; i < steps; i++) {
                if (finished)
                    break;
                step_bowyer_watson(state, &init, &finished, n);
            }
        }

        BeginDrawing();
        ClearBackground(color_background);
        BeginMode2D(camera);
        draw(state, true, init);
        EndMode2D();
        if(hud)
            draw_hud(state, triangulation_time_ms, init, &point_slider, &speed_slider, &points_edit, &speed_edit);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
//int main(void) {
//    std::chrono::steady_clock::time_point begin;
//    std::chrono::steady_clock::time_point end;
//    SceneState state;
//    //std::vector<int> nPoints = {100, 10000, 100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000, 1000000};
//    std::vector<uint32_t> nPoints = {1000, 10000, 100000, 1000000, 10000000};
//    for(auto const nPoint: nPoints) {
//        generate_points(state, nPoint);
//        begin = std::chrono::steady_clock::now();
//        graph2D_triangulate_bowyer_watson(state.graph);
//        end = std::chrono::steady_clock::now();
//        std::printf("[node=%d] Compute time: %lu ms\n", nPoint, std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
//        malloc_trim(0);
//    }
//}
