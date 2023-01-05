#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

namespace GFX
{

RenderableObject::RenderableObject(VKW::Pipeline* pipeline)
    : m_Material{ pipeline }
    , m_ModelM{ glm::identity<glm::mat4>() }
{
}

void RenderableObject::Transform(glm::vec3 pos, glm::vec3 eulerRotation, glm::vec3 scale)
{
    m_ModelM = glm::mat4{ 
        glm::vec4{ scale.x, 0.0f, 0.0f, 0.0f },
        glm::vec4{ 0.0f, scale.y, 0.0f, 0.0f },
        glm::vec4{ 0.0f, 0.0f, scale.z, 0.0f },
        glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f }
    };

    glm::quat orientation{ glm::radians(eulerRotation) };
    m_ModelM *= glm::mat4_cast(orientation);

    m_ModelM[0][3] = pos.x;
    m_ModelM[0][3] = pos.y;
    m_ModelM[0][3] = pos.z;
}

}