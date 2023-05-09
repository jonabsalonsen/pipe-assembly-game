#pragma once

#include "common.h"
#include "Renderer.h"
#include "EntityManager.h"

class Game {
    public:
        Game();
        
        void init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
        
        void game_loop();
        void handleEvents();
        void handleInputs();
        void update();
        void render();
        void clean();
        bool running() {return isRunning;}
        static SDL_Event event;

    private:
        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<EntityManager> entityManager;
        int FPS = 60;
        int frameDelay = 1000 / FPS;
        unsigned int frameStart;
        int frameTime;
        int count;
        bool isRunning = false;
        SDL_Window* window;
        SDL_GLContext GL_context;
};