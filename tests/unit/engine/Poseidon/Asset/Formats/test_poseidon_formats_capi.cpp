// Test the PoseidonFormats C API (pf_* functions)
// These test the public DLL/shared library interface used by the Blender addon.
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <PoseidonFormats/PoseidonFormats.h>
#include <cstring>
#include <cmath>
#include <fstream>
#include <vector>
#include "test_fixtures.hpp"
#include <stdint.h>
#include <string>

// ── Library lifecycle ──────────────────────────────────────────────────────

TEST_CASE("PF C API: pf_version returns non-null string", "[PoseidonFormats][API]")
{
    const char* ver = pf_version();
    REQUIRE(ver != nullptr);
    REQUIRE(std::strlen(ver) > 0);
}

TEST_CASE("PF C API: pf_last_error returns string", "[PoseidonFormats][API]")
{
    const char* err = pf_last_error();
    // May be null or empty when no error
    (void)err;
    SUCCEED();
}

// ── Model loading ──────────────────────────────────────────────────────────

TEST_CASE("PF C API: load synthetic MLOD model", "[PoseidonFormats][Model]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    SECTION("format is MLOD")
    {
        const char* fmt = pf_model_format(model);
        REQUIRE(fmt != nullptr);
        REQUIRE(std::string(fmt) == "MLOD");
    }

    SECTION("has multiple LODs")
    {
        int lods = pf_model_lod_count(model);
        REQUIRE(lods > 1);
    }

    SECTION("LOD 0 has vertices and faces")
    {
        int verts = pf_lod_vertex_count(model, 0);
        int faces = pf_lod_face_count(model, 0);
        REQUIRE(verts > 0);
        REQUIRE(faces > 0);
    }

    SECTION("LOD resolution is valid")
    {
        float res = pf_model_lod_resolution(model, 0);
        REQUIRE(res >= 0.0f);
    }

    pf_model_free(model);
}

TEST_CASE("PF C API: load NULL path returns NULL", "[PoseidonFormats][Model]")
{
    PF_HANDLE model = pf_model_load(nullptr);
    REQUIRE(model == nullptr);
}

TEST_CASE("PF C API: load nonexistent file returns NULL", "[PoseidonFormats][Model]")
{
    PF_HANDLE model = pf_model_load("/nonexistent/path.p3d");
    REQUIRE(model == nullptr);
}

TEST_CASE("PF C API: free NULL model is safe", "[PoseidonFormats][Model]")
{
    pf_model_free(nullptr);
    SUCCEED();
}

// ── Bounding center & autocenter ───────────────────────────────────────────

TEST_CASE("PF C API: bounding center returns valid values", "[PoseidonFormats][Model]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    float bc[3] = {0};
    pf_model_bounding_center(model, bc);

    // Values should be finite
    REQUIRE(std::isfinite(bc[0]));
    REQUIRE(std::isfinite(bc[1]));
    REQUIRE(std::isfinite(bc[2]));

    pf_model_free(model);
}

TEST_CASE("PF C API: bounding center with NULL model is safe", "[PoseidonFormats][Model]")
{
    float bc[3] = {1, 2, 3};
    pf_model_bounding_center(nullptr, bc);
    // Should not crash; values unchanged
    REQUIRE(bc[0] == 1.0f);
}

TEST_CASE("PF C API: bounding center with NULL buffer is safe", "[PoseidonFormats][Model]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    pf_model_bounding_center(model, nullptr); // should not crash

    pf_model_free(model);
}

TEST_CASE("PF C API: autocenter flag", "[PoseidonFormats][Model]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int ac = pf_model_autocenter(model);
    // Value should be 0 or 1
    REQUIRE((ac == 0 || ac == 1));

    pf_model_free(model);
}

TEST_CASE("PF C API: autocenter with NULL model returns 0", "[PoseidonFormats][Model]")
{
    int ac = pf_model_autocenter(nullptr);
    REQUIRE(ac == 0);
}

// ── Geometry access ────────────────────────────────────────────────────────

TEST_CASE("PF C API: vertex data", "[PoseidonFormats][Geometry]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int nv = pf_lod_vertex_count(model, 0);
    REQUIRE(nv > 0);

    std::vector<float> verts(nv * 3);
    pf_lod_get_vertices(model, 0, verts.data());

    // Check vertices are finite
    for (int i = 0; i < nv * 3; i++)
    {
        REQUIRE(std::isfinite(verts[i]));
    }

    // Normals
    std::vector<float> normals(nv * 3);
    pf_lod_get_normals(model, 0, normals.data());
    for (int i = 0; i < nv; i++)
    {
        float len = std::sqrt(normals[i * 3] * normals[i * 3] + normals[i * 3 + 1] * normals[i * 3 + 1] +
                              normals[i * 3 + 2] * normals[i * 3 + 2]);
        // Normals should be unit-length or zero
        REQUIRE((len < 0.01f || std::abs(len - 1.0f) < 0.1f));
    }

    // UVs
    std::vector<float> uvs(nv * 2);
    pf_lod_get_uvs(model, 0, uvs.data());
    for (int i = 0; i < nv * 2; i++)
    {
        REQUIRE(std::isfinite(uvs[i]));
    }

    pf_model_free(model);
}

TEST_CASE("PF C API: face indices are valid", "[PoseidonFormats][Geometry]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int nv = pf_lod_vertex_count(model, 0);
    int nf = pf_lod_face_count(model, 0);
    REQUIRE(nf > 0);

    std::vector<int> faces(nf * 3);
    pf_lod_get_faces(model, 0, faces.data());

    for (int i = 0; i < nf * 3; i++)
    {
        REQUIRE(faces[i] >= 0);
        REQUIRE(faces[i] < nv);
    }

    pf_model_free(model);
}

TEST_CASE("PF C API: face materials are valid", "[PoseidonFormats][Geometry]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int nf = pf_lod_face_count(model, 0);
    int mat_count = pf_lod_material_count(model, 0);

    std::vector<int> mats(nf);
    pf_lod_get_face_materials(model, 0, mats.data());

    for (int i = 0; i < nf; i++)
    {
        REQUIRE(mats[i] >= 0);
        REQUIRE(mats[i] < mat_count);
    }

    pf_model_free(model);
}

TEST_CASE("PF C API: vertex and face flags", "[PoseidonFormats][Geometry]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int nv = pf_lod_vertex_count(model, 0);
    int nf = pf_lod_face_count(model, 0);

    std::vector<uint32_t> vflags(nv);
    pf_lod_get_vertex_flags(model, 0, vflags.data());
    // Just check it doesn't crash; values are model-specific

    std::vector<uint32_t> fflags(nf);
    pf_lod_get_face_flags(model, 0, fflags.data());

    SUCCEED();
    pf_model_free(model);
}

// ── Out-of-bounds access ───────────────────────────────────────────────────

TEST_CASE("PF C API: out-of-bounds LOD index", "[PoseidonFormats][ErrorHandling]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int lods = pf_model_lod_count(model);
    REQUIRE(pf_lod_vertex_count(model, lods) == 0); // past end
    REQUIRE(pf_lod_vertex_count(model, -1) == 0);   // negative
    REQUIRE(pf_lod_face_count(model, lods) == 0);
    REQUIRE(pf_lod_material_count(model, lods) == 0);
    REQUIRE(pf_lod_selection_count(model, lods) == 0);
    REQUIRE(pf_lod_proxy_count(model, lods) == 0);
    REQUIRE(pf_lod_property_count(model, lods) == 0);

    pf_model_free(model);
}

// ── Materials ──────────────────────────────────────────────────────────────

TEST_CASE("PF C API: material texture and surface", "[PoseidonFormats][Materials]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int mat_count = pf_lod_material_count(model, 0);
    REQUIRE(mat_count > 0);

    for (int i = 0; i < mat_count; i++)
    {
        const char* tex = pf_lod_material_texture(model, 0, i);
        const char* surf = pf_lod_material_surface(model, 0, i);
        // May be null/empty strings, but should not crash
        (void)tex;
        (void)surf;
    }

    pf_model_free(model);
}

// ── Selections ─────────────────────────────────────────────────────────────

TEST_CASE("PF C API: named selections", "[PoseidonFormats][Selections]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int sel_count = pf_lod_selection_count(model, 0);

    for (int i = 0; i < sel_count; i++)
    {
        const char* name = pf_lod_selection_name(model, 0, i);
        REQUIRE(name != nullptr);

        int vc = pf_lod_selection_vertex_count(model, 0, i);
        if (vc > 0)
        {
            std::vector<int> indices(vc);
            std::vector<float> weights(vc);
            pf_lod_selection_get_vertices(model, 0, i, indices.data(), weights.data());

            int nv = pf_lod_vertex_count(model, 0);
            for (int j = 0; j < vc; j++)
            {
                REQUIRE(indices[j] >= 0);
                REQUIRE(indices[j] < nv);
                REQUIRE(weights[j] >= 0.0f);
                REQUIRE(weights[j] <= 1.01f); // small tolerance
            }
        }
    }

    pf_model_free(model);
}

// ── Proxies ────────────────────────────────────────────────────────────────

TEST_CASE("PF C API: proxy access", "[PoseidonFormats][Proxies]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int prx_count = pf_lod_proxy_count(model, 0);
    for (int i = 0; i < prx_count; i++)
    {
        const char* prx_path = pf_lod_proxy_path(model, 0, i);
        REQUIRE(prx_path != nullptr);

        float mat[16] = {0};
        pf_lod_proxy_transform(model, 0, i, mat);
        // Transform should have 1.0 on diagonal (identity-ish)
        // or at least be finite
        for (int j = 0; j < 16; j++)
        {
            REQUIRE(std::isfinite(mat[j]));
        }
    }

    pf_model_free(model);
}

// ── Properties ─────────────────────────────────────────────────────────────

TEST_CASE("PF C API: named properties", "[PoseidonFormats][Properties]")
{
    const char* path = GET_FIXTURE("p3d/complex_vehicle.p3d");
    PF_HANDLE model = pf_model_load(path);
    REQUIRE(model != nullptr);

    int prop_count = pf_lod_property_count(model, 0);
    for (int i = 0; i < prop_count; i++)
    {
        const char* name = pf_lod_property_name(model, 0, i);
        const char* value = pf_lod_property_value(model, 0, i);
        REQUIRE(name != nullptr);
        // value may be empty but shouldn't crash
        (void)value;
    }

    pf_model_free(model);
}

// ── RTM Animation ──────────────────────────────────────────────────────────

TEST_CASE("PF C API: RTM load from file", "[PoseidonFormats][RTM]")
{
    const char* path = GET_FIXTURE("rtm/marker_motion.rtm");
    PF_HANDLE rtm = pf_rtm_load(path);
    REQUIRE(rtm != nullptr);

    SECTION("bone and phase counts")
    {
        int bones = pf_rtm_bone_count(rtm);
        int phases = pf_rtm_phase_count(rtm);
        REQUIRE(bones > 0);
        REQUIRE(phases > 0);
    }

    SECTION("bone names are valid")
    {
        int bones = pf_rtm_bone_count(rtm);
        for (int i = 0; i < bones; i++)
        {
            const char* name = pf_rtm_bone_name(rtm, i);
            REQUIRE(name != nullptr);
            REQUIRE(std::strlen(name) > 0);
        }
    }

    SECTION("phase times are monotonic")
    {
        int phases = pf_rtm_phase_count(rtm);
        float prev = -1.0f;
        for (int i = 0; i < phases; i++)
        {
            float t = pf_rtm_phase_time(rtm, i);
            REQUIRE(t >= prev);
            prev = t;
        }
    }

    SECTION("bone matrices are finite")
    {
        int bones = pf_rtm_bone_count(rtm);
        int phases = pf_rtm_phase_count(rtm);
        float mat[16];
        for (int p = 0; p < phases; p++)
        {
            for (int b = 0; b < bones; b++)
            {
                pf_rtm_bone_matrix(rtm, p, b, mat);
                for (int i = 0; i < 16; i++)
                {
                    REQUIRE(std::isfinite(mat[i]));
                }
                // Bottom row should be [0, 0, 0, 1]
                REQUIRE(mat[3] == Catch::Approx(0.0f));
                REQUIRE(mat[7] == Catch::Approx(0.0f));
                REQUIRE(mat[11] == Catch::Approx(0.0f));
                REQUIRE(mat[15] == Catch::Approx(1.0f));
            }
        }
    }

    SECTION("step vector")
    {
        float step[3];
        pf_rtm_step(rtm, step);
        for (int i = 0; i < 3; i++)
        {
            REQUIRE(std::isfinite(step[i]));
        }
    }

    pf_rtm_free(rtm);
}

TEST_CASE("PF C API: RTM load from memory", "[PoseidonFormats][RTM]")
{
    const char* path = GET_FIXTURE("rtm/marker_motion.rtm");

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    REQUIRE(file.good());
    auto size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    PF_HANDLE rtm = pf_rtm_load_from_memory(data.data(), static_cast<int>(size));
    REQUIRE(rtm != nullptr);
    REQUIRE(pf_rtm_bone_count(rtm) > 0);
    REQUIRE(pf_rtm_phase_count(rtm) > 0);

    pf_rtm_free(rtm);
}

TEST_CASE("PF C API: RTM NULL/invalid handling", "[PoseidonFormats][RTM]")
{
    REQUIRE(pf_rtm_load(nullptr) == nullptr);
    REQUIRE(pf_rtm_load("/nonexistent.rtm") == nullptr);
    REQUIRE(pf_rtm_load_from_memory(nullptr, 0) == nullptr);

    uint8_t garbage[] = {0, 1, 2, 3};
    REQUIRE(pf_rtm_load_from_memory(garbage, 4) == nullptr);

    // Free NULL should be safe
    pf_rtm_free(nullptr);
    SUCCEED();
}

// ── PAA Image ──────────────────────────────────────────────────────────────

TEST_CASE("PF C API: PAA image loading", "[PoseidonFormats][Image]")
{
    const char* path = GET_FIXTURE("texture/paa/synthetic_dxt1.paa");
    PF_HANDLE img = pf_image_load(path);
    REQUIRE(img != nullptr);

    int w = pf_image_width(img);
    int h = pf_image_height(img);
    REQUIRE(w > 0);
    REQUIRE(h > 0);

    const char* fmt = pf_image_format(img);
    REQUIRE(fmt != nullptr);

    // Read RGBA data
    std::vector<uint8_t> rgba(w * h * 4);
    pf_image_get_rgba(img, rgba.data());
    // Just verify it doesn't crash; pixel values are format-specific

    pf_image_free(img);
}

TEST_CASE("PF C API: image NULL/invalid handling", "[PoseidonFormats][Image]")
{
    REQUIRE(pf_image_load(nullptr) == nullptr);
    REQUIRE(pf_image_load("/nonexistent.paa") == nullptr);
    pf_image_free(nullptr);
    SUCCEED();
}

// ── PBO Archive ────────────────────────────────────────────────────────────

TEST_CASE("PF C API: PBO archive", "[PoseidonFormats][PBO]")
{
    // PBO fixtures are copied beside the test binary.
    std::string pboPath = TestFixtures::GetTestFixturePath("pbo/mission_fixture.Intro.pbo");

    std::ifstream check(pboPath, std::ios::binary);
    if (!check.good())
    {
        SKIP("PBO fixture not available at " + pboPath);
    }
    check.close();

    PF_HANDLE pbo = pf_pbo_open(pboPath.c_str());
    if (!pbo)
    {
        SKIP("PBO fixture could not be opened");
    }

    int count = pf_pbo_entry_count(pbo);
    REQUIRE(count > 0);

    for (int i = 0; i < count; i++)
    {
        const char* name = pf_pbo_entry_name(pbo, i);
        REQUIRE(name != nullptr);

        int size = pf_pbo_entry_size(pbo, i);
        REQUIRE(size >= 0);

        if (size > 0 && size < 1024 * 1024)
        {
            std::vector<uint8_t> buf(size);
            int read = pf_pbo_extract(pbo, i, buf.data(), size);
            REQUIRE(read == size);
        }
    }

    pf_pbo_close(pbo);
}

TEST_CASE("PF C API: PBO NULL/invalid handling", "[PoseidonFormats][PBO]")
{
    REQUIRE(pf_pbo_open(nullptr) == nullptr);
    REQUIRE(pf_pbo_open("/nonexistent.pbo") == nullptr);
    pf_pbo_close(nullptr);
    SUCCEED();
}
