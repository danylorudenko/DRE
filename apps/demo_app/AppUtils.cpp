#include <demo_app/AppUtils.hpp>

#include <foundation/memory/MemoryOps.hpp>
#include <engine/data/Geometry.hpp>

void GeneratePlaneMesh(std::uint32_t width, std::uint32_t height, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut)
{
	std::uint32_t constexpr vertexSize = sizeof(Data::DREVertex);
	std::uint32_t constexpr indexSize = sizeof(std::uint32_t);

	std::uint32_t const vertexMemorySize = (width) * (height) * vertexSize;
	std::uint32_t const indexMemorySize = indexSize * width * height * 6;

	vertexOut.Resize(vertexMemorySize);
	indexOut.Resize(indexMemorySize);

	void* vertexMemory = vertexOut.Data();
	void* indexMemory = indexOut.Data();

	Data::DREVertex v;
	for (std::uint32_t y = 0; y < height; y++)
	{
		for (std::uint32_t x = 0; x < width; x++)
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
	std::uint32_t const quadCount = (width - 1) * (height - 1);
	for (std::uint32_t i = 0; i < quadCount; i++)
	{
		std::uint32_t const quadX = i % width;
		std::uint32_t const quadY = i / width;

		std::uint32_t const v0 = quadX + quadY * (width);
		std::uint32_t const v1 = v0 + width;
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

void GenerateSphereMesh(std::uint32_t resolution, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut)
{

}