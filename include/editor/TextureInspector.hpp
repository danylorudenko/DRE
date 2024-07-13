#pragma once

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>
#include <foundation\container\InplaceVector.hpp>

#include <editor\BaseEditor.hpp>

namespace GFX
{
class TextureBank;
class GraphResourcesManager;
class Texture;
}

namespace EDITOR
{

class TextureInspector : public BaseEditor
{
public:
    TextureInspector(BaseEditor* rootEditor, EditorFlags flags, GFX::TextureBank* textureBank, GFX::GraphResourcesManager* graphResources);
    TextureInspector(TextureInspector&& rhs);

    TextureInspector& operator=(TextureInspector&& rhs);

    virtual ~TextureInspector() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::TextureInspector; }

    virtual void Render() override;

private:
    GFX::TextureBank*           m_TextureBank;
    GFX::GraphResourcesManager* m_GraphResources;

    DRE::InplaceVector<GFX::Texture*, 12> m_DisplayedTextures;
};

}