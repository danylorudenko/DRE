#pragma once

#include <foundation\Common.hpp>

#include <editor\BaseEditor.hpp>

namespace WORLD
{
class ISceneNodeUser;
}

namespace EDITOR
{

class TransformEditor : public BaseEditor
{
public:
    TransformEditor(BaseEditor* rootEditor, EditorFlags flags);
    TransformEditor(TransformEditor&& rhs);

    TransformEditor& operator=(TransformEditor&& rhs);

    virtual ~TransformEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::Transform; }

    virtual void Render() override;

    void SetNode(WORLD::ISceneNodeUser* user);


private:
    void RenderTransformProperties();

private:
    WORLD::ISceneNodeUser* m_EditedNode;
};

}