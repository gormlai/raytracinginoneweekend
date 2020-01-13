#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <string>
#include <cstddef>

#include "singlefile/CircularArray.h"
#include "singlefile/SDL_FileStream.h"
#include "vulkan-setup/VulkanSetup.h"

#include "Mesh.h"
#include "Raytracer.h"

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

void render(Vulkan::AppDescriptor & appDesc, Vulkan::VulkanContext & context, bool recreateSwapChain)
{
    const unsigned currentFrame = context._currentFrame;
    bool earlyOut = false;
    if (recreateSwapChain)
    {
        Vulkan::recreateSwapChain(appDesc, context);
//        earlyOut = true;
    }

    const VkResult waitForFencesResult = vkWaitForFences(context._device, 1, &context._fences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    assert(waitForFencesResult == VK_SUCCESS);
    const VkResult resetFencesResult = vkResetFences(context._device, 1, &context._fences[currentFrame]);
    assert(resetFencesResult == VK_SUCCESS);

    if(earlyOut)
        return;


    uint32_t frameIndex=0;
    const VkResult acquireNextImageResult = vkAcquireNextImageKHR(context._device, context._swapChain, std::numeric_limits<uint64_t>::max(), context._imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &frameIndex);
    if (acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        Vulkan::recreateSwapChain(appDesc, context);
        return;
    }

    assert(acquireNextImageResult == VK_SUCCESS);
    if(acquireNextImageResult != VK_SUCCESS)
        return; // skip frame and try again later

    // final submit
    VkSubmitInfo submitInfo;
    memset(&submitInfo, 0, sizeof(VkSubmitInfo));
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    constexpr VkPipelineStageFlags stageFlags[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &context._imageAvailableSemaphores[currentFrame];
    submitInfo.pWaitDstStageMask = stageFlags;

    VkCommandBuffer commandBuffers[]
    {
        context._commandBuffers[currentFrame],
    };

    submitInfo.commandBufferCount = sizeof(commandBuffers)/sizeof(VkCommandBuffer);
    submitInfo.pCommandBuffers = commandBuffers;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &context._renderFinishedSemaphores[currentFrame];

    VkResult submitResult = vkQueueSubmit(context._graphicsQueue, 1, &submitInfo, context._fences[currentFrame]);
    assert(submitResult == VK_SUCCESS);
    if(submitResult != VK_SUCCESS)
        return; // skip frame and try again later

    VkPresentInfoKHR presentInfo;
    memset(&presentInfo, 0, sizeof(VkPresentInfoKHR));
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &context._renderFinishedSemaphores[currentFrame];
    presentInfo.pSwapchains = &context._swapChain;
    presentInfo.swapchainCount = 1;
    presentInfo.pImageIndices = &frameIndex;
    presentInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(context._presentQueue, &presentInfo);
    if (presentResult != VK_SUCCESS)
    {
        Vulkan::recreateSwapChain(appDesc, context);
    }

    //  if (presentResult != VK_SUCCESS)
    //      return;

    context._currentFrame = (context._currentFrame+1) % (unsigned int)context._frameBuffers.size();

}


void modifyGraphicsPipelineInfo(Vulkan::VkGraphicsPipelineCreateInfoDescriptor& createInfo, unsigned int index)
{
    createInfo._rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;

    VkVertexInputBindingDescription bindingDescription;
    memset(&bindingDescription, 0, sizeof(VkVertexInputBindingDescription));

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(MeshVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    createInfo._vertexInputBindingDescriptions.push_back(bindingDescription);

    std::array<VkVertexInputAttributeDescription, 2> attributes = {};
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[0].offset = 0;
    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[1].offset = sizeof(glm::vec4);
    createInfo._vertexInputAttributeDescriptions.push_back(attributes[0]);
    createInfo._vertexInputAttributeDescriptions.push_back(attributes[1]);
}



int main(int argc, char *argv[])
{
    bool recreateSwapChain = false;

    std::string appName("Raytracing In One Weekend");

    SDL_Window * window = initSDL(appName);

    Vulkan::AppDescriptor appDesc;
    appDesc._appName = appName;
#if defined(__APPLE__)
    appDesc._requiredVulkanVersion = VK_API_VERSION_1_0;
#else
    appDesc._requiredVulkanVersion = VK_API_VERSION_1_1;
#endif
    appDesc._window = window;

    std::vector<Vulkan::Shader> shaders =
    {
        {"triangle_vert.spv", Vulkan::ShaderType::Vertex},
        {"triangle_frag.spv", Vulkan::ShaderType::Fragment},
    };
    readShaders(shaders);
    appDesc._shaders = shaders;

    appDesc._numGraphicsPipelines = 1;


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

    appDesc._graphicsPipelineCreationCallback = [](Vulkan::VkGraphicsPipelineCreateInfoDescriptor& createInfo, unsigned int index) {
        modifyGraphicsPipelineInfo(createInfo, index); };

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

    RayTracer tracer(appDesc, context);


    static char pixelBuffer[windowWidth][windowHeight][4];
    memset(pixelBuffer, 0xFF, sizeof(pixelBuffer));

	CircularArray<60, double> fpsCounter;
    bool gameIsRunning = true;
    while (gameIsRunning)
    {
		static unsigned int frameCount = 0;
		Uint64 currentTicks = SDL_GetPerformanceCounter();
        static Uint64 lastTicks = currentTicks;

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

        gameIsRunning = Vulkan::update(appDesc, context, context._currentFrame);
        if (gameIsRunning)
        {
            tracer.update();
            render(appDesc, context, recreateSwapChain);
            SDL_UpdateWindowSurface(appDesc._window);
            recreateSwapChain = false;
        }

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
                        recreateSwapChain = true;
						break;
					}
				}

            }
        }

		frameCount++;
    }

    return 0;
}
