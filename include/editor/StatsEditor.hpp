#pragma once

#include <editor\BaseEditor.hpp>

namespace WORLD
{
class Camera;
}


namespace EDITOR
{

class StatsEditor : public BaseEditor
{
public:
    StatsEditor(BaseEditor* rootEditor, EditorFlags flags);
    StatsEditor(StatsEditor&& rhs);

    StatsEditor& operator=(StatsEditor&& rhs);

    virtual ~StatsEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::Stats; }

    virtual void Render() override;
};

}