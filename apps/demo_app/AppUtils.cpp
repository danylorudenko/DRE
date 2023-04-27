#include <demo_app/AppUtils.hpp>

#include <foundation/memory/MemoryOps.hpp>
#include <engine/data/Geometry.hpp>

void GeneratePlaneMesh(std::uint32_t width, std::uint32_t height, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut)
{
	std::uint32_t constexpr vertexSize = sizeof(float) * 3;
	std::uint32_t constexpr indexSize = sizeof(std::uint32_t);

	std::uint32_t const vertexMemorySize = (width + 1) * (height + 1) * vertexSize;
	std::uint32_t const indexMemorySize = indexSize * width * height * 6;

	vertexOut.Resize(vertexMemorySize);
	indexOut.Resize(indexMemorySize);

	void* vertexMemory = vertexOut.Data();
	void* indexMemory = indexOut.Data();

	float pos[3];
	for (std::uint32_t x = 0; x < width + 1; x++)
	{
		for (std::uint32_t y = 0; y < height + 1; y++)
		{
			pos[0] = static_cast<float>(x) - static_cast<float>(width) / 2;
			pos[1] = 0.0f;
			pos[2] = static_cast<float>(y) - static_cast<float>(height) / 2;

			DRE::WriteMemorySequence(vertexMemory, pos, sizeof(pos));
		}
	}

	/*
	
	0--------2
	|		 |
	|	     |
	|		 |
	1--------3
	
	*/
	std::uint32_t const quadCount = width * height;
	for (std::uint32_t i = 0; i < quadCount; i++)
	{
		std::uint32_t const quadX = i % height;
		std::uint32_t const quadY = i / height;

		std::uint32_t const v0 = quadX + quadY * (height + 1);
		std::uint32_t const v1 = v0 + width + 1;
		std::uint32_t const v2 = v0 + 1;
		std::uint32_t const v3 = v1 + 1;

		std::uint32_t ids[6];
		ids[0] = v0;
		ids[1] = v1;
		ids[2] = v2;
		
		ids[3] = v2;
		ids[4] = v1;
		ids[5] = v3;

		DRE::WriteMemorySequence(indexMemory, ids, sizeof(ids));
	}
}