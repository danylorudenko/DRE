---
name: create-rendering-pass
description: Create a rendering pass in the DRE render graph
---

To create a completely new pass in the DRE rendering engine, follow these steps:

0. if any doubts, check out the existing passes in src/gfx/pass/ and include/gfx/pass/ for reference
1. If there's no file provided by user, create new .cpp file in src/gfx/pass/ with the name of the pass. Add accompanying header file in include/gfx/pass/ with the same name.
2. In file /include/gfx/pass/PassID.hpp create new enum entry for the pass
3. Create a new class for the pass, inheriting from `BasePass`. Create overrides for required virtual methods (GetID, RegisterResources, Initialize, Render)
3.1 GetID should return the enum entry created in step 2
3.2 RegisterResources should register all resources used by the pass (textures, buffers, render targets)
3.3 Initialize is usually empty
3.4 Render fetch all registered resources for the pass and do transitions via g_GraphicsManager->GetDependencyManager().ResourceBarrier(...)
4. Add pass to the render graph in GraphicsManager::CreateAllPasses
5. add new files to src/gfx/CMakeLists.txt