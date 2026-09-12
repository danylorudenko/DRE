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

class ViewportInputManager;

class SceneGraphEditor : public BaseEditor
{
public:
    SceneGraphEditor(BaseEditor* rootEditor, ViewportInputManager* inputManager, EditorFlags flags, WORLD::Scene* scene);
    SceneGraphEditor(SceneGraphEditor&& rhs);

    SceneGraphEditor& operator=(SceneGraphEditor&& rhs);

    virtual ~SceneGraphEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::SceneGraph; }

    virtual void Render() override;

private:

    class RenderingContext
    {
    public:
        RenderingContext();

        // warning that the string becomes invalid after the next call to GetNextUniqueLabel()
        char const* GetNextUniqueLabel(char const* displayLabel);
    private:
        DRE::U32 m_CurrentID = 0u;
        char m_UniqueLabel[128];
    };

    DRE::String128 GetUniqueSceneNodeLabel(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context);

    void RenderSceneNodeRecursive(WORLD::SceneNode* node, RenderingContext& context);

    bool RenderNodeProperties();
    void RenderEntityProperties(SceneGraphEditor::RenderingContext& context);
    bool RenderLightProperties(bool wasTransformUpdated);

private:
    WORLD::Scene* m_Scene;
    bool m_ShowIDs;

    ViewportInputManager* m_ViewportInputManager;
};

}