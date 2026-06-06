#include <engine\data\GeometryLibrary.hpp>

#include <foundation\util\Hash.hpp>
#include <glm\ext\scalar_constants.hpp>
#include <glm\trigonometric.hpp>
#include <gfx\GraphicsManager.hpp>

namespace Data
{

GeometryLibrary::Hash::Hash(char const* name)
{
    DRE::U32 const nameSize = std::strlen(name);
    m_Hash = fasthash32(name, nameSize, DRE::U32(0xE527A10B));
}

GeometryLibrary::Hash::Hash(DRE::U32 id, const char* sceneName)
{
    DRE::U32 buffer[32];
    DRE::MemZero(buffer, sizeof(buffer));

    void* ptr = buffer;
    DRE::WriteMemorySequence(ptr, id);

    DRE::U32 const nameSize = std::strlen(sceneName);
    DRE_ASSERT(nameSize < sizeof(buffer) - 5, "GeometryLibrary doesn't support big file names.");
    DRE::WriteMemorySequence(ptr, sceneName, nameSize);

    m_Hash = fasthash32(buffer, sizeof(buffer), uint32_t(0xE527A10B));
}

GeometryLibrary::GeometryLibrary(DRE::DefaultAllocator* allocator)
    : m_Geometries{ allocator }
{
}

void GeometryLibrary::AddGeometry(DRE::U32 id, char const* sceneName, Geometry&& data)
{
    AddGeometry(Hash(id, sceneName), DRE_MOVE(data));
}

void GeometryLibrary::AddGeometry(Hash hash, Geometry&& data)
{
    m_Geometries.Emplace(hash, DRE_MOVE(data));
}

void GeometryLibrary::AddGeometry(char const* name, Geometry&& data)
{
    AddGeometry(Hash(name), DRE_MOVE(data));
}

void GeometryLibrary::GeneratePlaneMesh(DRE::U32 width, DRE::U32 height, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut)
{
    DRE::U32 constexpr vertexSize = sizeof(Data::DREVertex);
    DRE::U32 constexpr indexSize = sizeof(DRE::U32);

    DRE::U32 const vertexMemorySize = (width) * (height)*vertexSize;
    DRE::U32 const indexMemorySize = indexSize * width * height * 6;

    vertexOut.Resize(vertexMemorySize);
    indexOut.Resize(indexMemorySize);

    void* vertexMemory = vertexOut.Data();
    void* indexMemory = indexOut.Data();

    Data::DREVertex v;
    for (DRE::U32 y = 0; y < height; y++)
    {
        for (DRE::U32 x = 0; x < width; x++)
        {
            v.pos[0] = static_cast<float>(x) - static_cast<float>(width) / 2;
            v.pos[1] = 0.0f;
            v.pos[2] = static_cast<float>(y) - static_cast<float>(height) / 2;

            v.norm[0] = 0.0f;
            v.norm[1] = 1.0f;
            v.norm[2] = 0.0f;

            v.tan[0] = 1.0f;
            v.tan[1] = 0.0f;
            v.tan[2] = 0.0f;

            v.btan[0] = 0.0f;
            v.btan[1] = 0.0f;
            v.btan[2] = 1.0f;

            v.uv0[0] = static_cast<float>(x) / (width);
            v.uv0[1] = static_cast<float>(y) / (height);

            DRE::WriteMemorySequence(vertexMemory, &v, sizeof(v));
        }
    }

    /*

    0--------2
    |		 |
    |	     |
    |		 |
    1--------3

    */
    DRE::U32 const quadCount = (width - 1) * (height - 1);
    for (DRE::U32 i = 0; i < quadCount; i++)
    {
        DRE::U32 const quadX = i % width;
        DRE::U32 const quadY = i / width;

        DRE::U32 const v0 = quadX + quadY * (width);
        DRE::U32 const v1 = v0 + width;
        DRE::U32 const v2 = v0 + 1;
        DRE::U32 const v3 = v1 + 1;

        DRE::U32 ids[6];
        ids[0] = v0;
        ids[1] = v2;
        ids[2] = v1;

        ids[3] = v2;
        ids[4] = v3;
        ids[5] = v1;

        DRE::WriteMemorySequence(indexMemory, ids, sizeof(ids));
    }
}

void GeometryLibrary::GenerateSphereMesh(DRE::U32 verticalResolution, DRE::U32 horizontalResolution, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut)
{
    DRE_ASSERT(verticalResolution >= 2, "Sphere verticalResolution must be at least 2.");
    DRE_ASSERT(horizontalResolution >= 2, "Sphere horizontalResolution must be at least 2.");

    DRE::U32 constexpr vertexSize = sizeof(Data::DREVertex);
    DRE::U32 constexpr indexSize = sizeof(DRE::U32);

    // (verticalResolution+1) rings (latitude) x (horizontalResolution+1) columns (longitude)
    DRE::U32 const ringCount = verticalResolution + 1;
    DRE::U32 const columnCount = horizontalResolution + 1;

    DRE::U32 const vertexCount = ringCount * columnCount;
    DRE::U32 const quadCount = verticalResolution * horizontalResolution;

    vertexOut.Resize(vertexCount * vertexSize);
    indexOut.Resize(quadCount * 6 * indexSize);

    void* vertexMemory = vertexOut.Data();
    void* indexMemory = indexOut.Data();

    float constexpr tau = 2.0f * glm::pi<float>();

    // Vertices: iterate phi (latitude, 0..pi) then theta (longitude, 0..2pi)
    for (DRE::U32 ring = 0; ring < ringCount; ring++)
    {
        float const phi = glm::pi<float>() * static_cast<float>(ring) / static_cast<float>(verticalResolution);

        float const sinPhi = glm::sin(phi);
        float const cosPhi = glm::cos(phi);

        for (DRE::U32 col = 0; col < columnCount; col++)
        {
            float const theta = tau * static_cast<float>(col) / static_cast<float>(horizontalResolution);

            float const sinTheta = glm::sin(theta);
            float const cosTheta = glm::cos(theta);

            // Unit sphere position (normal = position on unit sphere)
            float const nx = sinPhi * cosTheta;
            float const ny = cosPhi;
            float const nz = sinPhi * sinTheta;

            // Tangent: derivative of position with respect to theta (longitude)
            float const tx = -sinPhi * sinTheta;
            float const ty = 0.0f;
            float const tz = sinPhi * cosTheta;

            // Bitangent: derivative of position with respect to phi (latitude)
            float const bx = cosPhi * cosTheta;
            float const by = -sinPhi;
            float const bz = cosPhi * sinTheta;

            Data::DREVertex v;
            v.pos[0] = nx; v.pos[1] = ny; v.pos[2] = nz;
            v.norm[0] = nx; v.norm[1] = ny; v.norm[2] = nz;
            v.tan[0] = tx; v.tan[1] = ty; v.tan[2] = tz;
            v.btan[0] = bx; v.btan[1] = by; v.btan[2] = bz;
            v.uv0[0] = static_cast<float>(col) / static_cast<float>(horizontalResolution);
            v.uv0[1] = static_cast<float>(ring) / static_cast<float>(verticalResolution);

            DRE::WriteMemorySequence(vertexMemory, &v, sizeof(v));
        }
    }

    /*

    v0-------v2
    |         |
    |         |
    v1-------v3

    */
    for (DRE::U32 ring = 0; ring < verticalResolution; ring++)
    {
        for (DRE::U32 col = 0; col < horizontalResolution; col++)
        {
            DRE::U32 const v0 = ring * columnCount + col;
            DRE::U32 const v1 = v0 + columnCount;
            DRE::U32 const v2 = v0 + 1;
            DRE::U32 const v3 = v1 + 1;

            DRE::U32 ids[6];
            ids[0] = v0;
            ids[1] = v2;
            ids[2] = v1;

            ids[3] = v2;
            ids[4] = v3;
            ids[5] = v1;

            DRE::WriteMemorySequence(indexMemory, ids, sizeof(ids));
        }
    }
}

void GeometryLibrary::LoadDefaultGeometry()
{
    {
        DRE::ByteBuffer planeVertices;
        DRE::ByteBuffer planeIndices;
        GeneratePlaneMesh(10, 10, planeVertices, planeIndices);

        Geometry planeGeometry{ sizeof(Data::DREVertex), sizeof(Data::DREIndex) };
        planeGeometry.SetVertexData(DRE_MOVE(planeVertices));
        planeGeometry.SetIndexData(DRE_MOVE(planeIndices));

        AddGeometry("dre_plane", DRE_MOVE(planeGeometry));
    }

    {
        DRE::ByteBuffer sphereVertices;
        DRE::ByteBuffer sphereIndices;
        GenerateSphereMesh(20, 20, sphereVertices, sphereIndices);

        Geometry sphereGeometry{ sizeof(Data::DREVertex), sizeof(Data::DREIndex) };
        sphereGeometry.SetVertexData(DRE_MOVE(sphereVertices));
        sphereGeometry.SetIndexData(DRE_MOVE(sphereIndices));

        AddGeometry("dre_sphere", DRE_MOVE(sphereGeometry));
    }

}

}

