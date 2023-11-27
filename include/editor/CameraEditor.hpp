#pragma once

#include <editor\BaseEditor.hpp>

namespace WORLD
{
class Camera;
}


namespace EDITOR
{

class CameraEditor : public BaseEditor
{
public:
    CameraEditor(BaseEditor* rootEditor, EditorFlags flags, WORLD::Camera* camera);
    CameraEditor(CameraEditor&& rhs);

    CameraEditor& operator=(CameraEditor&& rhs);

    virtual ~CameraEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::Camera; }

    virtual void Render() override;

private:
    WORLD::Camera* m_Camera;
};

}