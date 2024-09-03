#pragma once

#include <foundation\Common.hpp>
#include <glm\vec3.hpp>
#include <glm\vec2.hpp>
#include <glm\mat4x4.hpp>

DRE_BEGIN_NAMESPACE

struct Line2D // at + b
{
    float a;
    float b;

    inline float Evaluate(float t) const { return a * t + b; }
};

struct Circle2D // (x-h)^2 + (y-k)^2 = r^2
{
    float x;
    float y;
    float r;
    float h;
    float k;
};

struct Sphere
{
    glm::vec3 c;
    float     r;
};

struct Ray
{
    glm::vec3 origin;
    glm::vec3 dir;

    glm::vec3 Evaluate(float t) { return dir * t + origin; }
};

struct Cylinder
{
    glm::vec3 p0;
    glm::vec3 p1;
    float     r;
};

struct Plane // nX*x + nY*y + nZ*z + d = 0
{
    glm::vec3 n;
    float     d;
};

//Ray     RayFromCamera(glm::uvec2 mousePos, glm::uvec2 screenSize, glm::mat4 iVP, glm::vec3 cameraPos);
Ray     RayFromCamera(glm::uvec2 pixel, glm::uvec2 screenSize, float fovDeg, glm::mat4 const& iV);
Plane   PlaneFromNormalAndPoint(glm::vec3 n, glm::vec3 p);

bool    LineCircleIntersection(Line2D const& line, Circle2D const& circle, float& t1, float& t2);

bool    RayPlaneIntersection(Ray const& r, Plane const& p, float& t);
bool    RayCylinderIntersection(Ray const& r, Cylinder const& c, float& t1, float& t2);

DRE_END_NAMESPACE

