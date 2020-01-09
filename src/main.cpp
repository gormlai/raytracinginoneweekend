#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <string>

#include "singlefile/CircularArray.h"
#include "singlefile/SDL_FileStream.h"
#include "vulkan-setup/VulkanSetup.h"

#include "Mesh.h"

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
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
        {
            const char * error = SDL_GetError();
            SDL_LogError(0, "Error = %s\n", error);
            
        }
        
        return window;
    }

    std::vector<char> readFile(const std::string file)
    {
        SDL_FileStream stream(file, (unsigned int)SDL_FileStream::OpenFlags::READ_ONLY | (unsigned int)SDL_FileStream::OpenFlags::BINARY);

        std::vector<char> data;
        stream.read(data);
        return data;
    }

    bool readShaders(std::vector<Vulkan::Shader>& shaders)
    {
        SDL_LogInfo(0, "Loading Shaders!\n");
        unsigned int numShaders = (unsigned int)shaders.size();
        for (unsigned int i = 0; i < numShaders; i++)
        {
            const std::string& shaderName = shaders[i]._filename;
            SDL_LogInfo(0, "\tLoading Shader: %s!\n", shaderName.c_str());

            std::vector<char> byteCode = readFile(shaderName);
            if (!byteCode.empty())
                shaders[i]._byteCode = byteCode;
        }
        return true;
    }


}




int main(int argc, char *argv[])
{
    std::string appName("Raytracing In One Weekend");

    SDL_Window * window = initSDL(appName);

    Vulkan::AppDescriptor appDesc;
    appDesc._appName = appName;
    appDesc._requiredVulkanVersion = VK_API_VERSION_1_1;
    appDesc._window = window;

    std::vector<Vulkan::Shader> shaders =
    {
        {"triangle_vert.spv", Vulkan::ShaderType::Vertex},
        {"triangle_frag.spv", Vulkan::ShaderType::Fragment},
    };
    readShaders(shaders);
    appDesc._shaders = shaders;


    glm::vec3 camPos{ 0,0,8 };
    glm::vec3 camDir{ 0,0,-1 };
    glm::vec3 camUp{ 0,1,0 };
    appDesc._cameraUpdateFunction = [&camPos, &camDir, &camUp](glm::vec3& pos, glm::vec3& lookat, glm::vec3& up)
    {
        pos = camPos;
        lookat = camPos + camDir;
        up = camUp;
    };


    //    Mesh mesh = MeshFactory::createPlane(1.0f, 1.0f);

    appDesc._backgroundClearColor = []() { return glm::vec4{ 0.2f, 0.2f, 0.2f, 1.0 }; };

    glm::quat totalRot;
    appDesc._updateModelMatrix = [&totalRot](const void* userData, float timePassed, float deltaTime)
    {
        glm::mat4 rVal;
        if (userData != nullptr)
        {
            const Mesh* mesh = static_cast<const Mesh*>(userData);
            rVal = mesh->getWorldMatrix();
        }

        return rVal;
    };

    appDesc._getBindingDescription = []() { return MeshVertex::getBindingDescription(); };
    appDesc._getAttributes = []() { return MeshVertex::getAttributes(); };

    appDesc._updateFunction = [&totalRot](float timePassed, float deltaTime)
    {
        glm::quat xRot = glm::rotate(timePassed * 0.8f * glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat yRot = glm::rotate(timePassed * 0.6f * glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat zRot = glm::rotate(timePassed * glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
        //        totalRot = xRot * yRot * zRot;
        return true;
    };


    Vulkan::VulkanContext context;
    if(!Vulkan::handleVulkanSetup(appDesc, context))
    {
        SDL_LogError(0, "Failed to handle vulkan setup\n");
        return 1;
    }

    // creating a renderer
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    SDL_Texture * texture = SDL_CreateTexture(renderer,
                                              SDL_PIXELFORMAT_RGBA8888,
                                              0,
                                              windowWidth,
                                              windowHeight);
    
    static char pixelBuffer[windowWidth][windowHeight][4];
    memset(pixelBuffer, 0xFF, sizeof(pixelBuffer));

	CircularArray<60, double> fpsCounter;
    bool gameIsRunning = true;
    while (gameIsRunning)
    {
		static unsigned int frameCount = 0;
		Uint64 currentTicks = SDL_GetPerformanceCounter();
        static Uint64 lastTicks = currentTicks;

        constexpr float TICKS_RESOLUTION = 1.0f;
        Uint64 delta = currentTicks - lastTicks;
		const double secondsDelta = double(delta) / SDL_GetPerformanceFrequency();
		if (delta > 0)
        {
            fpsCounter.addValue(secondsDelta);
            lastTicks = currentTicks;
        }
        
        constexpr int WINDOW_TITLE_BUFFER_SIZE = 1024;
        static char * windowTitleBuffer = new char[WINDOW_TITLE_BUFFER_SIZE];
        const double average = fpsCounter.average();
        const int fpsAverage = (average>DBL_EPSILON) ? (int)(1.0 / average) : 0;
        snprintf(windowTitleBuffer, WINDOW_TITLE_BUFFER_SIZE, "%s. FPS: %d", appName.c_str(), fpsAverage);
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

        SDL_UpdateTexture(texture, NULL, (void*)pixelBuffer, 0);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
		frameCount++;
    }

    return 0;
}
