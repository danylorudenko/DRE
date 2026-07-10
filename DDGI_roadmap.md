# DDGI: implementation roadmap from the current DRE code

This is a learning-oriented roadmap based on the code as it exists on 2026-07-10. It deliberately describes contracts, milestones, and ways to validate each stage instead of providing an implementation to paste in.

The most important conclusion is that the project already has the beginnings of a complete DDGI *integration* path: `LightingPass` invokes `CalculateIndirectLighting`, and the latter samples the probe irradiance atlas. The next task is therefore not to create a separate screen-space apply pass. It is to make the data written to that atlas physically and numerically meaningful.

## 1. What exists today

The pass order in `GraphicsManager::CreateAllPasses` is:

```text
GBuffer
  -> DDGIProbeScatterPass
  -> DDGIProbeTracePass
  -> DDGIProbeLightingPass
  -> DDGIProbeBlendPass (empty)
  -> deferred LightingPass (already samples DDGI)
```

The graph resources are created once by `GraphResourcesManager::InitResources`, so the current single irradiance texture survives from frame to frame. This is useful for history, but it also means that any texel a pass does not write still contains old or undefined data.

| Area | Current state | Evidence |
|---|---|---|
| Probe grid | Static positions are generated each frame and stored in `DDGI_ProbeData`. | `ddgi_probe_scatter.slang` |
| Ray tracing | One ray is intended per probe per frame; on a hit, material properties are placed in a ray-direction texel. | `ddgi_probe_trace.slang` |
| Probe lighting | The whole G-buffer atlas is lit and directly written into `DDGI_ProbeIrradiance`. | `ddgi_probe_lighting.slang` |
| Irradiance history | There is an in-place exponential blend in the lighting shader, but no valid current estimate to blend yet. | `ddgi_probe_lighting.slang` |
| Visibility | A single hit distance is written to an `R16_FLOAT` atlas; it is never sampled. | `DDGI.cpp`, `ddgi_probe_trace.slang` |
| Border handling | Shader and pipeline entry exist, but the shader and `DDGIProbeBlendPass` are empty, so it is never dispatched. | `ddgi_probe_border_blend.slang`, `DDGI.cpp` |
| Scene application | Present, but incomplete: `CalculateIndirectLighting` is used by `lighting_deferred.slang`. | `lighting.slang`, `lighting_deferred.slang` |
| Probe debugging | Probe spheres, a selected probe, and ray debug lines are useful foundations for validation. | `debug_view_ddgi_probes.slang`, `DebugViewEditor.cpp` |

## 2. Define the data contract before adding more passes

The word “irradiance” is currently used for data that is actually closer to sparse outgoing radiance from traced hit points. Decide on the following contract and keep the names consistent with it.

```text
probe + ray direction
    -> ray result: radiance returning to the probe, distance, validity
    -> directional filtering / cosine convolution
    -> probe irradiance E_p(n), indexed by surface normal n
    -> surface diffuse response: (albedo / pi) * E_p(n)
```

`E_p(n)` is irradiance for a surface whose normal is `n`; it is not merely the radiance from one direction. Conceptually:

```text
E_p(n) = integral over sphere of L_p(w) * max(0, dot(n, w)) dw
```

where `L_p(w)` is radiance arriving at the probe from direction `w`. At the final shaded surface, diffuse outgoing radiance is `albedo / pi * E`. This contract makes three code decisions straightforward:

- The trace stage must create a dense, valid set of ray results, including misses.
- An update stage must integrate ray radiance with a cosine lobe for every irradiance output direction. Directly copying a ray's lighting result to the same octahedral texel is not that integration.
- `CalculateIndirectLighting` must normalize probe weights and apply the receiving surface's diffuse BRDF, rather than adding sampled atlas values directly to final lighting.

For the first working version, treat DDGI as one-bounce *diffuse* indirect light. At a ray hit, compute outgoing diffuse radiance toward the probe from direct lighting at the hit. Do not use the camera as the view direction there. `CalculateDirectLighting` currently derives `v` from `GetCameraPos()`, so it is appropriate for camera shading but is not directly reusable for a probe ray result without an explicit outgoing direction (or a diffuse-only helper).

## 3. Fix these correctness blockers first

Do these before tuning ray count, temporal settings, or visibility. Otherwise the observations will be dominated by invalid data.

### 3.1 Complete ray-query traversal

`ddgi_probe_trace.slang` calls `rayQuery.Proceed()` only once and immediately examines the committed hit. Inline ray queries must be advanced until traversal is complete before consuming committed results. Make this a small isolated change, then confirm a selected probe's debug rays terminate on the closest expected geometry.

### 3.2 Bounds checks in the trace shader

The CPU dispatch rounds probe dimensions up to the `(4, 4, 4)` group size. Unlike the scatter shader, the trace shader has no early return for `DTid >= probeGridDimentions`. The extra threads compute invalid probe indices and can read/write out of bounds. Add the same bounds check before calculating `probeIndex` or reading `ddgiProbeData`.

The lighting shader likewise dispatches rounded-up atlas dimensions and has no atlas-size guard. It happens to be safe only when the atlas sizes are multiples of eight; make that condition explicit in the shader instead of relying on the present defaults.

### 3.3 Make each ray result complete and dense

With a 6x6 tile and a one-pixel border, a probe currently has only 4x4 = 16 interior texels, but one trace invocation writes at most one of them. Misses write nothing. Consequently most atlas texels contain stale/undefined G-buffer data and are then “lit” every frame.

For the first stable design, map **exactly one ray to every interior texel**. Dispatch over `(probe, interior-ray-index)`, derive the ray direction from that texel's octahedral coordinate, and make every invocation write one unique texel. This removes collisions and atomics from the learning version. Increase the interior resolution later when it is correct.

Each ray needs defined output for both outcomes:

| Result | Radiance input | Visibility distance | Validity / notes |
|---|---|---|---|
| Triangle hit | Directly lit radiance at the actual hit point, travelling back to the probe | `CommittedRayT()` | Store a validity flag if later passes need it. |
| Miss | Environment/sky radiance, or black while debugging | `maxRayDistance` | Never leave the previous frame's texel behind. |

Use an explicit maximum ray distance in DDGI settings and use it consistently for `RayDesc.TMax`, misses, visibility, and debug drawing. `0.01` is also not a robust universal `TMin`; eventually express the bias relative to probe spacing and scene scale.

### 3.4 Preserve the information needed to light the hit

`ddgi_probe_lighting.slang` reconstructs the probe index from the atlas pixel, then sets `surface.wpos` to the **probe position**. Lighting/shadow rays are therefore evaluated from the wrong place. The G-buffer currently contains material values but not the world-space hit position or enough information to recover it.

Choose one of these learning-friendly designs:

1. Trace writes a compact ray-result texture/buffer containing hit position, normal, diffuse material data, distance, and validity; a later pass lights that result at its hit position.
2. Trace shades the hit immediately and writes radiance plus distance, eliminating the intermediate material G-buffer.

The second is easier to get correct first. The first is useful if you specifically want to study a decoupled probe G-buffer pipeline. In either case, normalize transformed normals, use a correct normal matrix for non-uniform transforms, and pass the hit-to-probe direction to any view-dependent lighting function.

### 3.5 Correct texel addressing

`uint2(probeLocalUV * probeInnerRes)` can produce `probeInnerRes` when an octahedral coordinate is exactly one. That enters the border rather than the interior. Define a single helper that maps an interior texel centre to an octahedral direction and, separately, maps an octahedral direction to a clamped interior texel. Use it for trace, filtering, visibility, and sampling.

Do not treat the border as ray data. It is a post-process copy of wrapped interior values for filtered sampling only.

## 4. A practical staged pipeline

This order keeps each milestone visible and testable.

### Milestone A — dense radiance rays, no history, no visibility

**Goal:** every displayed probe sphere shows a stable directional pattern matching the local scene.

1. Keep the static scatter pass, but draw selected-probe rays from the dense per-texel directions.
2. Trace every interior direction once. Use deterministic octahedral texel-centre directions first; introduce per-frame rotation only after the fixed version works.
3. Write a complete radiance ray map. A direct hit should change with a light or occluder near the hit, not when the probe is moved through empty space.
4. Implement cosine convolution from the ray map to an irradiance map. An output irradiance texel represents an output normal; sum all ray radiances times the positive cosine with that normal and normalize by the corresponding kernel weight.
5. Fill/copy the octahedral border and display the result on debug spheres.

A small 4x4 interior map is enough to prove the pipeline, but will be blocky and noisy. Make resolution and rays-per-probe explicit constants/settings rather than leaving the relationship implicit in `GetProbeResolutionGBuffer()`.

### Milestone B — correct application to the camera shading path

**Goal:** a diffuse surface brightens from nearby indirect light without leaking through walls.

`CalculateIndirectLighting` already performs the eight-probe neighborhood lookup, but it needs these changes:

- Clamp/reject coordinates outside the probe volume before converting them to `uint3`. `GetProbeNeighborhood` currently converts a potentially negative/out-of-range base index to unsigned values and can access invalid probes. Clamp the base cell to `[0, dimensions - 2]`, or return a zero-contribution neighborhood outside the volume.
- Decide what `probeWorldOffset` means. The current start position uses `-dimensions * spacing / 2`, so the average probe position is half a cell away from `probeWorldOffset`. If the setting should be the grid centre, use the `dimensions - 1` extent convention consistently in scatter and lookup.
- The current receiver-facing test has its direction reversed: `surface.wpos - probePosition` points probe-to-surface. For a floor with an upward normal it rejects a probe above the floor. Use the surface-to-probe direction, normalize it, and apply a modest wrap/backface weight rather than an unscaled dot product.
- Accumulate `weightedIrradiance` and `weightSum`, then divide by a safe `weightSum`. The current code does not normalize after its extra normal term.
- Apply the receiving diffuse response (`surface.diffuseSpectrum / PI`) once, after reconstruction. Do not multiply the probe contribution by the hit material a second time.

Initially set all visibility weights to one. The output should already be plausible in an open scene, though it will leak through thin walls.

### Milestone C — temporal history

**Goal:** a low ray budget converges over time without smearing after a relevant change.

Separate the current-frame estimate from history:

```text
ray radiance / distance (transient)
    -> filtered current irradiance + current distance moments (transient)
    -> blend with previous persistent history
    -> current persistent irradiance + moments
    -> border copy
    -> deferred lighting samples the current persistent atlas
```

Use two persistent irradiance textures and two persistent visibility-moment textures. The existing TAA implementation in `AntiAliasingPass.cpp` is the engine pattern: `ColorHistoryBuffer0/1` are selected with `GetPrevFrameID()` and `GetCurrentFrameID()`. Add analogous DDGI resource IDs rather than trying to read and write the same history texture through different bindings.

This also requires the global descriptor in `DDGIConstantBuffer::irradianceAtlasTextureID` to name the **current completed history** for the current frame. `PrepareGlobalData` runs before `RenderGraph::Render`, so choose the target/history index deterministically before uploading the global uniform.

A simple starting update is an exponential moving average:

```text
historyNew = lerp(currentEstimate, historyOld, hysteresis)
```

Use a parameter called `hysteresis` if it weights old history, or `updateRate` if it weights new input; do not mix the two meanings. The current `irradianceUpdateRate` weights the new input. Reset/clear history when the probe layout, spacing, offset, resolution, ray distance, or relevant scene data changes. Without that, an editor change can leave a valid-looking but spatially wrong history.

Only after the basic EMA works should you add luminance clamping, change detection, gamma-encoded irradiance storage, or per-probe update budgets.

### Milestone D — visibility moments and anti-leak weighting

**Goal:** probes on the opposite side of a wall stop strongly lighting the visible surface.

The current `DDGI_AtlasVisibility` is an `R16_FLOAT` raw-distance map and is unused. Replace the persistent visibility representation with two directional moments:

```text
meanDistance, meanDistanceSquared
```

The trace result supplies `(t, t*t)` for a hit and `(maxRayDistance, maxRayDistance^2)` for a miss. Filter and temporally blend these moments with the same directional layout discipline as irradiance. Use a two-channel floating format; `R16G16_FLOAT` is a reasonable first experiment, but validate whether its precision is sufficient for your scene scale.

At a shaded point, for each neighboring probe:

1. Offset the query point a little along the receiving normal to reduce self-occlusion.
2. Compute the probe-to-query direction and distance.
3. Sample the two directional moments at that direction.
4. Compute `variance = max(meanSquared - mean * mean, epsilon)`.
5. Apply a one-sided Chebyshev visibility estimate. A common form returns one when query distance is no greater than the mean; otherwise use `variance / (variance + (distance - mean)^2)`. Bias, minimum variance, and an exponent are tuning controls, not replacements for correct moments.
6. Multiply the trilinear, receiver-facing, and visibility weights; normalize the final sum.

Make the visibility atlas viewable in the texture inspector and add a debug rendering mode for its first and second moments. Most visibility bugs are immediately obvious there.

### Milestone E — probe relocation and classification

**Goal:** a probe placed inside geometry does not contaminate nearby shading.

Extend `DDGIProbeData` beyond a position with an offset, state/classification, and perhaps a relocation/reset flag. From ray hits, track front/back-face information and nearby distances. A separate relocation pass can choose a bounded offset into free space; classification can deactivate probes that remain invalid. This is a quality improvement after the previous milestones, not a prerequisite for seeing DDGI.

## 5. Border handling

The border shader is registered in `PipelineDB`, but no C++ pass binds or dispatches it. Add it only after the inner irradiance tile is correct.

Each probe tile needs a one-pixel border whose values are copied from the octahedrally wrapped interior. Perform this after temporal blending, for both irradiance and visibility moments. Then sample using texel-centre coordinates that target the interior plus its border; plain atlas UVs over the entire tile are not sufficient because bilinear filtering can bleed into adjacent probe tiles.

Useful border tests:

- A probe with a deliberately asymmetric directional color should have no seam across the octahedral fold.
- The color must not change to a neighboring probe's color at an atlas-tile edge.
- Repeat for distance moments; a broken visibility border produces directional light leaks.

## 6. Recommended debug views and tests

Build observation tools alongside each stage. They will save much more time than trying to infer problems from the final image.

| Debug view/test | It should answer |
|---|---|
| Selected probe: draw every ray direction and hit/miss | Are directions uniformly covering the sphere and are closest hits correct? |
| Raw ray radiance tile | Are misses defined and do moving lights affect the correct directions? |
| Raw distance and distance-squared tiles | Are values finite, bounded, and coherent with visible walls? |
| Filtered irradiance sphere | Is the cosine convolution directional but smooth? |
| History age/reset indicator | Did a settings/scene change invalidate the correct probes? |
| Visibility weight heat map in deferred lighting | Does a wall suppress only probes behind it? |
| Cornell-box-style scene | Does indirect color bleed occur and does it disappear behind an occluder? |
| Empty scene / constant environment | Does every probe converge to the same result with no seams? |

For deterministic debugging, first use fixed ray directions and a fixed frame seed. Enable temporal jitter/rotation only after a capture is repeatable. The existing `PcgHash` takes its state as `inout`, so the two current calls do advance the seed; the issue is not identical random values. The current white-noise texture is fetched but unused, however, and one ray per probe is still far too sparse.

## 7. Suggested implementation checklist

1. Complete ray-query traversal; add bounds checks; ensure hit and miss paths write defined output.
2. Change dispatch/layout so every interior ray texel is written exactly once per probe; validate with debug rays.
3. Store or compute lighting at the ray hit position, with a probe-appropriate outgoing direction.
4. Produce a raw radiance ray map and implement cosine convolution into an irradiance map.
5. Implement and dispatch octahedral border copies; display a seam-free probe sphere.
6. Correct eight-probe reconstruction: volume bounds, direction sign, weight normalization, and receiving diffuse BRDF.
7. Introduce ping-pong irradiance history using the TAA resource-selection pattern; add invalidation.
8. Add filtered/temporally accumulated distance moments and Chebyshev visibility weighting.
9. Add probe relocation/classification, then optimize ray budgets and partial updates.

## 8. References worth reading alongside the implementation

- Morgan McGuire et al., *Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields*, JCGT 2019: the core DDGI paper and the best source for probe update, visibility, relocation, and classification concepts. <https://jcgt.org/published/0008/02/01/>
- NVIDIA RTXGI: an open-source production-oriented reference. Read its probe update and border logic only after you can explain your own data contract. <https://github.com/NVIDIAGameWorks/RTXGI>
- Cigolle et al., *A Survey of Efficient Representations for Independent Unit Vectors* (2014): background on octahedral mapping and why borders are necessary for filtered atlas sampling.

The intended progression is: first prove dense, correct local radiance; then prove correct irradiance and diffuse reconstruction; then use history to buy quality; finally use visibility and relocation to control leakage. That sequence keeps each new DDGI feature solving a specific, observable limitation.
