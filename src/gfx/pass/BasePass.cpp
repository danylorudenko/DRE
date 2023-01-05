#include <gfx\pass\BasePass.hpp>

#include <foundation\Common.hpp>

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

}
