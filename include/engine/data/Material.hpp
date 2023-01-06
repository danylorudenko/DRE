#pragma once

#include <foundation\String\InplaceString.hpp>
#include <foundation\memory\ByteBuffer.hpp>

#include <engine\data\Texture2D.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>

namespace Data
{

class Material
{
public:
    //////////////////////////////////////
    // TextureProperty
    class TextureProperty
    {
    public:
        friend class Material;

        enum Slot
        {
            DIFFUSE,
            NORMAL,
            HEIGHT,
            METALNESS,
            MAX
        };

        Texture2D const& GetTexture() const;
        void SetTexture(Texture2D&& texture);

    private:
        Texture2D m_Texture;
    };
    //////////////////////////////////////


    //////////////////////////////////////
    // DataProperty
    class DataProperty
    {
    public:
        friend class Material;

        void AssignData(void const* data, DRE::SizeT size);
        template<typename T>
        void AssignData(T const& data) { AssignData(&data, sizeof(data)); }

        void* GetData() const;

    private:
        DRE::ByteBuffer m_DataBuffer;
    };
    //////////////////////////////////////


    //////////////////////////////////////
    // PipelineProperties
    class PipelineProperties
    {
    public:
        void SetShader(char const* shaderName);
        
        VKW::Pipeline* GetPSO() const;

    private:
        DRE::String64 m_ShaderName;
        bool m_Transparent;// examples. this to be translated into another Gfx object with pso
        bool m_Emissive;   // examples. this to be translated into another Gfx object with pso

        VKW::Pipeline* m_CachedPSO = nullptr;
    };


    Material(char const* name);

    //////////////////////////////////////
    // Material
    void AssignTextureToSlot(TextureProperty::Slot slot, Texture2D&& texture);
    void AssignData(void const* data, DRE::SizeT size);
    template<typename T>
    void AssignData(T const& data) { m_DataProperty.AssignData(data); }

    PipelineProperties& GetPipelineProperties() { return m_PipelineProperties; }
    PipelineProperties const& GetPipelineProperties() const { return m_PipelineProperties; }

private:
    DRE::String32       m_Name;

    TextureProperty     m_TextureProperties[TextureProperty::Slot::MAX];
    DataProperty        m_DataProperty;
    PipelineProperties  m_PipelineProperties;
};

}
