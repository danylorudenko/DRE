#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include <glm\vec3.hpp>
#include <glm\vec2.hpp>

#include <engine\data\Material.hpp>

namespace Data
{

class Geometry
{
public:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 norm;
        glm::vec3 tan;
        glm::vec3 btan;
        glm::vec2 uv0;
    };

    void AddVertex(
        glm::vec3 const& pos, 
        glm::vec3 const& norm = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 const& tan = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 const& bTan = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec2 const& uv0 = glm::vec2(0.0f, 0.0f)
    );

    void AddIndex(std::uint32_t index);


private:
    std::vector<Vertex>        m_VertexStorage;
    std::vector<std::uint32_t> m_IndexStorage;
};

////////////////////////////////////////////////////////

class Model
{
public:
    Model();

private:
    using Surface = std::pair<Geometry, Material>;

    Surface m_MainSurface;
};

}

