
#include <stdlib.h>
#include <stdio.h>
#include <threads.h>
//define it so compiler dont complain, also add compiler flag /experimental:c11atomics 
#define _Atomic(v) v
#include <stdatomic.h>

#include "glad/glad.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#define STB_INCLUDE_IMPLEMENTATION
#define STB_INCLUDE_LINE_GLSL
#include "../include/stb_include.h"

#include "game.h"

//#define SEPARATE_GAME_THREAD

typedef struct {
    uint8_t press : 1, release : 1, down : 1;
} mouse_button_t;
typedef struct {
    Vector2 pos;
    Vector2 pos_world;
    float d_wheel;
    mouse_button_t left, middle, right;
} mouse_t;

typedef struct Vector2i{
    int x, y;
} Vector2i;
Vector2 vec2i_to_vec2(Vector2i v) {
    return (Vector2){(float)v.x, (float)v.y};
}
Vector2i vec2_to_vec2i(Vector2 v) {
    return (Vector2i){(int)v.x, (int)v.y};
}

Vector2i world_to_grid(Vector2 pos, Vector2 grid_start, float grid_cell_size) {
    Vector2 out = Vector2Subtract(pos, grid_start);
    out = Vector2Scale(out, 1.f / grid_cell_size);
    return vec2_to_vec2i(out);
}

#define GRID_CELL_SIZE 10.f

typedef _Atomic(material_type_e_t) atomic_material_type_e_t;
typedef _Atomic(Vector2) atomic_Vector2;
typedef struct {
    Vector2 grid_start;
    atomic_Vector2 mouse_pos_world;
    atomic_long simulation_interval;
    atomic_material_type_e_t current_material;
    atomic_int place_size;
    atomic_int scatter_size;
    atomic_bool place;
    atomic_bool tick;
} shared_game_place_data_t;
shared_game_place_data_t shared_game_place_data;

fsgame_t* game;
mtx_t game_mutex;
atomic_bool app_running = true;

struct timespec get_curr_time(void) {
    struct timespec current_time;
    timespec_get(&current_time, TIME_UTC);
    return current_time;
}

#ifdef SEPARATE_GAME_THREAD
int thread_game_simulate(void* arg) {
    long duration;
    struct timespec start_time, end_time, duration_time;
    while(app_running) {
    	start_time = get_curr_time();
        mtx_lock(&game_mutex);

        if(shared_game_place_data.place) {
            Vector2i grid_pos = world_to_grid(shared_game_place_data.mouse_pos_world, shared_game_place_data.grid_start, GRID_CELL_SIZE);
            //game_place(game, SAND, grid_pos.x, grid_pos.y, 50, 20, true);
            game_place(game, shared_game_place_data.current_material, grid_pos.x, grid_pos.y, 10, 20, true);
            //game_place(game, shared_game_place_data.current_material, grid_pos.x, grid_pos.y, 2, 0, false);
            shared_game_place_data.place = false;
        }

        if(shared_game_place_data.tick) {
            game_tick(game);
            shared_game_place_data.tick = false;
        }

        end_time = get_curr_time();
        duration = (end_time.tv_sec - start_time.tv_sec) * 1000000000L - start_time.tv_nsec + end_time.tv_nsec;

        if(duration > shared_game_place_data.simulation_interval) {
            duration = 0;
        }else {
            duration = shared_game_place_data.simulation_interval - duration;
        }
        duration_time = (struct timespec){ 0, duration };

        mtx_unlock(&game_mutex);

        thrd_sleep(&duration_time, NULL);
    }
    return 0;
}
#endif

unsigned int glLoadVertexBuffer(void* data, size_t size, int usage) {
    unsigned int id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    return id;
}
void glUpdateVertexBuffer(unsigned int id, void* data, size_t size, int usage) {
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, usage); // orphan buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

Shader load_rl_shader_multiple_files(char* vs_file_name, char* fs_file_name, char* path_to_includes) {
	char* vs_str = NULL;
    char* fs_str = NULL;

    char error[256];

    if(vs_file_name != NULL) {
        vs_str = stb_include_file(vs_file_name, "", path_to_includes, error);
    }
    if(fs_file_name != NULL){
        fs_str = stb_include_file(fs_file_name, "", path_to_includes, error);
    }

    Shader shader = LoadShaderFromMemory(vs_str, fs_str);

    if(vs_str){
        free(vs_str);
    }
    if(fs_str){
        free(fs_str);
    }

    return shader;
}


void matrix_print(Matrix mat){
    printf("%f %f %f %f\n", mat.m0, mat.m4, mat.m8, mat.m12);
    printf("%f %f %f %f\n", mat.m1, mat.m5, mat.m9, mat.m13);
    printf("%f %f %f %f\n", mat.m2, mat.m6, mat.m10, mat.m14);
    printf("%f %f %f %f\n", mat.m3, mat.m7, mat.m11, mat.m15);
}
void vector3_print(Vector3 v) {
    printf("%f, %f, %f\n", v.x, v.y, v.z);
}

//Vector2 grid_xy_offsets[GRID_SIZE];

int main(void) {
    Vector2 screen = (Vector2){1200, 650};

    mouse_t mouse;

    InitWindow(screen.x, screen.y, "Window title");
    SetWindowState(FLAG_WINDOW_RESIZABLE);


    Shader shader_grid_bg = load_rl_shader_multiple_files("resources/shaders/grid_bg.vert", "resources/shaders/grid_bg.frag", "resources/shaders");

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

    Vector2 move_bounds_offset = {100.f, 100.f};
    Rectangle move_bounds = {grid_start.x - move_bounds_offset.x, grid_start.y - move_bounds_offset.y,
    	GRID_WIDTH * GRID_CELL_SIZE + move_bounds_offset.x * 2, GRID_HEIGHT * GRID_CELL_SIZE + move_bounds_offset.y * 2};

    shared_game_place_data.current_material = SAND;
    shared_game_place_data.simulation_interval = 1000000000L / 60;
    shared_game_place_data.grid_start = grid_start;
    shared_game_place_data.mouse_pos_world = (Vector2){0, 0};
    shared_game_place_data.place_size = 10;
    shared_game_place_data.scatter_size = 10;
    shared_game_place_data.place = false;
    shared_game_place_data.tick = true;

    float grid_outline_thickness = 5 * GRID_CELL_SIZE;
    Rectangle grid_outline;
    grid_outline.x = grid_start.x - grid_outline_thickness;
    grid_outline.y = grid_start.y - grid_outline_thickness;
    grid_outline.width = GRID_WIDTH * GRID_CELL_SIZE + grid_outline_thickness * 2;
    grid_outline.height = GRID_HEIGHT * GRID_CELL_SIZE + grid_outline_thickness * 2;

    Matrix grid_scale_matrix = MatrixScale(GRID_CELL_SIZE, GRID_CELL_SIZE, 0);
    Matrix grid_translate_matrix = MatrixTranslate(grid_start.x, grid_start.y, 0);
    Matrix grid_model_matrix = MatrixMultiply(grid_scale_matrix, grid_translate_matrix);

    //for(int y = 0; y < GRID_HEIGHT; y++) {
    //    for(int x = 0; x < GRID_WIDTH; x++) {
    //        grid_xy_offsets[x + y * GRID_WIDTH] = (Vector2){(float)x, (float)y};
    //    }
    //}
    float vertices[] = {
        // Positions         Texcoords
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    };
    unsigned int quad_vao, quad_vbo, grid_material_vbo, grid_temperature_vbo, grid_flag_vbo; // , grid_y_offsets_vbo;
    quad_vao = rlLoadVertexArray();
    rlEnableVertexArray(quad_vao);
    quad_vbo = rlLoadVertexBuffer(vertices, sizeof(vertices), false);
    rlEnableVertexAttribute(0);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 5*sizeof(float), (void*)0);
	rlEnableVertexAttribute(1);
    rlSetVertexAttribute(1, 2, RL_FLOAT, false, 5*sizeof(float), (void *)(3*sizeof(float)));
    //grid_material_vbo = rlLoadVertexBuffer(game->grid, GRID_SIZE * sizeof(game->grid[0]), false);
    grid_material_vbo = glLoadVertexBuffer(game->grid, GRID_SIZE * sizeof(game->grid[0]), GL_STREAM_DRAW);
    rlEnableVertexAttribute(2);
    glVertexAttribIPointer(2, 1, GL_INT, 1*sizeof(int), (void *)(0));
    rlSetVertexAttributeDivisor(2, 1);
    grid_temperature_vbo = glLoadVertexBuffer(game->temperatures, GRID_SIZE * sizeof(game->temperatures[0]), GL_STREAM_DRAW);
    rlEnableVertexAttribute(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 1*sizeof(float), (void *)(0));
    rlSetVertexAttributeDivisor(3, 1);
    grid_flag_vbo = glLoadVertexBuffer(game->flags, GRID_SIZE * sizeof(game->flags[0]), GL_STREAM_DRAW);
    rlEnableVertexAttribute(4);
    glVertexAttribIPointer(4, 1, GL_INT, 1*sizeof(int), (void *)(0));
    rlSetVertexAttributeDivisor(4, 1);
    //grid_y_offsets_vbo = glLoadVertexBuffer(grid_xy_offsets, sizeof(grid_xy_offsets), GL_STREAM_DRAW);
    //rlEnableVertexAttribute(5);
    //glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    //rlSetVertexAttributeDivisor(5, 1);
    rlDisableVertexArray();

    int shader_mvp_loc = GetShaderLocation(shader_grid_bg, "mvp");

    int shader_material_colors_loc = rlGetLocationUniform(shader_grid_bg.id, "material_colors");
    int shader_grid_sizes_loc = rlGetLocationUniform(shader_grid_bg.id, "grid_size");

    Vector4* material_colors = arena_alloc(arena, sizeof(Vector4) * MATERIALS_COUNT);
    for(int i = 0; i < MATERIALS_COUNT; i++) {
        rgba_t col = game->materials[i].color;
        material_colors[i].x = col.r / 255.f;
        material_colors[i].y = col.g / 255.f;
    	material_colors[i].z = col.b / 255.f;
    	material_colors[i].w = col.a / 255.f;
    }

    rlEnableShader(shader_grid_bg.id);
    rlSetUniform(shader_material_colors_loc, material_colors, RL_SHADER_UNIFORM_VEC4, MATERIALS_COUNT);
    Vector2i grid_size = (Vector2i){GRID_WIDTH, GRID_HEIGHT};
    rlSetUniform(shader_grid_sizes_loc, &grid_size, RL_SHADER_UNIFORM_IVEC2, 1);

#ifdef SEPARATE_GAME_THREAD
    mtx_init(&game_mutex, mtx_plain);
    thrd_t game_thread;
    thrd_create(&game_thread, thread_game_simulate, NULL);
#endif

    SetTargetFPS(60);
    
    while ((app_running = !WindowShouldClose())) {
        screen.x = (float)GetScreenWidth();
        screen.y = (float)GetScreenHeight();
        camera.offset = (Vector2){screen.x * 0.5f, screen.y * 0.5f};

        mouse.pos = GetMousePosition();
        //printf("%f %f\n", mouse.pos.x, mouse.pos.y);
        mouse.pos_world = GetScreenToWorld2D(mouse.pos, camera);
        mouse.left = (mouse_button_t){IsMouseButtonPressed(MOUSE_BUTTON_LEFT), IsMouseButtonReleased(MOUSE_BUTTON_LEFT), IsMouseButtonDown(MOUSE_BUTTON_LEFT)};
        mouse.middle = (mouse_button_t){IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE), IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE), IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)};
        mouse.right = (mouse_button_t){IsMouseButtonPressed(MOUSE_BUTTON_RIGHT), IsMouseButtonReleased(MOUSE_BUTTON_RIGHT), IsMouseButtonDown(MOUSE_BUTTON_RIGHT)};
        mouse.d_wheel = GetMouseWheelMove();
        
        Vector2 mouse_world_pre_zoom = GetScreenToWorld2D(mouse.pos, camera);

        bool can_zoom = true;
        if(IsKeyDown(KEY_LEFT_CONTROL)){
            shared_game_place_data.place_size += mouse.d_wheel;
            if(shared_game_place_data.place_size < 1) {
                shared_game_place_data.place_size = 1;
            }
            if((int)mouse.d_wheel != 0){
                printf("Place size: %d\n", shared_game_place_data.place_size);
            }
            can_zoom = false;
        }
        if(IsKeyDown(KEY_LEFT_SHIFT)) {
            shared_game_place_data.scatter_size += mouse.d_wheel;
            if(shared_game_place_data.scatter_size < 0) {
                shared_game_place_data.scatter_size = 0;
            }
            if((int)mouse.d_wheel != 0){
                printf("Scatter size: %d\n", shared_game_place_data.scatter_size);
            }
            can_zoom = false;
        }
        if(IsKeyDown(KEY_LEFT_ALT)) {
            can_zoom = false;
            shared_game_place_data.current_material += mouse.d_wheel;
            shared_game_place_data.current_material = clampi(shared_game_place_data.current_material, AIR, MATERIALS_COUNT - 1);
            if((int)mouse.d_wheel != 0) {
                printf("Material chosen: %s\n", MATERIAL_ENUM_STRING[shared_game_place_data.current_material]);
            }
        }
    	if(can_zoom) {
            camera.zoom += mouse.d_wheel * 0.035f;
        }
        camera.zoom = Clamp(camera.zoom, 0.075f, 5.0f);

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
        camera.target.x = Clamp(camera.target.x, move_bounds.x, move_bounds.x + move_bounds.width);
        camera.target.y = Clamp(camera.target.y, move_bounds.y, move_bounds.y + move_bounds.height);

        if (mouse.left.down) {
            shared_game_place_data.mouse_pos_world = mouse.pos_world;
            shared_game_place_data.place = true;
        }

        if(IsKeyPressed(KEY_DOWN)) {
            shared_game_place_data.current_material--;
            shared_game_place_data.current_material = clampi(shared_game_place_data.current_material, 0, MATERIALS_COUNT - 1);
            printf("Material chosen: %s\n", MATERIAL_ENUM_STRING[shared_game_place_data.current_material]);
        }
        if(IsKeyPressed(KEY_UP)) {
            shared_game_place_data.current_material++;
            shared_game_place_data.current_material = clampi(shared_game_place_data.current_material, 0, MATERIALS_COUNT - 1);
            printf("Material chosen: %s\n", MATERIAL_ENUM_STRING[shared_game_place_data.current_material]);
        }
        if (IsKeyPressed(KEY_F11)){
            ToggleBorderlessWindowed();
        }
        if(!IsKeyDown(KEY_SPACE)) {
            shared_game_place_data.tick = true;
        }
        if(IsKeyPressed(KEY_RIGHT)) {
            shared_game_place_data.tick = true;
        }

#ifndef SEPARATE_GAME_THREAD
        if(shared_game_place_data.place) {
            Vector2i grid_pos = world_to_grid(shared_game_place_data.mouse_pos_world, shared_game_place_data.grid_start, GRID_CELL_SIZE);
            //game_place(game, SAND, grid_pos.x, grid_pos.y, 50, 20, true);
            game_place(game, shared_game_place_data.current_material, grid_pos.x, grid_pos.y, shared_game_place_data.place_size, shared_game_place_data.scatter_size, true);
            //game_place(game, shared_game_place_data.current_material, grid_pos.x, grid_pos.y, 2, 0, false);
            shared_game_place_data.place = false;
        }

        if(shared_game_place_data.tick) {
            game_tick(game);
            shared_game_place_data.tick = false;
        }
#endif

        BeginDrawing();

        ClearBackground((Color){ 30, 30, 30, 255 });

        glUpdateVertexBuffer(grid_material_vbo, game->grid, GRID_SIZE * sizeof(game->grid[0]), GL_STREAM_DRAW);
        glUpdateVertexBuffer(grid_temperature_vbo, game->temperatures, GRID_SIZE * sizeof(game->temperatures[0]), GL_STREAM_DRAW);
        glUpdateVertexBuffer(grid_flag_vbo, game->flags, GRID_SIZE * sizeof(game->flags[0]), GL_STREAM_DRAW);

        Matrix model_view = GetCameraMatrix2D(camera);
        Matrix projection = MatrixOrtho(0, screen.x, screen.y, 0, -1.f, 1.f);
        model_view = MatrixMultiply(grid_model_matrix, model_view);
        Matrix mvp = MatrixMultiply(model_view, projection);
        rlEnableShader(shader_grid_bg.id);
        rlSetUniformMatrix(shader_mvp_loc, mvp);
        rlEnableVertexArray(quad_vao);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GRID_SIZE);
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

#ifdef SEPARATE_GAME_THREAD
    int res;
    thrd_join(game_thread, &res);
    mtx_destroy(&game_mutex);
#endif

    rlUnloadVertexArray(quad_vao);
    rlUnloadVertexBuffer(quad_vbo);
    rlUnloadVertexBuffer(grid_material_vbo);

    arena_free(arena);

    CloseWindow();

    return 0;
}
