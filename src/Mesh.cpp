#include "Mesh.h"

#include <glm/gtx/matrix_decompose.hpp>

namespace
{
    constexpr unsigned int NumIndicesPrVertex = 3;
}

Mesh::Mesh()
	:_parent(nullptr)
{
    
}

Mesh::Mesh(const Mesh & src)
	: _parent(nullptr)
	,_localTransform(src._localTransform)
{
	setMeshData(src._vertices, src._indices);
}

Mesh::Mesh(const std::vector<MeshVertex> & vertices, const std::vector<uint16_t> & indices)
	:_parent(nullptr)
{
	setMeshData(vertices, indices);
}

void Mesh::generateNormals()
{
    const unsigned int numTriangles = (unsigned int)_indices.size() / NumIndicesPrVertex;
    
    _faceNormals.resize(numTriangles);
    for(unsigned int i=0 ; i < numTriangles ; i++ )
    {
        const unsigned int index0 = _indices[i*NumIndicesPrVertex+0];
        const unsigned int index1 = _indices[i*NumIndicesPrVertex+1];
        const unsigned int index2 = _indices[i*NumIndicesPrVertex+2];
        
        MeshVertex & v0 = _vertices[index0];
        MeshVertex & v1 = _vertices[index1];
        MeshVertex & v2 = _vertices[index2];
        
        const glm::vec3 v1v0 = v0._position - v1._position;
        const glm::vec3 v1v2 = v2._position - v1._position;
        const glm::vec3 cross = glm::cross(v1v0, v1v2);
        const glm::vec4 normal = glm::vec4{glm::normalize(cross), 0.0f};
        _faceNormals[i] = normal;
    }
}

unsigned int Mesh::numTriangles() const
{
    const unsigned int triangleCount = (unsigned int)_indices.size() / NumIndicesPrVertex;
    return triangleCount;
}

void Mesh::setMeshData(const std::vector<MeshVertex> & vertices, const std::vector<uint16_t> & indices)
{
	_vertices = vertices;
	_indices = indices;
	generateNormals();
	
	// calculate AABB
	_aabb = AABB3();
	for (const MeshVertex & vertex : vertices)
		_aabb.extend(vertex._position);
}

glm::mat4 Mesh::getWorldMatrix() const
{
	std::function<glm::mat4(const Mesh*) > matrixTraverser;

	matrixTraverser = [&matrixTraverser](const Mesh * mesh)
	{
		if (mesh == nullptr)
		{
			glm::mat4 result = glm::mat4(1.0f);
			return result;
		}

		const Transform & transform = mesh->getLocalTransform();
		const glm::mat4 localMat = transform.toMatrix();
		const glm::mat4 parentMat = matrixTraverser(mesh->_parent.get());
		const glm::mat4 result = parentMat * localMat;
		return result;
	};

	return matrixTraverser(this);
}

Transform Mesh::getWorldTransform() const
{ 
	Transform transform;
	glm::mat4 worldMatrix = getWorldMatrix();

	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(worldMatrix, transform._scale, transform._rotation, transform._position, skew, perspective);

	return transform;
}

namespace
{
	inline Transform decompose(const glm::mat4 & srcMatrix)
	{
		Transform result;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(srcMatrix, result._scale, result._rotation, result._position, skew, perspective);
		return result;
	}
}

void Mesh::setWorldTransform(const Transform & transform)
{

	if(_parent == nullptr)
		_localTransform = transform;
	else
	{
		const glm::mat4 worldMatrix = transform.toMatrix();
		const glm::mat4 parentWorldMatrix = _parent->getWorldMatrix();
		const glm::mat4 inverseParentWorldMatrix = glm::inverse(parentWorldMatrix);
		const glm::mat4 localMatrix =  inverseParentWorldMatrix * worldMatrix;

		_localTransform = decompose(localMatrix);
		Transform worldTransform = decompose(worldMatrix);
		Transform parentWorldTransform = decompose(parentWorldMatrix);

		int k=0;
		k=1;

	}
}
