#include <gfx\pass\BasePass.hpp>

#include <foundation\Common.hpp>
#include <glm\vec3.hpp>

namespace GFX
{

BasePass::BasePass()
{
}

BasePass::BasePass(BasePass&& rhs)
{
    operator=(DRE_MOVE(rhs));
}

BasePass& BasePass::operator=(BasePass&& rhs)
{
    return *this;
}

BasePass::~BasePass()
{
}

glm::uvec3 BasePass::GetComputeGroupCount(glm::uvec3 const& totalSize, glm::uvec3 const& groupSize)
{
    return (totalSize + groupSize - 1u) / groupSize;
}

}
