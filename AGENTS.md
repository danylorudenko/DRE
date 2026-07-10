# DRE � GitHub Copilot Instructions

## Project Overview
DRE is a C++20 Vulkan real-time rendering engine targeting Windows only (MSVC, Visual Studio 2022).
Third-party dependencies: GLM (math), Assimp (asset import), Dear ImGui (UI), slang shader language (for shaders).
The Vulkan API is fully wrapped under the `VKW` namespace � never call Vulkan directly.

---

## Language & Standard
- C++20, MSVC, Visual Studio 2022, Windows only (Win32 API)
- `#pragma once` in every header � no include guards
- No exceptions anywhere in the codebase

---

## Primitive Types  *(from `<foundation\Common.hpp>`)*
**Always** use DRE aliases. Do NOT write `std::uint32_t`, `int`, `size_t` etc. in engine code.

| DRE alias | Underlying |
|---|---|
| `U8`, `U16`, `U32`, `U64` | `std::uint8/16/32/64_t` |
| `S8`, `S16`, `S32`, `S64` | `std::int8/16/32/64_t` |
| `SizeT` | `std::size_t` |
| `PtrDiff` | `std::ptrdiff_t` |
| `UPtr` | `std::uintptr_t` |
| `Char`, `WChar` | `S8`, `S16` |

---

## DRE Macros  *(from `<foundation\Common.hpp>`)*
Use these instead of the underlying C++ equivalents everywhere in engine code:

| Macro | Replaces |
|---|---|
| `DRE_MOVE(x)` | `std::move(x)` |
| `DRE_SWAP(a, b)` | `std::swap(a, b)` |
| `DRE_SWAP_MEMBER(m)` | `std::swap(m, rhs.m)` |
| `DRE_ASSERT(cond, msg)` | `assert(...)` |
| `DRE_DEBUG_ONLY(expr)` | stripped in release builds |
| `DRE_LOG(fmt, ...)` | `OutputDebugStringA` formatted |
| `DRE_BEGIN_NAMESPACE` / `DRE_END_NAMESPACE` | `namespace DRE {` / `}` |

Limit macros: `DRE_U32_MAX`, `DRE_S32_MAX`, `DRE_FLT_EPS`, etc.

---

## Containers *(STL containers are legacy leftovers � do NOT introduce new STL container usage)*

### `DRE::InplaceVector<T, STORAGE_SIZE>`  *(`<foundation\container\InplaceVector.hpp>`)*
Fixed compile-time capacity, no heap allocation, order not preserved on removal.
- Copy/move assignment **requires empty destination** � `DRE_ASSERT(m_Size == 0, ...)`
- `EmplaceBack(args...)` � asserts on overflow
- `EmplaceBackUnique(value)` � inserts only if not already present, returns ref
- `RemoveIndex(U32)` � swaps last element into the gap, O(1)
- `RemovePtr(T*)` � removes by pointer
- `RemoveValue(T const&)` � finds and removes first match
- `Find(T const&)` / `FindIf(predicate)` ? `U32` index, returns `Size()` if not found
- `SortBubble(predicate)`
- `Clear()`, `Last()`, `Size()`, `Empty()`, `Data()`, `Capacity()`, `SizeInBytes()`
- `ResizeUnsafe(U32)` � sets size without constructing/destructing

### `DRE::Vector<T, TAllocator>`  *(`<foundation\container\Vector.hpp>`)*
Dynamic, heap-backed with SVO (12-element inplace buffer). Requires allocator pointer.
- `Vector(TAllocator* allocator)`
- `EmplaceBack(args...)`, `RemoveIndex(U32)`, `Reserve(U32)`, `Resize(U32)`, `Clear()`
- `Find(T const&)` / `FindIf(predicate)` ? `U32`, returns `Size()` if not found
- `Reset(TAllocator*)` � clears and reassigns allocator

### `DRE::InplaceHashTable<TKey, TValue, BUCKET_COUNT = 256>`  *(`<foundation\container\InplaceHashTable.hpp>`)*
Fixed-capacity open hash map with inplace storage and a collision pool.

### `DRE::InplaceString<TChar, SIZE>` / `DRE::String32/64/128/256/512`  *(`<foundation\string\InplaceString.hpp>`)*
Fixed-size inplace strings, no heap. Implicitly converts to `const char*`.
- `Append(str)`, `AppendFormat(fmt, ...)`, `Shrink(U16)`, `GetData()`, `GetSize()`
- Comparable to `const char*` via `operator==` / `operator!=`

### `DRE::ByteBuffer`  *(`<foundation\memory\ByteBuffer.hpp>`)*
Owning heap buffer for raw byte data. Used for geometry / shader binary storage.

### `DRE::ParallelFor<MAX>(count, func, parallel)`  *(`<foundation\system\Parallel.hpp>`)*
Parallel task dispatch. Returns a `ParallelTaskGroup<MAX>` � call `.Wait()` on it.
`std::future` / `std::async` are acceptable at this system-layer boundary only.

---

## Memory & Allocators  *(`<foundation\memory\Memory.hpp>`)*

| Allocator | Type | Use for |
|---|---|---|
| `DRE::g_MainAllocator` | `AllocatorBuddy` | Persistent engine allocations |
| `DRE::g_PersistentDataAllocator` | `AllocatorLinear` | Persistent data, never freed individually |
| `DRE::g_FrameScratchAllocator` | `AllocatorLinear` | Per-frame scratch, reset each frame |

Allocator interface: `Alloc(size, alignment)`, `Free(ptr)`.
- Do **not** use raw `new` / `delete` in engine code
- `std::make_unique` / `std::unique_ptr` is acceptable at the application boundary only

---

## Namespaces

| Namespace | Purpose |
|---|---|
| `DRE` | Foundation: types, allocators, containers, math, strings |
| `VKW` | Vulkan wrapper: device, context, pipelines, resources, descriptors |
| `GFX` | Graphics: manager, render passes, materials, render graph, texture bank |
| `WORLD` | Scene graph, entities, camera, lights |
| `IO` | IOManager, ShaderDB, asset loading, GLTF parsing |
| `SYS` | System: Window, InputSystem |
| `EDITOR` | Editor tools, viewport input, inspectors |
| `Data` | CPU-side data: Geometry, Texture2D, Material |

---

## Naming Conventions

| Category | Pattern | Example |
|---|---|---|
| Member variables | `m_` + PascalCase | `m_GraphicsManager`, `m_MainScene` |
| Classes / Structs | PascalCase | `GraphicsManager`, `RenderableObject` |
| Methods | PascalCase | `GetMainContext()`, `RenderFrame()` |
| `constexpr` constants | `C_` + ALL_CAPS | `C_COMPILE_HLSL_SOURCES_ON_START` |
| Local variables | camelCase | `deltaSeconds`, `sunLight` |
| Global extern instances | `g_` prefix | `DRE::g_MainAllocator`, `GFX::g_GraphicsManager` |
| Template parameters | PascalCase | `TAllocator`, `TPredicate`, `STORAGE_SIZE` |

---

## Class Design
- Non-copyable: inherit `NonCopyable` from `<foundation\class_features\NonCopyable.hpp>`
- Non-movable: inherit `NonMovable` from `<foundation\class_features\NonMovable.hpp>`
- Use `final` on concrete leaf classes: `class GraphicsManager final`
- Prefer composition over inheritance
- Headers under `include\`, sources under `src\`
- Constructor member-initializer lists: one member per line, `, ` aligned

---

## Include Style
Use **backslash** path separators in angle-bracket includes:
```cpp
#include <foundation\container\Vector.hpp>
#include <vk_wrapper\pipeline\Pipeline.hpp>
```
Group order: standard library ? third-party (glm, assimp, imgui) ? engine headers

---

## Vulkan / GPU Architecture
- All GPU work goes through `VKW::Context` � never call Vulkan API directly
- Pipeline creation & management: `GFX::PipelineDB`
- Texture loading & lookup: `GFX::TextureBank`
- Per-instance GPU data: `GFX::InstanceDataManager`
- Per-material GPU data: `GFX::MaterialsManager`
- Geometry: `GFX::GlobalGeometry`
- Ray tracing AS: `GFX::RayTracingManager`
- Per-frame buffered resources indexed by `FrameID` (`U8`, value in `[0, FRAMES_BUFFERING)`)
- Flush GPU work: `context.FlushAll()`

## Render Graph
- Render passes register resources via `GFX::RenderGraph`
- Named graph resources use `GFX::TextureID` / `GFX::BufferID` enums (`<gfx\scheduling\GraphResource.hpp>`)
- Dependency tracking via `GFX::DependencyManager`

## Shader System
- slang shaders compiled at startup: `IO::ShaderDB::CompileSources(bool parallel)`
- Shader entries retrieved via `ShaderDB::GetShaderEntry(String64 const& name)`
- Shader binding interface described by `IO::ShaderInterface` (members, push constants)
- Hot-reload pending: `GFX::PipelineDB::ReloadPipeline(char const* name)`
- Force reload all: `GFX::PipelineDB::ReloadAllPipelines()`

## Material System
- CPU-side materials are managed by `Data::MaterialLibrary`
- GPU-side materials are managed by `GFX::MaterialsManager`
- GPU-side materials are accessed by GPU pointer `S_MATERIAL*` present in `S_INSTANCE` struct

---

## Scene Architecture
- Scene root: `WORLD::Scene` � global access via `WORLD::g_MainScene`
- Objects attach to `WORLD::SceneNode` via `WORLD::ISceneNodeUser`
- Entity types: `WORLD::Entity` (opaque renderables), `WORLD::Light`
- Camera: `m_MainScene.GetMainCamera()` � `SetPosition`, `SetCameraEuler`, `Move`, `RotateCamera`

## GPU Scene
- GPU scene data (object matrices, material indices, light data) managed by `GFX::InstanceDataManager`

---

## Anti-Patterns � Never Do These
- `std::vector`, `std::unordered_map`, `std::string` in engine code � use DRE containers
- `std::move(x)` � use `DRE_MOVE(x)`
- `std::swap(a, b)` � use `DRE_SWAP(a, b)`
- `assert(...)` � use `DRE_ASSERT(cond, msg)`
- `std::uint32_t` / raw `int` for sizes/indices � use `U32` / `S32`
- Raw `new` / `delete` in engine code � use allocators
- Vulkan API calls outside `VKW`
- Exceptions
