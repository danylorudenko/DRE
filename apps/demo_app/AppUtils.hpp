#pragma once

#include <foundation/memory/ByteBuffer.hpp>

void GeneratePlaneMesh(std::uint32_t width, std::uint32_t height, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut);

void GenerateSphereMesh(std::uint32_t resolution, DRE::ByteBuffer& vertexOut, DRE::ByteBuffer& indexOut);


