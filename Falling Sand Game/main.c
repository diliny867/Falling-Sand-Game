
#include <stdlib.h>
#include <stdio.h>

#include "glad/glad.h"
#include "raylib.h"
#include "raymath.h"

#include "game.h"

typedef struct {
    uint8_t press : 1, release : 1, down : 1;
} mouse_button_t;
typedef struct {
    Vector2 pos;
    Vector2 pos_world;
    mouse_button_t left, middle, right;
} mouse_t;

Rectangle grid[GRID_SIZE] = { 0 };

typedef struct Vector2i{
    int x, y;
} Vector2i;
Vector2 Vec2iToVec2(Vector2i v) {
    return (Vector2){(float)v.x, (float)v.y};
}
Vector2i Vec2ToVec2i(Vector2 v) {
    return (Vector2i){(int)v.x, (int)v.y};
}

Vector2i world_to_grid(Vector2 pos, Vector2 grid_start, float grid_cell_size) {
    Vector2 out = Vector2Subtract(pos, grid_start);
    out = Vector2Scale(out, 1.f / grid_cell_size);
    return Vec2ToVec2i(out);
}

Color rgba_to_color(rgba_t rgba) {
    return (Color){ rgba.r, rgba.g, rgba.b, rgba.a };
}

int main(void) {
    Vector2 screen = (Vector2){800, 450};
    
    InitWindow(screen.x, screen.y, "Window title");

    mouse_t mouse;

    arena_t* arena = arena_new();
    fsgame_t* game = game_new(arena);
    game_init(game);

    Vector2 pos = {0, 0};
    Vector2 last_mouse_pos = { 0.0f, 0.0f };
    bool dragging = false;

    Camera2D camera = {0};
    camera.target = pos;
    camera.offset = (Vector2){screen.x/2.0f, screen.y/2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 0.1f;

    float grid_cell_size = 10;
    Vector2 grid_start = {-GRID_WIDTH / 2, -GRID_HEIGHT / 2};
    grid_start = Vector2Scale(grid_start, grid_cell_size);
    for (int i = 0; i < GRID_SIZE; i++){
        grid[i].width = grid_cell_size;
    	grid[i].height = grid_cell_size;
        int y = i / GRID_WIDTH;
        int x = i - y * GRID_WIDTH;
        grid[i].x = grid_start.x + x * grid_cell_size - grid_cell_size / 2;
        grid[i].y = grid_start.y + y * grid_cell_size - grid_cell_size / 2;
    }

    float grid_outline_thickness = 5;
    Rectangle grid_outline;
    grid_outline.x = grid_start.x - grid_outline_thickness / 2;
    grid_outline.y = grid_start.y - grid_outline_thickness / 2;
    grid_outline.width = GRID_WIDTH * grid_cell_size + grid_outline_thickness / 2;
    grid_outline.height = GRID_HEIGHT * grid_cell_size + grid_outline_thickness / 2;

    Color color;
    
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        mouse.pos = GetMousePosition();
        //printf("%f %f\n", mouse.pos.x, mouse.pos.y);
        mouse.pos_world = GetScreenToWorld2D(mouse.pos, camera);
        mouse.left = (mouse_button_t){IsMouseButtonPressed(MOUSE_BUTTON_LEFT), IsMouseButtonReleased(MOUSE_BUTTON_LEFT), IsMouseButtonDown(MOUSE_BUTTON_LEFT)};
        mouse.middle = (mouse_button_t){IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE), IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE), IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)};
        mouse.right = (mouse_button_t){IsMouseButtonPressed(MOUSE_BUTTON_RIGHT), IsMouseButtonReleased(MOUSE_BUTTON_RIGHT), IsMouseButtonDown(MOUSE_BUTTON_RIGHT)};

        Vector2 mouse_world_pre_zoom = GetScreenToWorld2D(mouse.pos, camera);

        camera.zoom += GetMouseWheelMove() * 0.035f;
        if(camera.zoom < 0.01f) {
            camera.zoom = 0.01f;
        }
        if(camera.zoom > 5.0f) {
            camera.zoom = 5.0f;
        }

        Vector2 zoom_offset = Vector2Subtract(mouse_world_pre_zoom, GetScreenToWorld2D(mouse.pos, camera));
        camera.target = Vector2Add(camera.target, zoom_offset);

        if (mouse.right.press) {
            last_mouse_pos = mouse.pos;
            dragging = true;
        }
        if (mouse.right.release) {
            dragging = false;
        }
        if (dragging) {
            Vector2 mouse_delta = Vector2Subtract(last_mouse_pos, mouse.pos);
            mouse_delta = Vector2Scale(mouse_delta, 1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, mouse_delta);
            last_mouse_pos = mouse.pos;
        }

        if(mouse.left.down) {
            Vector2i grid_pos = world_to_grid(mouse.pos_world, grid_start, grid_cell_size);
            game_place(game, SAND, grid_pos.x, grid_pos.y, 50, 20, true);
        }

        if(!IsKeyDown(KEY_SPACE)) {
            game_tick(game);
        }

        BeginDrawing();

        ClearBackground((Color){ 30, 30, 30, 255 });

        BeginMode2D(camera);

        for(int i = 0; i < GRID_SIZE; i++) {
            color = rgba_to_color(game->materials[game->grid[i]].color);
            DrawRectangleRec(grid[i], color);
        }
        
        DrawRectangleLinesEx(grid_outline, grid_outline_thickness * grid_cell_size, (Color){80, 80, 80, 255});

        EndMode2D();

        DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}