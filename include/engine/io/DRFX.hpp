#pragma once

/*
#include <foundation\String\InplaceString.hpp>
#include <foundation\Container\InplaceVector.hpp>

#include <vk_wrapper\Format.hpp>

namespace DRFX
{

enum Keyword : std::uint16_t
{
    Less = 0,
    Equals,
    LessEquals,
    Greater,
    GreaterEquals,
    Always,
    Never,
    Zero,
    One,
    Keep,
    Replace,
    Increment,
    Decrement,
    Vertex,
    Fragment,
    Compute,
    Graphics,
    Raytracing,
    Front,
    Back,
    Solid,
    Outline,
    TriangleList,
    None,
    Alpha,
    Texture,
    Buffer,
    Uniform,
    MAX
};

void InitializeDictionary();

class Entity
{
public:
    Entity(char const* name) : m_Name{ name } {}
    inline DRE::String32 const& GetName() const { return m_Name; }
private:
    DRE::String32 m_Name;
};

class VertexState : public Entity
{
public:
    VertexState();

    void AddBinding(Keyword type, std::uint16_t stride);
    void AddAttribute(std::uint16_t binding, VKW::Format format, std::uint16_t offsetInBinding);

    void EnableIndex(std::uint32_t indexSize);

private:
    struct Binding
    {
        std::uint16_t stride;
        Keyword       type;
    };

    struct Attribute
    {
        std::uint16_t binding;
        std::uint16_t offsetInBinding;
        VKW::Format   format;
    };


    DRE::InplaceVector<std::uint16_t, 3>    m_BindingStrides;
    DRE::InplaceVector<Attribute, 6>        m_Attributes;
};

class DepthStencilState : public Entity
{
public:
    DepthStencilState(char const* name);

    void EnableDepth(bool writeOn, Keyword compOp);
    void EnableStencil(bool writeOn, Keyword compOp, Keyword failOp, Keyword passOp);
};

class ShaderModule : public Entity
{
public:
    ShaderModule(char const* name, Keyword type);
};

class Pipeline : public Entity
{

public:
    Pipeline(char const* name, Keyword type);

    void AddTextureProperty(char const* name);
    void AddBufferProperty(char const* name);
    void AddUniformProperty(std::uint16_t size);

    void AddVertexState(VertexState const& state, ShaderModule const& shader);
    void AddFragmentState(Keyword blending, std::uint32_t rtCount, VKW::Format* rtFormats, ShaderModule const& shader);
    void AddDepthStencilState(DepthStencilState const& state);
    void SetCullMode(Keyword mode);
    void SetFillMode(Keyword mode);
    void SetTopology(Keyword topology);
};


class Document : public Entity
{
public:
    Document(char const* name, char const* filePath);

private:
    DRE::String128      m_FilePath;

    DRE::InplaceVector<Pipeline, 32> m_Pipelines;
};


}
*/
