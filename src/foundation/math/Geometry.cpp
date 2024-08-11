#pragma once

#include <foundation\math\Geometry.hpp>
#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>


DRE_BEGIN_NAMESPACE

bool LineCircleIntersection(Line2D const& line, Circle2D const& circle, float& t1, float& t2)
{
    float c = line.b - circle.k;

    float A = 1.0f + line.a * line.a;
    float B = -2.0f * circle.h + 2 * line.a * c;
    float C = circle.h * circle.h + c * c - circle.r * circle.r;

    float sqrt = glm::sqrt(B*B - 4*A*C);

    t1 = (-B + sqrt) / 4*A;
    t2 = (-B - sqrt) / 4*A;

    return ((t1 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f) || ((t2 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f);
}

bool RayCylinderIntersection(Ray const& r, Cylinder const& c, float& t1, float& t2)
{
    glm::vec3 const shaft = c.p1 - c.p0;
    float const shaftLen = glm::length(shaft);
    float const pitch = glm::asin(shaft.y / shaftLen) - glm::pi<float>() / 2.0f; 
    float const yaw = glm::atan(shaft.z / glm::max(shaft.x, 0.00001f));

    glm::quat const q{ glm::vec3{ pitch, yaw, 0.0f } };
    glm::quat const invq = glm::inverse(q);

    glm::mat3 const toLocal{ invq };

    glm::vec3 p1Local = c.p1 - c.p0;
    p1Local = toLocal * p1Local;

    glm::vec3 const rayStartLocal = r.origin - c.p0;
    glm::vec3 const rayDirLocal = toLocal * r.dir;

    float A = rayDirLocal.x * rayDirLocal.x + rayDirLocal.z * rayDirLocal.z;
    float B = 2 * (rayDirLocal.x * rayStartLocal.x + rayDirLocal.z * rayStartLocal.z);
    float C = rayStartLocal.x * rayStartLocal.x + rayStartLocal.z * rayStartLocal.z - c.r * c.r;

    float sqrt = glm::sqrt(B * B - 4 * A * C);

    t1 = (-B+sqrt) / (2*A);
    t2 = (-B-sqrt) / (2*A);

    return ((t1 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f) || ((t2 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f);
}

DRE_END_NAMESPACE

