#ifndef _DEFINES_H_
#define _DEFINES_H_

#include "singlefile/AABB.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

typedef AABB<glm::vec3, float, 3, INT_MIN, INT_MAX> AABB3;
constexpr float PHYSICS_STEP = 1.0f / 60.0f;


#endif // ! _DEFINES_H_
