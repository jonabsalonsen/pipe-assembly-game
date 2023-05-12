# Pipe Assembly Game
**Pipe Assembly** is a puzzle game in which pipe pieces are combined and rotated in order to create a larger pipe. Still a work in progress.

The game is written in C++ using SDL2 and OpenGL and has been compiled to WebAssembly using Emscripten on Ubuntu. All graphics are generated using GLSL shaders without the use of textures. Since the game compiles to WebAssembly, it can be played directly in the browser. Try the playable demo here: https://jonabsalonsen.github.io/PipeAssemblyPost.html

![alt text](https://github.com/jonabsalonsen/pipe-assembly-game/blob/main/game_footage.gif "in-game footage")

## How to install and run

In a Linux environment (e.g. Ubuntu on WSL), download and install the [Emscripten SDK](https://github.com/emscripten-core/emsdk)

clone this project

    git clone https://github.com/jonabsalonsen/pipe-assembly-game.git
    cd <path/to/pipe-assembly-game>

Activate PATH and other environment variables in the current terminal

    source <path/to/emsdk>/emsdk_env.sh
    
Compile using the Emscripten compiler

    emcc ./source/*.cpp -o index.html -s USE_SDL=2
    
Start a local server using the following Emscripten command:

    emrun ./index.html --port 8080 --no_browser
    
(the '--no_browser' flag is necessary in WSL, since WSL has no GUI. If using standalone Linux, this can be omitted)

Open a browser and navigate to the local address:

    http://localhost:8080/
    
The game should appear in the browser.

    
