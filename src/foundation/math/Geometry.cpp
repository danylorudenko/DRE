#pragma once

#include <foundation\math\Geometry.hpp>
#include <glm\glm.hpp>


DRE_BEGIN_NAMESPACE

bool LineCircleIntersection(Line2D const& line, Circle2D const& circle, float& t0, float& t1)
{
    float c = line.b - circle.k;

    float A = 1.0f + line.a * line.a;
    float B = -2.0f * circle.h + 2 * line.a * c;
    float C = circle.h * circle.h + c * c - circle.r * circle.r;

    float sqrt = glm::sqrt(B*B - 4*A*C);

    t0 = (-B + sqrt) / 4*A;
    t1 = (-B - sqrt) / 4*A;

    return ((t0 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f) || ((t1 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f);
}

bool RayCylinderIntersection(Ray const& r, Cylinder const& c, float& outT)
{


    return false;
}

DRE_END_NAMESPACE

