#include "MeshFactory.h"

#include "singlefile/Palette.h"
#include "singlefile/Random.h"

/////////////////////////////////////////////////////////
//// color creation ////////////////////////////////////

namespace
{
    glm::vec4 getRandomColor()
    {
        constexpr float baseColors[4][3] =
        {
            {0.8f, 0.5f, 0.4f},
            {0.2f, 0.4f, 0.2f},
            {2.0f, 1.0f, 1.0f},
            {0.00f, 0.25f, 0.25f},
        };
        
        static UniformDistribution<float> dist(0.0f, 1.0f);
        float t = dist.generate();
        float v[3];
        Palette::inigoQuilez<float,3>(t, baseColors, v);
        
//        glm::vec4 result(v[0], v[1], v[2], 1.0f);
        glm::vec4 result(1.0f, 1.0f, 1.0f, 1.0f);
        return result;
    }
}



MeshFactory::MeshFactory()
{
    
}

MeshFactory & MeshFactory::instance()
{
    static MeshFactory factory;
    return factory;
}


MeshPtr MeshFactory::createPlane(const float width, const float height)
{
    MeshPtr pMesh(new Mesh());
    Mesh & mesh = *pMesh;
    
    const glm::vec4 vertexColor = getRandomColor();
    
    const std::vector<MeshVertex> vertices =
    {
        MeshVertex( {-width/2.0f, - height/2.0f, 0.0, 1.0f},vertexColor),
        MeshVertex( {width/2.0f, - height/2.0f, 0.0f, 1.0f},vertexColor),
        MeshVertex( {width/2.0f, height/2.0f, 0.0f, 1.0f},vertexColor),
        MeshVertex( {-width/2.0f, height/2.0f, 0.0f, 1.0f},vertexColor),
    };
    
    const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };
	mesh.setMeshData(vertices, indices);
    
    return pMesh;
}

MeshPtr MeshFactory::createBox(const float width, const float height, const float depth)
{
    MeshPtr pMesh(new Mesh());
    Mesh & mesh = *pMesh;
    const glm::vec4 vertexColor = getRandomColor();
    
    glm::vec3 vertexData[] = {
        {-width/2.0f, -height/2.0f, -depth/2.0f},
        {-width/2.0f, height/2.0f,  -depth/2.0f},
        {width/2.0f,  height/2.0f,  -depth/2.0f},
        {width/2.0f,  -height/2.0f, -depth/2.0f},
        {-width/2.0f, -height/2.0f, depth/2.0f},
        {-width/2.0f, height/2.0f,  depth/2.0f},
        {width/2.0f,  height/2.0f,  depth/2.0f},
        {width/2.0f,  -height/2.0f, depth/2.0f},
    };
    const unsigned int numVertices = sizeof(vertexData) / sizeof(glm::vec3);
	std::vector<MeshVertex> vertices(numVertices);
    for(unsigned int i=0 ; i < numVertices ; i++)
    {
       const glm::vec4 position{vertexData[i],1.0f};
       MeshVertex vertex{position, vertexColor};
       vertices[i] = vertex;
    }
    
    std::vector<uint16_t> indices =
    {
        0,1,2,
        0,2,3,
        4,6,5,
        4,7,6,
        4,5,1,
        4,1,0,
        3,2,6,
        3,6,7,
        1,5,6,
        1,6,2,
        4,0,3,
        4,3,7,
    };

	pMesh->setMeshData(vertices, indices);
    
    return pMesh;
}

MeshPtr MeshFactory::createBox(const glm::vec3 dimensions)
{
    return createBox(dimensions.x, dimensions.y, dimensions.z);
}
