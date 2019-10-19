#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <string>

#include "singlefile/CircularArray.h"

namespace
{
	constexpr unsigned int windowWidth = 800;
	constexpr unsigned int windowHeight = 600;
    
    SDL_Window * initSDL(const std::string & appName)
    {
        SDL_Window * window = nullptr;
        
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0)
            return window;
        
        window = SDL_CreateWindow(appName.c_str(),
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  windowWidth,
                                  windowHeight,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
        {
            const char * error = SDL_GetError();
            SDL_LogError(0, "Error = %s\n", error);
            
        }
        
        return window;
        
    }
}


int main(int argc, char *argv[])
{
    std::string appName("Raytracing In One Weekend");
    SDL_Window * window = initSDL(appName);
    
    // creating a renderer
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x7F, 0xFF);
    

	CircularArray<60, float> fpsCounter;
    bool gameIsRunning = true;
    while (gameIsRunning)
    {
		static unsigned int frameCount = 0;
		Uint32 currentTicks = SDL_GetTicks();
        static Uint32 lastTicks = currentTicks;
        
        Uint32 delta = currentTicks - lastTicks;
		const float secondsDelta = float(delta) / 1000.0f;
		if (delta > 0)
        {
            fpsCounter.addValue(secondsDelta);
            lastTicks = currentTicks;
        }
        
        static char windowTitleBuffer[1024];
        const float average = fpsCounter.average();
        const int fpsAverage = (average>FLT_EPSILON) ? (int)(1.0f / average) : 0;
        sprintf(windowTitleBuffer, "%s. FPS: %d", appName.c_str(), fpsAverage);
        SDL_SetWindowTitle(window, windowTitleBuffer);
        
        SDL_Event event;
        while (SDL_PollEvent(&event) > 0)
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    gameIsRunning = false;
                    break;
                case SDL_KEYUP:
                {
                    SDL_KeyboardEvent & key = event.key;
                    switch (key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            gameIsRunning = false;
                            break;
                    }
                    break;
                }
				case SDL_WINDOWEVENT:
				{
					switch (event.window.event)
					{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
					case SDL_WINDOWEVENT_RESIZED:
						break;
					}
				}

            }
        }

        SDL_RenderPresent(renderer);
		frameCount++;
    }

    return 0;
}