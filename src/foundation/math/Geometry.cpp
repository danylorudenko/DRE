#pragma once

#include <foundation\math\Geometry.hpp>

#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>
#include <foundation\math\SimpleMath.hpp>


DRE_BEGIN_NAMESPACE

Ray RayFromCamera(glm::uvec2 mousePos, glm::uvec2 screenSize, glm::mat4 iVP, glm::vec3 cameraPos)
{
    glm::vec2 screenHalf = glm::vec2{ screenSize } / 2.0f;
    glm::vec2 ndcXY = (glm::vec2{ mousePos } - screenHalf) / (screenHalf);

    glm::vec4 rayPNDC = glm::vec4{ ndcXY.x, ndcXY.y, 1.0f, 1.0f };

    glm::vec4 rayPWorld = iVP * rayPNDC;
    glm::vec3 rayDirWorld = glm::normalize(glm::vec3{ rayPWorld } / rayPWorld.w - cameraPos);

    return Ray{ cameraPos, glm::vec3{ rayDirWorld } };
}

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

Plane PlaneFromNormalAndPoint(glm::vec3 n, glm::vec3 p)
{
    float d = -glm::dot(n, p);

    return Plane{ n, d };
}

bool RayPlaneIntersection(Ray const& r, Plane const& p, float& t)
{
    // nX*x + nY*y + nZ*z + d = 0

    float const nominator = -glm::dot(p.n, r.origin) + p.d;
    float const denominator = glm::dot(p.n, r.dir);

    t = nominator / denominator;

    return !DRE::FloatCompare(denominator, 0.0f, 0.001f);
}

bool RayCylinderIntersection(Ray const& r, Cylinder const& c, float& t1, float& t2)
{
    glm::vec3 const localY = glm::normalize(c.p1 - c.p0);

    glm::vec3 arbitrary = glm::abs(glm::dot(glm::vec3{ 1.0f, 0.0f, 0.0f }, localY)) < 0.99f ? glm::vec3{ 1.0f, 0.0f, 0.0f } : glm::vec3{ 0.0f, 0.0f, 1.0f };
    glm::vec3 localZ = glm::normalize(glm::cross(arbitrary, localY));
    glm::vec3 localX = glm::cross(localY, localZ);

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

    glm::vec3 pos1 = rayDirLocal * t1 + rayStartLocal;
    glm::vec3 pos2 = rayDirLocal * t2 + rayStartLocal;

    float const rsq = c.r * c.r;
    float d1 = pos1.x * pos1.x + pos1.z * pos1.z;
    float d2 = pos2.x * pos2.x + pos2.z * pos2.z;

    float const shaftLength = glm::length(c.p1 - c.p0);
    bool const intersecion1 = pos1.y > 0.0f && pos1.y < shaftLength && DRE::FloatCompare(rsq, d1, 0.001);
    bool const intersecion2 = pos2.y > 0.0f && pos2.y < shaftLength && DRE::FloatCompare(rsq, d2, 0.001);

    return intersecion1 || intersecion2;
}

DRE_END_NAMESPACE

