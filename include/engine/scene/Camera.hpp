#pragma once

#include <engine\scene\Positional.hpp>

namespace WORLD
{

class Camera : public Positional
{
public:
    Camera();

    inline glm::vec2        GetRange() const { return glm::vec2{ 0.1f, 100.0f }; }
    inline float            GetFOV() const { return m_FOV; }
    void                    SetFOV(float fov);

protected:
    virtual void            CalculateVectors() override;

private:
    float       m_FOV;
};

}

