#ifndef _MESH_FACTORY_H_
#define _MESH_FACTORY_H_

#include "Mesh.h"
#include "Math.h"

class MeshFactory
{
public:
    MeshPtr createPlane(const float width, const float height);
    MeshPtr createBox(const float width, const float height, const float depth);
    MeshPtr createBox(const glm::vec3 dimensions);
    
    static MeshFactory & instance();
        
private:
    MeshFactory();
    
};

#endif


