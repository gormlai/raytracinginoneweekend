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
    std::vector<MeshVertex> vertices = mesh->getVertices();
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
        _vulkanMeshes = meshes;
    }


    m_background = mesh; // store pointer
}
