#pragma once

#include <glm\mat4x4.hpp>
#include <glm\vec3.hpp>

namespace VKW
{
class Pipeline;
}


namespace GFX
{

class RenderableObject
{
public:
    RenderableObject(VKW::Pipeline* material);

    inline glm::mat4x4 const&   GetModelM() const { return m_ModelM; }
    inline VKW::Pipeline*       GetMaterial() const{ return m_Material; }

    void                        Transform(glm::vec3 pos, glm::vec3 eulerRotation, glm::vec3 scale);

private:
    glm::mat4x4     m_ModelM;
    VKW::Pipeline*  m_Material;
    // material?
};

}
