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

bool recordStandardCommandBuffers(Vulkan::AppDescriptor& appDesc, Vulkan::Context & context, Vulkan::EffectDescriptor & effectDescriptor)
{

    std::vector<VkCommandBuffer>& commandBuffers = effectDescriptor._commandBuffers;

    if (!Vulkan::resetCommandBuffers(context, commandBuffers))
        return false;

    const uint32_t currentFrame = context._currentFrame;

    VkCommandBufferBeginInfo beginInfo;
    memset(&beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    const VkResult beginCommandBufferResult = vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);
    assert(beginCommandBufferResult == VK_SUCCESS);
    if (beginCommandBufferResult != VK_SUCCESS)
    {
        SDL_LogError(0, "call to vkBeginCommandBuffer failed, i=%d\n", currentFrame);
        return false;
    }

    VkRenderPassBeginInfo renderPassBeginInfo;
    memset(&renderPassBeginInfo, 0, sizeof(VkRenderPassBeginInfo));
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = context._renderPass;
    renderPassBeginInfo.framebuffer = context._frameBuffers[currentFrame];
    renderPassBeginInfo.renderArea.offset = { 0,0 };
    renderPassBeginInfo.renderArea.extent = context._swapChainSize;

    const glm::vec4 bgColor = appDesc._backgroundClearColor();
    VkClearValue clearColorValue[2];
    clearColorValue[0].color = VkClearColorValue{ bgColor[0], bgColor[1], bgColor[2], bgColor[3] };
    clearColorValue[1].depthStencil = VkClearDepthStencilValue{ 1.0f, 0 };

    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColorValue[0];

    vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, effectDescriptor._pipeline);

    std::vector<Vulkan::MeshPtr>& vulkanMeshes = effectDescriptor._meshes;
    for (unsigned int meshCount = 0; meshCount < vulkanMeshes.size(); meshCount++)
    {
        VkBuffer vertexBuffer[] = { vulkanMeshes[meshCount]->_vertexBuffer._buffer };
        VkDeviceSize offsets[] = { 0 };

        vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, &vertexBuffer[0], offsets);
        vkCmdBindIndexBuffer(commandBuffers[currentFrame], vulkanMeshes[meshCount]->_indexBuffer._buffer, 0, VK_INDEX_TYPE_UINT16);
        
        vkCmdBindDescriptorSets(commandBuffers[currentFrame],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            effectDescriptor._pipelineLayout,
            0,
//            (uint32_t)vulkanMeshes[meshCount]->_descriptorSets.size(),
            1,
            &vulkanMeshes[meshCount]->_descriptorSets[0],
            0,
            nullptr);

            
        vkCmdDrawIndexed(commandBuffers[currentFrame], vulkanMeshes[meshCount]->_numIndices, 1, 0, 0, 0);

    }
    vkCmdEndRenderPass(commandBuffers[currentFrame]);

    const VkResult endCommandBufferResult = vkEndCommandBuffer(commandBuffers[currentFrame]);
    assert(endCommandBufferResult == VK_SUCCESS);
    if (endCommandBufferResult != VK_SUCCESS)
    {
        SDL_LogError(0, "Call to vkEndCommandBuffer failed (i=%d)\n", currentFrame);
        return false;
    }

    return true;
}


void render(Vulkan::AppDescriptor & appDesc, Vulkan::Context & context, bool recreateSwapChain)
{
    const unsigned currentFrame = context._currentFrame;
    bool earlyOut = false;
    if (recreateSwapChain)
    {
        Vulkan::recreateSwapChain(appDesc, context);
//        earlyOut = true;
    }

//    const VkResult waitForFencesResult = vkWaitForFences(context._device, 1, &context._fences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    const VkResult waitForFencesResult = vkWaitForFences(context._device, 1, &context._fences[currentFrame], VK_TRUE, 1);
//    assert(waitForFencesResult == VK_SUCCESS);

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
    constexpr VkPipelineStageFlags stageFlags[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &context._imageAvailableSemaphores[currentFrame];
    submitInfo.pWaitDstStageMask = stageFlags;

    static std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.clear();

    for(size_t i= 0 ; i < context._effects.size() ; i++)
    {
        Vulkan::EffectDescriptorPtr & effect = context._effects[i];
        std::vector<VkCommandBuffer> & effectCommandBuffers = effect->_commandBuffers;
        VkCommandBuffer& effectCommandBuffer = effectCommandBuffers[currentFrame];
        commandBuffers.push_back(effectCommandBuffer);
    }

    submitInfo.commandBufferCount = commandBuffers.size();

    submitInfo.pCommandBuffers = commandBuffers.empty() ? VK_NULL_HANDLE : &commandBuffers[0];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &context._renderFinishedSemaphores[currentFrame];
    
    const VkResult resetFencesResult = vkResetFences(context._device, 1, &context._fences[currentFrame]);
    assert(resetFencesResult == VK_SUCCESS);

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


void modifyGraphicsPipelineInfo(Vulkan::VkGraphicsPipelineCreateInfoDescriptor& createInfo)
{
    createInfo._rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;

    VkVertexInputBindingDescription bindingDescription;
    memset(&bindingDescription, 0, sizeof(VkVertexInputBindingDescription));

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(MeshVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    createInfo._vertexInputBindingDescriptions.push_back(bindingDescription);

    std::array<VkVertexInputAttributeDescription, 3> attributes = {};
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[0].offset = 0;
    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[1].offset = sizeof(glm::vec4);
    attributes[2].binding = 0;
    attributes[2].location = 2;
    attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[2].offset = sizeof(glm::vec4) + sizeof(glm::vec2);
    createInfo._vertexInputAttributeDescriptions.push_back(attributes[0]);
    createInfo._vertexInputAttributeDescriptions.push_back(attributes[1]);
    createInfo._vertexInputAttributeDescriptions.push_back(attributes[2]);
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
        {"triangle_vert.spv", VK_SHADER_STAGE_VERTEX_BIT },
        {"triangle_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT },
    };
    readShaders(shaders);


    //    Mesh mesh = MeshFactory::createPlane(1.0f, 1.0f);

    appDesc._backgroundClearColor = []() { return glm::vec4{ 0.2f, 0.2f, 0.2f, 1.0 }; };

    Vulkan::Context context;
    if(!Vulkan::handleVulkanSetup(appDesc, context))
    {
        SDL_LogError(0, "Failed to handle vulkan setup\n");
        return 1;
    }


    Vulkan::EffectDescriptorPtr squareEffect( new Vulkan::EffectDescriptor() );
    squareEffect->_uniformBufferSizes.push_back((uint32_t)sizeof(UniformBufferObject));
    squareEffect->_shaderModules = shaders;
    if(!Vulkan::initEffectDescriptor(appDesc, context, modifyGraphicsPipelineInfo, *squareEffect))
    {
        SDL_LogError(0, "Failed to init effect descriptor\n");
        return 1;
    }

    context._effects.push_back(squareEffect);
    // raytracer object
    RayTracer tracer(appDesc, context, *squareEffect);
    squareEffect->_meshes = tracer._vulkanMeshes;
    squareEffect->_recordCommandBuffers = recordStandardCommandBuffers;


    // create the textured image
    constexpr int imageWidth = 200;
    constexpr int imageHeight = 150;
    static glm::vec4 pixelBuffer[imageWidth][imageHeight];
    for(int y = imageHeight-1 ; y>=0 ; y--)
    {
        for(int x = 0 ; x < imageWidth ; x++)
        {
            float red = (float)x / imageWidth;
            float green = (float)y / imageHeight;
            float blue = 0.2f;
            float alpha = 1.0f;
            pixelBuffer[x][y] = glm::vec4{red, green, blue, alpha};
        }
    }

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

        tracer.update();
        Vulkan::updateUniforms(appDesc, context, context._currentFrame);
        render(appDesc, context, recreateSwapChain);
        SDL_UpdateWindowSurface(appDesc._window);
        recreateSwapChain = false;

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
