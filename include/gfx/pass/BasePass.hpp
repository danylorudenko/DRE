#pragma once

#include <vector>

#include <vulkan\vulkan.h>

#include <vk_wrapper\Context.hpp>

#include <gfx\pass\PassID.hpp>
#include <gfx\scheduling\GraphResource.hpp>


namespace GFX
{

class RenderGraph;

class BasePass
{
public:
    /////////////////////////
    BasePass();

    BasePass(BasePass&& rhs);
    BasePass& operator=(BasePass&& rhs);

    virtual ~BasePass();


    /////////////////////////
    virtual PassID GetID() const = 0;

    /////////////////////////
    virtual void RegisterResources  (RenderGraph& graph) = 0;
    virtual void Initialize         (RenderGraph& graph) = 0;
    virtual void Render             (RenderGraph& graph, VKW::Context& context) = 0;
};

}

