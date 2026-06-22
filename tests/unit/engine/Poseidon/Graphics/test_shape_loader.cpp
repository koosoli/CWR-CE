#include <catch2/catch_test_macros.hpp>
#include "test_fixtures.hpp"

#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

#include <fstream>
#include <iterator>
#include <vector>

using namespace Poseidon;

// fuzz_shape reproducer. The runtime P3D model loader's #Property#
// tag read 64-byte name/value fields into char[64] with no terminator, then strlwr / the
// NamedProperty ctor ran strlen off the end of the stack when all 64 bytes were non-NUL —
// a stack-buffer-overflow read. The +1 NUL-guard buffers now bound it. ASan reports the
// over-read pre-fix; the load completing (parse or thrown error) is the regression's teeth.
//
// GApp is provided by the test main (TestApplication); NoTextures keeps the material path
// graphics-free so the parse runs in the unit-test process.
TEST_CASE("Shape: fuzz reproducer parses #Property# names without stack OOB", "[Graphics][Shape][fuzz]")
{
    NoTextures = true;

    std::ifstream fs(GET_FIXTURE("mlod/fuzz_tagg_oob.p3d"), std::ios::binary);
    REQUIRE(fs.good());
    std::vector<char> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    REQUIRE(!data.empty());

    QIStream f(data.data(), static_cast<int>(data.size()));
    try
    {
        LODShape lod(f, false);
    }
    catch (...)
    {
    }
    SUCCEED("LODShape parsed the reproducer without an out-of-bounds read");
}

// fuzz_shape reproducer for the optimized-binary (Shape::SerializeBin) path. The binary
// shape format read texture / face / section counts and texture / vertex indices off the
// wire with no bound — driving huge/negative Realloc, OOB _textures[]/_vertex[] access, and
// a memcpy-with-bad-size in Poly. The count/index guards now reject malformed values. ASan
// reports the OOB pre-fix; the load completing is the teeth.
TEST_CASE("Shape: fuzz reproducer loads optimized binary without OOB", "[Graphics][Shape][fuzz]")
{
    NoTextures = true;

    std::ifstream fs(GET_FIXTURE("mlod/fuzz_shape_serializebin_oob.p3d"), std::ios::binary);
    REQUIRE(fs.good());
    std::vector<char> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    REQUIRE(!data.empty());

    QIStream f(data.data(), static_cast<int>(data.size()));
    try
    {
        LODShape lod(f, false);
    }
    catch (...)
    {
    }
    SUCCEED("LODShape parsed the optimized-binary reproducer without an out-of-bounds access");
}

// fuzz_shape reproducer for the LOD-count / SP3D face-count guards: _nLods (a fixed
// _lods[MAX_LOD_LEVELS] array) and a face's vertex count (srcFace.vs[MAX_DATA_POLY], the
// poly's fixed _vertex buffer) were both used unbounded off the wire — OOB writes. The
// guards reject out-of-range counts; ASan reports the OOB pre-fix, replays clean post-fix.
TEST_CASE("Shape: fuzz reproducer bounds LOD and face counts", "[Graphics][Shape][fuzz]")
{
    NoTextures = true;

    std::ifstream fs(GET_FIXTURE("mlod/fuzz_shape_lod_oob.p3d"), std::ios::binary);
    REQUIRE(fs.good());
    std::vector<char> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    REQUIRE(!data.empty());

    QIStream f(data.data(), static_cast<int>(data.size()));
    try
    {
        LODShape lod(f, false);
    }
    catch (...)
    {
    }
    SUCCEED("LODShape parsed the reproducer without an out-of-bounds access");
}
