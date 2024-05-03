#pragma once

#include <foundation\Common.hpp>

#include <editor\BaseEditor.hpp>

namespace WORLD
{
class ISceneNodeUser;
}

namespace EDITOR
{

class LightPropertiesEditor : public BaseEditor
{
public:
    LightPropertiesEditor(BaseEditor* rootEditor, EditorFlags flags);
    LightPropertiesEditor(LightPropertiesEditor&& rhs);

    LightPropertiesEditor& operator=(LightPropertiesEditor&& rhs);

    virtual ~LightPropertiesEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::LightProperties; }

    virtual void Render() override;

    void SetLight(WORLD::ISceneNodeUser* user);


private:
    void RenderDirectionalLightProperties();

private:
    WORLD::ISceneNodeUser* m_EditedLight;
};

}