#ifndef HACKLIB_MATH_H
#define HACKLIB_MATH_H

#ifdef _WIN32
#include "d3dx9.h"
#endif

#define GLM_FORCE_NO_CTOR_INIT
#include "glm/glm.hpp"


namespace hl
{
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Mat4x4 = glm::mat4x4;
}

#endif
