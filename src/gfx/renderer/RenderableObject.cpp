#include <gfx\renderer\RenderableObject.hpp>

#include <glm\gtc\quaternion.hpp>

namespace GFX
{

RenderableObject::RenderableObject(VKW::Pipeline* pipeline, VKW::BufferResource* vertexBuffer, VKW::BufferResource* indexBuffer, TexturesVector&& textures, DescriptorSetVector&& sets)
    : m_ModelM{ glm::identity<glm::mat4>() }
    , m_Pipeline{ pipeline }
    , m_VertexBuffer{ nullptr }
    , m_IndexBuffer{ nullptr }
    , m_DescriptorSets{ DRE_MOVE(sets) }
    , m_Textures{ DRE_MOVE(textures) }
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