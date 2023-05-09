#include "Game.hpp"

SDL_Event Game::event;

Game::Game()
{
    
}

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen) {

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if(fullscreen) {
        flags = SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) == 0) {
        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        GL_context = SDL_GL_CreateContext(window);
        GlCall(glEnable(GL_BLEND));
        GlCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        SDL_GL_SetSwapInterval(1);
        entityManager = std::make_unique<EntityManager>();
        renderer = std::make_unique<Renderer>();
        isRunning = true;
    }
}

void Game::game_loop()
{
    if (!isRunning)
        return;

    frameStart = SDL_GetTicks();

    this->handleEvents();
    this->update();
    this->render();

    frameTime = SDL_GetTicks() - frameStart;

    if (frameTime < frameDelay)
        SDL_Delay(frameDelay - frameTime);
}

void Game::handleEvents() 
{
    SDL_PollEvent(&event);
    switch (event.type) {
        case SDL_QUIT:
            isRunning = false;
            break;
        default:
            break;
    }
}

void Game::handleInputs() {
    if (event.key.repeat != 0 || event.key.type != SDL_KEYDOWN)
        return;

    switch(event.key.keysym.sym)
    {
        case SDLK_w:
            entityManager->MoveAllToAdjacent(UP);
            break;
        case SDLK_d:
            entityManager->MoveAllToAdjacent(RIGHT);
            break;
        case SDLK_s:
            entityManager->MoveAllToAdjacent(DOWN);
            break;
        case SDLK_a:
            entityManager->MoveAllToAdjacent(LEFT);
            break;
        case SDLK_LEFT:
            entityManager->RotateAll(LEFT);
            break;
        case SDLK_RIGHT:
            entityManager->RotateAll(RIGHT);
            break;
        default:
            break;
    }
}

void Game::update() 
{
    renderer->UpdateGraphicsData(entityManager);
    handleInputs();
}

void Game::render() 
{
    glClear(GL_COLOR_BUFFER_BIT);
    renderer->Draw();
    SDL_GL_SwapWindow(window);
}

void Game::clean() 
{
    SDL_GL_DeleteContext(GL_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "Game cleaned" << std::endl;
}