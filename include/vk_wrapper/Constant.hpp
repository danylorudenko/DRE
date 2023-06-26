#pragma once

#include <cstdint>

//#define DRE_COMPILE_FOR_RENDERDOC

namespace VKW
{

namespace CONSTANTS
{

std::uint8_t constexpr FRAMES_BUFFERING             = 2;
std::uint8_t constexpr MAX_COMMANDLIST_PER_QUEUE    = 20;
std::uint8_t constexpr MAX_COLOR_ATTACHMENTS        = 5;

std::uint32_t constexpr MAX_ALLOCATIONS = 128;

std::uint32_t constexpr TEXTURE_DESCRIPTOR_HEAP_SIZE = 100;

std::uint16_t constexpr MAX_SET_LAYOUT_MEMBERS      = 6;
std::uint16_t constexpr MAX_PIPELINE_LAYOUT_MEMBERS = 6;
std::uint8_t  constexpr MAX_PUSH_CONSTANTS          = 1;

}

}