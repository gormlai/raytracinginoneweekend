#ifndef _RAYTRACER_H_
#define _RAYTRACER_H_

#include "vulkan-setup/VulkanSetup.h"
#include "Mesh.h"



class RayTracer
{
public:
    RayTracer(Vulkan::AppDescriptor & appInfo, Vulkan::Context & context, Vulkan::EffectDescriptor & effect);

    void update();

    ~RayTracer();

    std::vector<Vulkan::MeshPtr> _vulkanMeshes;

private:
    Vulkan::AppDescriptor & m_appInfo;
    Vulkan::Context & m_context;
    Vulkan::EffectDescriptor & m_effect;

    void createBackground();

    MeshPtr m_background;
};


#endif
