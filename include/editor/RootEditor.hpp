#pragma once

#include <editor\BaseEditor.hpp>

#include <foundation\container\Vector.hpp>
#include <foundation\memory\Memory.hpp>

namespace WORLD
{
class Scene;
}

namespace EDITOR
{

class CameraEditor;

class RootEditor : public BaseEditor
{
public:
    RootEditor(WORLD::Scene* mainScene);
    RootEditor(RootEditor&& rhs);

    RootEditor& operator=(RootEditor&& rhs);

    void CloseEditor(BaseEditor* editor);

    virtual ~RootEditor();

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::Root; }

    virtual void Render() override;
    virtual void Close() override;

    BaseEditor* GetEditorByType(BaseEditor::Type type);

private:
    WORLD::Scene* m_MainScene;

    DRE::Vector<BaseEditor*, DRE::DefaultAllocator> m_Editors;
    DRE::Vector<BaseEditor*, DRE::DefaultAllocator> m_CloseQueue;
};

}