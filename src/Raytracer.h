#ifndef _RAYTRACER_H_
#define _RAYTRACER_H_

#include "vulkan-setup/VulkanSetup.h"
#include "Mesh.h"

class RayTracer
{
public:
    RayTracer(Vulkan::AppDescriptor & appInfo, Vulkan::VulkanContext & context);

    void update();

    ~RayTracer();

private:
    Vulkan::AppDescriptor & m_appInfo;
    Vulkan::VulkanContext & m_context;

    void createBackground();

    MeshPtr m_background;
};


#endif
