#pragma once

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

#include <editor\BaseEditor.hpp>

namespace WORLD
{
class Entity;
class Scene;
class SceneNode;
}

namespace EDITOR
{

class SceneGraphEditor : public BaseEditor
{
public:
    SceneGraphEditor(BaseEditor* rootEditor, EditorFlags flags, WORLD::Scene* scene);
    SceneGraphEditor(SceneGraphEditor&& rhs);

    SceneGraphEditor& operator=(SceneGraphEditor&& rhs);

    virtual ~SceneGraphEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::SceneGraph; }

    virtual void Render() override;

private:

    struct RenderingContext
    {
        DRE::U32 m_CurrentID = 0u;
    };

    static DRE::String64 GetUniqueLabel(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context);

    void RenderSceneNodeRecursive(WORLD::SceneNode* node, RenderingContext& context);

    bool RenderNodeProperties();
    bool RenderLightProperties(bool wasTransformUpdated);

private:
    WORLD::Scene* m_Scene;
};

}