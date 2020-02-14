#include "Raytracer.h"
#include "MeshFactory.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


RayTracer::RayTracer(Vulkan::AppDescriptor & appInfo, Vulkan::Context & context, Vulkan::EffectDescriptor& effect)
:m_appInfo(appInfo)
,m_context(context)
,m_effect(effect)
{
    createBackground();
}

RayTracer::~RayTracer()
{
    
}

void RayTracer::update()
{
    
}

void RayTracer::createBackground()
{
    MeshFactory & factory = MeshFactory::instance();
    MeshPtr mesh = factory.createPlane(1, 1);
    if(mesh == nullptr)
        return;

    std::vector<unsigned char> indexData, vertexData;
    const std::vector<MeshVertex> & vertices = mesh->getVertices();
    const std::vector<uint16_t> & indices = mesh->getIndices();

    indexData.resize(indices.size() * sizeof(uint16_t));
    memcpy(&indexData[0], &indices[0], indexData.size());

    vertexData.resize(vertices.size() * sizeof(MeshVertex));
    memcpy(&vertexData[0], &vertices[0], vertexData.size());

    Transform transform;
    transform._position = glm::vec3{0, 0, 0};
    mesh->setLocalTransform(transform);

    mesh->_name = std::string("Floor");

    void *userData = (void*)mesh.get();
    Vulkan::MeshPtr vulkanMesh(new Vulkan::Mesh());
    if (Vulkan::addMesh(m_appInfo, m_context, vertexData, indexData, userData, m_effect, *vulkanMesh))
    {
        std::vector<Vulkan::MeshPtr> meshes{ vulkanMesh };
        for (Vulkan::MeshPtr mesh : meshes)
        {
            mesh->_updateUniform = [this](unsigned int uniformIndex, std::vector<unsigned char>& receiver)
            {
                constexpr size_t dataSizeNeeded = sizeof(UniformBufferObject);
                switch (uniformIndex)
                {
                case 0:
                {
                    glm::vec3 camPos{ 0,0,8 };
                    glm::vec3 camDir{ 0,0,-1 };
                    glm::vec3 camUp{ 0,1,0 };
                    UniformBufferObject ubo;
                    ubo._view = glm::lookAt(camPos, camPos + camDir, camUp);
                    ubo._projection = glm::perspective(glm::radians(45.0f), m_context._swapChainSize.width / (float)m_context._swapChainSize.height, 0.1f, 1000.0f);
                    ubo._model = glm::identity<glm::mat4>();

                    if (receiver.size() < dataSizeNeeded)
                        receiver.resize(dataSizeNeeded);
                    memcpy(&receiver[0], reinterpret_cast<const void*>(&ubo), dataSizeNeeded);
                    break;
                }
                }
                return (unsigned int)dataSizeNeeded;
            };
        }
        _vulkanMeshes = meshes;
    }


    m_background = mesh; // store pointer
}
