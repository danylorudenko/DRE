#pragma once

#include <gfx\pass\BasePass.hpp>

namespace GFX
{

class FFTButterflyGenPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    // Inherited via BasePass
    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};


class FFTWaterH0GenPass : public BasePass
{
public:
    virtual PassID  GetID() const override;

    // Inherited via BasePass
    virtual void    RegisterResources(RenderGraph& graph) override;
    virtual void    Initialize(RenderGraph& graph) override;
    virtual void    Render(RenderGraph& graph, VKW::Context& context) override;
};


class FFTWaterHxtGenPass : public BasePass
{
public:
    virtual PassID  GetID() const override;

    // Inherited via BasePass
    virtual void    RegisterResources(RenderGraph& graph) override;
    virtual void    Initialize(RenderGraph& graph) override;
    virtual void    Render(RenderGraph& graph, VKW::Context& context) override;
};

class FFTWaterHeightGenPass : public BasePass
{
public:
    virtual PassID  GetID() const override;

    // Inherited via BasePass
    virtual void    RegisterResources(RenderGraph& graph) override;
    virtual void    Initialize(RenderGraph& graph) override;
    virtual void    Render(RenderGraph& graph, VKW::Context& context) override;

private:
    DRE::InplaceVector<VKW::DescriptorSet, 24> m_StageSets0;
    DRE::InplaceVector<VKW::DescriptorSet, 24> m_StageSets1;
};

class FFTInvPermutationPass : public BasePass
{
public:
    virtual PassID  GetID() const override;

    // Inherited via BasePass
    virtual void    RegisterResources(RenderGraph& graph) override;
    virtual void    Initialize(RenderGraph& graph) override;
    virtual void    Render(RenderGraph& graph, VKW::Context& context) override;
};

}

