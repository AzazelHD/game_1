#include "engine/core/App.h"
#include "engine/scene/Scene.h"
#include "config/GameConstants.h"
#include "scenes/BootState.h"
#include <SDL3/SDL.h>

#include <exception>
#include <stdexcept>

// main() is the only place the game and engine are wired together.
// Keep it minimal — all real logic lives in App and the states.
//
// App accepts an optional scene factory so the game can provide its first scene
// without editing engine internals.
//
// TODO: Wrap it in a try/catch(std::exception&) and show a message box on error:
//       SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", e.what(), nullptr);
//
// Note: on Windows, SDL redefines main as SDL_main. Including "engine/core/App.h"
// will indirectly pull in SDL_main.h. Make sure SDL3::SDL3main is linked (see CMakeLists).

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    try
    {
        App app("TRPG", GameConstants::VIEW_W, GameConstants::VIEW_H, []
                {
                    auto *sceneStack = App::getSceneStack();
                    auto *renderer = App::getRenderer();

                    if (!sceneStack)
                    {
                        throw std::runtime_error("Scene stack is not initialized");
                    }

                    return std::unique_ptr<Scene>(
                        std::make_unique<BootState>(*sceneStack, renderer)); });
        app.run();
        return 0;
    }
    catch (const std::exception &e)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", e.what(), nullptr);
        return 1;
    }
}
