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
        //static constexpr DRE::U32 C_PADDING = 10u;
        //DRE::U32 m_CurrentPadding = 0u;

        DRE::U32 m_CurrentID = 0u;
    };

    static DRE::String64 GetUniqueLabel(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context);

    void RenderSceneNodeRecursive(WORLD::SceneNode* node, RenderingContext& context);
    void AttemptOpenProperties(WORLD::SceneNode* node);

private:
    WORLD::Scene* m_Scene;
};

}