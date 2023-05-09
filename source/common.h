#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_opengles2.h>
#include <emscripten/emscripten.h>
#include <iostream>
#include <memory>
#include <algorithm>
#include <math.h>

#define GlCall(x) GlClearError();\
    x;\
    GlLogCall(#x, __FILE__, __LINE__)


static void GlClearError() {
    while (glGetError() != GL_NO_ERROR);
}

static bool GlLogCall(const char* function, const char* file, int line) {
    while (GLenum error = glGetError()) 
    {
        std::cout << "[OpenGL Error] (" << error << "): " << function <<
            " " << file << ": " << line << std::endl;
        return false;
    }
    return true;
}

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define PIXEL_WIDTH 800
#define PIXEL_HEIGHT 640

#define TILES_COLUMNS 20
#define TILES_ROWS 16
#define TILE_SIZE PIXEL_HEIGHT / TILES_ROWS
#define NUMBER_OF_TILES TILES_COLUMNS * TILES_ROWS
#define POSITIONS_LENGTH NUMBER_OF_TILES * 2
#define ORIENTATIONS_LENGTH NUMBER_OF_TILES
#define ANGLES_LENGTH NUMBER_OF_TILES
#define PIPE_TYPE_LENGTH NUMBER_OF_TILES

enum class VertexAttributeType {
    GRID_POSITION, ORIENTATION, ANGLE, Count
};

#define VERTEX_ATTR_COUNT static_cast<int>(VertexAttributeType::Count)

enum class EntityType {
    BACKGROUND, PLAYER, BENT_PIPE, STRAIGHT_PIPE, BOX, Count
};

#define ENTITY_TYPE_COUNT static_cast<int>(EntityType::Count)

enum Direction {
    UP, LEFT, DOWN, RIGHT
};

enum class ShaderType {
    NONE,
    PIPE_SHADOW,
    PIPE,
    BACKGROUND,
    Count
};

#define SHADER_TYPE_COUNT static_cast<int>(ShaderType::Count)

struct RotationCounts {
    unsigned int leftRotations;
    unsigned int rightRotations;
};

struct posf {
    union {
        struct {
            float x, y;
        };
        struct {
            float array[2];
        };
    };
};

struct pos {
    int x, y;
};

struct Push {
    float priority;
    posf fromPosition;
    Direction direction;
};