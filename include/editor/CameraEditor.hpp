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
    CameraEditor(EditorFlags flags, WORLD::Camera* camera);
    CameraEditor(CameraEditor&& rhs);

    CameraEditor& operator=(CameraEditor&& rhs);

    virtual ~CameraEditor() {}

    virtual void Render() override;

private:
    WORLD::Camera* m_Camera;
};

}