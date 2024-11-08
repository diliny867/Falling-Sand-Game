
#include <stdlib.h>
#include <stdio.h>
#include <threads.h>

#include "glad/glad.h"
#include "raylib.h" //recompiled for opengl 4.3
#include "rlgl.h"
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

#define GRID_CELL_SIZE 10.f

fsgame_t* game;
mtx_t game_mutex;
bool app_running = true;
long simulation_interval = 1000000000L / 60;
bool update_ssbo = false;

struct timespec get_curr_time(void) {
    struct timespec current_time;
    timespec_get(&current_time, TIME_UTC);
    return current_time;
}
time_t nsec_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000LL + (start.tv_nsec - end.tv_nsec);
}

time_t time_clamp(time_t val, time_t min, time_t max) {
	if(val < min) {
        return min;
	}
    if(val > max) {
        return val;
    }
    return val;
}
int thread_game_simulate(void* arg) {
    time_t duration;
    struct timespec start_time, end_time, duration_time;
    while(true) {
        if(!app_running) {
            break;
        }
    	start_time = get_curr_time();

        mtx_lock(&game_mutex);
        if(!IsKeyDown(KEY_SPACE)) {
            game_tick(game);
            update_ssbo = true;
        }
        mtx_unlock(&game_mutex);

        end_time = get_curr_time();
        duration = time_clamp(simulation_interval - nsec_diff(start_time, end_time), 0, simulation_interval);
        duration_time = (struct timespec){ duration / 1000000000LL, duration % 1000000000LL };
        thrd_sleep(&duration_time, NULL);
    }
    return 0;
}

void matrix_print(Matrix mat){
    printf("[\n");
    printf("  %f %f %f %f\n", mat.m0, mat.m4, mat.m8, mat.m12);
    printf("  %f %f %f %f\n", mat.m1, mat.m5, mat.m9, mat.m13);
    printf("  %f %f %f %f\n", mat.m2, mat.m6, mat.m10, mat.m14);
    printf("  %f %f %f %f\n", mat.m3, mat.m7, mat.m11, mat.m15);
    printf("]\n");
}
void vector3_print(Vector3 v) {
    printf("%f, %f, %f\n", v.x, v.y, v.z);
}

int main(void) {
    Vector2 screen = (Vector2){1200, 650};

    InitWindow(screen.x, screen.y, "Window title");

    Shader shader_grid_bg = LoadShader("resources/grid_bg.vert", "resources/grid_bg.frag");
    
    mouse_t mouse;
    
    arena_t* arena = arena_new();
    game = game_new(arena);
    game_init(game);

    Vector2 pos = {0, 0};
    Vector2 last_mouse_pos = { 0.0f, 0.0f };
    bool dragging = false;

    Camera2D camera = {0};
    camera.target = pos;
    camera.offset = (Vector2){screen.x/2.0f, screen.y/2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 0.1f;

    Vector2 grid_start = {-GRID_WIDTH / 2.f, -GRID_HEIGHT / 2.f};
    grid_start = Vector2Scale(grid_start, GRID_CELL_SIZE);
    for (int i = 0; i < GRID_SIZE; i++){
        grid[i].width = GRID_CELL_SIZE;
    	grid[i].height = GRID_CELL_SIZE;
        int y = i / GRID_WIDTH;
        int x = i - y * GRID_WIDTH;
        grid[i].x = grid_start.x + x * GRID_CELL_SIZE;
        grid[i].y = grid_start.y + y * GRID_CELL_SIZE;
    }

    float grid_outline_thickness = 5 * GRID_CELL_SIZE;
    Rectangle grid_outline;
    grid_outline.x = grid_start.x - grid_outline_thickness;
    grid_outline.y = grid_start.y - grid_outline_thickness;
    grid_outline.width = GRID_WIDTH * GRID_CELL_SIZE + grid_outline_thickness * 2;
    grid_outline.height = GRID_HEIGHT * GRID_CELL_SIZE + grid_outline_thickness * 2;

    Matrix grid_scale_matrix = MatrixScale(GRID_CELL_SIZE, GRID_CELL_SIZE, 0);
    Matrix grid_translate_matrix = MatrixTranslate(grid_start.x, grid_start.y, 0);
    Matrix grid_model_matrix = MatrixMultiply(grid_scale_matrix, grid_translate_matrix);
    Matrix projection = MatrixOrtho(0, screen.x, screen.y, 0, -1.f, 1.f);

    material_type_e_t current_material = SAND;

    unsigned int grid_ssbo = rlLoadShaderBuffer(GRID_SIZE * sizeof(game->grid[0]), game->grid, 0);
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    unsigned int quadEBO = 0;
    float vertices[] = {
        // Positions         Texcoords
        1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    };
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    quadVAO = rlLoadVertexArray();
    rlEnableVertexArray(quadVAO);
    quadVBO = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
    quadEBO = rlLoadVertexBufferElement(indices, sizeof(indices), false);
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 5*sizeof(float), (void*)0);
	rlEnableVertexAttribute(1);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, 5*sizeof(float), (void *)(3*sizeof(float)));
    rlDisableVertexArray();

    int shader_mvp_loc = GetShaderLocation(shader_grid_bg, "mvp");
    
    int shader_grid_width_loc = rlGetLocationUniform(shader_grid_bg.id, "grid_width");
    int shader_material_colors_loc = rlGetLocationUniform(shader_grid_bg.id, "material_colors");

    Vector4* material_colors = arena_alloc(arena, sizeof(Vector4) * MATERIALS_COUNT);
    for(int i = 0; i < MATERIALS_COUNT; i++) {
        rgba_t col = game->materials[i].color;
        material_colors[i].x = col.r / 255.f;
        material_colors[i].y = col.g / 255.f;
    	material_colors[i].z = col.b / 255.f;
    	material_colors[i].w = col.a / 255.f;
    }

    rlEnableShader(shader_grid_bg.id);
    rlBindShaderBuffer(grid_ssbo, 4);
    int grid_width = GRID_WIDTH;
    rlSetUniform(shader_grid_width_loc, &grid_width, RL_SHADER_UNIFORM_INT, 1);
    rlSetUniform(shader_material_colors_loc, material_colors, RL_SHADER_UNIFORM_VEC4, MATERIALS_COUNT);

    mtx_init(&game_mutex, mtx_plain);
    thrd_t game_thread;
    thrd_create(&game_thread, thread_game_simulate, NULL);

    SetTargetFPS(60);
    
    while ((app_running = !WindowShouldClose())) {
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

        if(IsKeyPressed(KEY_DOWN)) {
            current_material--;
            current_material = clampi(current_material, 0, MATERIALS_COUNT - 1);
        }
        if(IsKeyPressed(KEY_UP)) {
            current_material++;
            current_material = clampi(current_material, 0, MATERIALS_COUNT - 1);
        }

        if(mouse.left.down) {
            Vector2i grid_pos = world_to_grid(mouse.pos_world, grid_start, GRID_CELL_SIZE);
            //game_place(game, SAND, grid_pos.x, grid_pos.y, 50, 20, true);
            mtx_lock(&game_mutex);
            game_place(game, current_material, grid_pos.x, grid_pos.y, 10, 20, true);
            rlUpdateShaderBuffer(grid_ssbo, game->grid, GRID_SIZE * sizeof(game->grid[0]), 0);
            mtx_unlock(&game_mutex);
        }

        BeginDrawing();

        ClearBackground((Color){ 30, 30, 30, 255 });

        if(update_ssbo){
            rlUpdateShaderBuffer(grid_ssbo, game->grid, GRID_SIZE * sizeof(game->grid[0]), 0);
            update_ssbo = false;
        }

        Matrix model_view = GetCameraMatrix2D(camera);
        model_view = MatrixMultiply(grid_model_matrix, model_view);
        Matrix mvp = MatrixMultiply(model_view, projection);
        rlEnableShader(shader_grid_bg.id);
        rlSetUniformMatrix(shader_mvp_loc, mvp);
        rlEnableVertexArray(quadVAO);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, GRID_SIZE);
        rlDisableVertexArray();

        BeginMode2D(camera);

        //for(int i = 0; i < GRID_SIZE; i++) {
        //    color = rgba_to_color(game->materials[game->grid[i]].color);
        //    DrawRectangleRec(grid[i], color);
        //}

        DrawRectangleLinesEx(grid_outline, grid_outline_thickness, (Color){80, 80, 80, 255});
        
        EndMode2D();

        DrawFPS(10, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
