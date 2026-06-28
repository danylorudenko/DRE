---
name: create-shader-file
description: Create a new shader in the DRE rendering engine
---

To create a new shader in the DRE rendering engine, follow these steps:

0. if any doubts, check out the existing shaders in shaders/
1. Create a new shader file in shaders/ with the name of the shader and the appropriate extension (.slang for slang shaders)
2. If it will be a reusable shader file, add #define guards, everything else goes inside guards
3. add #include "common/shaders_defines.slang" at the beginning of the file
4. add new shader file to src/gfx/CMakeLists.txt