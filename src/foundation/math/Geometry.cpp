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
    glm::vec3 const localY = glm::normalize(c.p1 - c.p0);

    glm::vec3 arbitrary = glm::abs(glm::dot(glm::vec3{ 1.0f, 0.0f, 0.0f }, localY)) < 0.99f ? glm::vec3{ 1.0f, 0.0f, 0.0f } : glm::vec3{ 0.0f, 0.0f, 1.0f };
    glm::vec3 localX = glm::normalize(glm::cross(arbitrary, localY));
    glm::vec3 localZ = glm::cross(localY, localX);

    glm::mat4 toLocal{
        glm::vec4{ localX, 0.0f },
        glm::vec4{ localY, 0.0f },
        glm::vec4{ localZ, 0.0f },
        glm::vec4{ c.p0, 1.0f }
    };

    toLocal = glm::inverse(toLocal);

    glm::vec3 const rayStartLocal = toLocal * glm::vec4{ r.origin, 1.0 };
    glm::vec3 const rayDirLocal = toLocal * glm::vec4{ r.dir, 0.0f };

    float A = rayDirLocal.x * rayDirLocal.x + rayDirLocal.z * rayDirLocal.z;
    float B = 2 * (rayDirLocal.x * rayStartLocal.x + rayDirLocal.z * rayStartLocal.z);
    float C = rayStartLocal.x * rayStartLocal.x + rayStartLocal.z * rayStartLocal.z - c.r * c.r;

    float sqrt = glm::sqrt(B * B - 4 * A * C);

    t1 = (-B+sqrt) / (2*A);
    t2 = (-B-sqrt) / (2*A);

    return ((t1 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f) || ((t2 - DRE_FLT_EPS) > DRE_FLT_EPS * 2.0f);
}

DRE_END_NAMESPACE

