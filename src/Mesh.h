#ifndef _MESH_H_
#define _MESH_H_

#include <memory>
#include <string>
#include <tuple>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vulkan-setup/VulkanSetup.h"
#include "Defines.h"


struct Transform
{
    glm::vec3 _position;
    glm::quat _rotation;
    glm::vec3 _scale;
    
    Transform()
    :_scale{1,1,1}
	,_position{0,0,0}
	,_rotation{1,0,0,0}
	{}
    
    glm::mat4 toMatrix() const
    {
		glm::mat4 mat = glm::translate(_position) * glm::mat4_cast(_rotation) * glm::scale(_scale);
//		glm::mat4 mat = glm::mat4_cast(_rotation) * glm::translate(_position) ;// * glm::scale(_scale);
//		glm::mat4 mat = glm::translate(_position) * glm::mat4_cast(_rotation) * glm::scale(_scale);
		return mat;
    }
    

};

struct MeshVertex
{
    MeshVertex() {}

    MeshVertex(const glm::vec4 & position, const glm::vec4 & color)
        :_position(position)
        ,_color(color) {}
    

    glm::vec4 _position;
    glm::vec4 _color;
};

struct Mesh;
typedef std::shared_ptr<Mesh> MeshPtr;

struct Mesh
{
public:
    Mesh();
    Mesh(const Mesh & src);
    Mesh(const std::vector<MeshVertex> & vertices, const std::vector<uint16_t> & indices);
    unsigned int numTriangles() const;
    
	std::vector<MeshPtr> _children;
	std::string _name;
	MeshPtr _parent;

	void setMeshData(const std::vector<MeshVertex> & vertices, const std::vector<uint16_t> & indices);

	inline const std::vector<MeshVertex> & getVertices() const { return _vertices; }
	inline const std::vector<uint16_t> & getIndices() const { return _indices; }
	inline const AABB3 getAABB() const { return _aabb; }
	inline Transform getLocalTransform() { return _localTransform; }
	inline const Transform & getLocalTransform() const { return _localTransform; }
	inline glm::mat4 getLocalMatrix() const { return _localTransform.toMatrix(); }
	inline void setLocalTransform(const Transform & transform) { _localTransform = transform; }

	 glm::mat4 getWorldMatrix() const;
	Transform getWorldTransform() const;
    void setWorldTransform(const Transform & transform);

private:
	std::vector<MeshVertex> _vertices;
    std::vector<uint16_t> _indices;
    
    
private:
    std::vector<glm::vec4> _faceNormals;
    std::vector<glm::vec4> _vertexNormals;
	AABB3 _aabb;
	Transform _localTransform;

    void generateNormals();
    
};



#endif
