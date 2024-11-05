
#include <stdlib.h>
#include <stdio.h>

#include "game.h"
#include "raylib.h"
#include "raymath.h"

Rectangle grid[GRID_SIZE] = { 0 };
Color grid_colors[GRID_SIZE] = { 0 };

int main(void) {

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Window title");

    Vector2 pos = {0, 0};
    Vector2 last_mouse_pos = { 0.0f, 0.0f };
    bool dragging = false;

    Camera2D camera = {0};
    camera.target = pos;
    camera.offset = (Vector2){screenWidth/2.0f, screenHeight/2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    float grid_draw_size = 10;
    for (int i = 0; i < GRID_SIZE; i++){
        grid[i].width = grid_draw_size;
    	grid[i].height = grid_draw_size;
        int y = i / GRID_WIDTH;
        int x = i - y * GRID_WIDTH;
        grid[i].x = (-GRID_WIDTH / 2 + x) * grid_draw_size - grid_draw_size / 2;
        grid[i].y = (-GRID_HEIGHT / 2 + y) * grid_draw_size - grid_draw_size / 2;

        grid_colors[i] = (Color){ GetRandomValue(200, 240), GetRandomValue(200, 240), GetRandomValue(200, 250), 255 };
    }

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        Vector2 mouse_pre_zoom = GetMousePosition();
        camera.zoom += GetMouseWheelMove() * 0.035f;
        if(camera.zoom < 0.01f) {
            camera.zoom = 0.01f;
        }
        if(camera.zoom > 5.0f) {
            camera.zoom = 5.0f;
        }
        Vector2 zoom_move = Vector2Subtract(GetScreenToWorld2D(mouse_pre_zoom, camera), GetScreenToWorld2D(GetMousePosition(), camera));
        camera.target = Vector2Add(camera.target, zoom_move);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            last_mouse_pos = GetMousePosition();
            dragging = true;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            dragging = false;
        }
        if (dragging) {
            Vector2 mouse_delta = Vector2Subtract(last_mouse_pos, GetMousePosition());
            mouse_delta = Vector2Scale(mouse_delta, 1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, mouse_delta);
            last_mouse_pos = GetMousePosition();
        }

        BeginDrawing();

        ClearBackground((Color){ 30, 30, 30, 255 });

        BeginMode2D(camera);

        for(int i = 0; i < GRID_SIZE; i++) {
            DrawRectangleRec(grid[i], grid_colors[i]);
        }

        EndMode2D();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}