#pragma once

#include <foundation\Common.hpp>
#include <glm\vec3.hpp>

DRE_BEGIN_NAMESPACE

struct Line2D // at + b
{
    float a;
    float b;
};

struct Circle2D // (x-h)^2 + (y-k)^2 = r^2
{
    float x;
    float y;
    float r;
    float h;
    float k;
};

struct Line // "at + b" for each axis
{
    glm::vec3 a;
    glm::vec3 b;
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

bool LineCircleIntersection(Line2D const& line, Circle2D const& circle, float& t1, float& t2);

bool RayCylinderIntersection(Ray const& r, Cylinder const& c, float& t1, float& t2);

DRE_END_NAMESPACE

