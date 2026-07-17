# Renderer Benchmark Notes

## Anotherfork WGPU Reference

The archived optimized renderer executable is:

`D:\SteamLibrary\steamapps\common\Arma Cold War Assault Demo\PoseidonGameDemo-ofpisnotdead.exe`

It defaults to GL33. Use the WGPU renderer explicitly for performance comparisons:

```powershell
.\PoseidonGameDemo-ofpisnotdead.exe --render wgpu
```

Keep resolution, mission, camera path, view distance, and visual settings identical to the current Vulkan run. Do not compare menu or placeholder-mission FPS to a gameplay scene.

Verified 2026-07-17: the executable identifies itself as `3.01-835e8ba-demo` and reports a WGPU-over-Vulkan renderer with 4x MSAA, HDR, and GPU-driven indirect draws. Its matching WGPU source is not present in either checked-in reference tree.
