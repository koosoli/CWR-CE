// LODShape/Shape pull AutoArray / Vector3 / Matrix4 / RString in via the engine PCH;
// the harness gets no PCH, so include the concrete headers explicitly.
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

#include "fuzz_init.hpp"

#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

#include <cstddef>
#include <cstdint>

// libFuzzer harness for the runtime P3D shape loader (LODShape -> Shape::LoadTagged).
//
// This is the renderer's model loader, distinct from the headless Asset/Formats P3D
// reader fuzz_p3d covers: it reads the MLOD/NLOD container, then per-LOD SP3D/SP3X
// sections (vertices, faces, then the TAGG #Property#/#Selection#/#Mass#/... blocks
// where the runtime-loader name-buffer bugs live). Models ride inside untrusted
// missions / mods. NoTextures makes GlobLoadTexture a no-op so the face/material path
// needs no graphics device — the parse runs headless. A thrown parse error is handled;
// only a memory-safety fault reaches ASan.

using namespace Poseidon;

namespace
{
// Application is abstract; the loader only needs GApp valid for ENGINE_CONFIG, so all
// the lifecycle hooks are no-ops (never called here).
class FuzzApp : public Application
{
  public:
    FuzzApp() : Application(DEDICATED_SERVER) {}
    int Run(const char*) override { return 0; }

  protected:
    bool InitializeMemorySystem() override { return true; }
    bool ParseCommandLine(const char*) override { return true; }
    bool ReadConfiguration() override { return true; }
    bool InitializeSubsystems() override { return true; }
    void RunMainLoop() override {}
    void ShutdownSubsystems() override {}
};
} // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    NoTextures = true;
    // The runtime loader reads ENGINE_CONFIG (GApp->GetConfig()) during material setup;
    // construct a headless Application so GApp is valid (gMergeTextures defaults false, so
    // the merge path stays off and needs no graphics).
    static FuzzApp app;
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        QIStream f(const_cast<char*>(reinterpret_cast<const char*>(data)), static_cast<int>(size));
        LODShape lod(f, false);
    }
    catch (...)
    {
    }
    return 0;
}
