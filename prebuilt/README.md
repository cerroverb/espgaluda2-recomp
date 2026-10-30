# Prebuilt host shaders

The renderer's own shaders (`src/renut_engine/gpu/shaders/host/*.hlsl`, not the game's), compiled to SPIR-V by DXC
on an x86-64 build (`renut_gpu/host_shaders/*.spirv.h`), for the Linux AArch64 build on GitHub's ARM runner, which has
no DXC (`.github/workflows/build-linux-arm64.yml`, `ESPGALUDA2_PREBUILT_GPU_DIR`). SPIR-V is the same on every CPU.
Rebuild and copy them again after changing a host shader.
