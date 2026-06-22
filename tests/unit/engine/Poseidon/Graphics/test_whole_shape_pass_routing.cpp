#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

// Whole-shape (non-T&L) pass routing — live decals over roads.
//
// Shapes with no vertex buffer (the live tyre-track ribbon, rebuilt every
// frame while the vehicle drives) can't section-split, so Shape::Draw's
// non-T&L fallback draws them whole, exactly once across the two filtered
// passes.  `DrawWholeShapeInPass` routes that single draw: a surface overlay
// must paint in the BlendOnly on-surface pass (sorted roads-first, decals
// over them); painting it in the opaque pass instead puts the whole live
// trail UNDER the road's asphalt, which is drawn later in the frame.
//
// Without the fix the fallback drew every whole shape in the opaque pass
// (`GSectionFilter != BlendOnly`), and a driven jeep's tracks vanished on
// roads: RenderDoc shows the ribbon drawn early and the road's blend section
// repainting over it.

using Poseidon::DrawWholeShapeInPass;
using Poseidon::SectionClassFilter;

TEST_CASE("DrawWholeShapeInPass: surface overlays paint in the on-surface pass", "[Graphics][Shape][surface]")
{
    // Unfiltered draws always paint.
    REQUIRE(DrawWholeShapeInPass(SectionClassFilter::All, /*surfaceOverlay=*/false));
    REQUIRE(DrawWholeShapeInPass(SectionClassFilter::All, /*surfaceOverlay=*/true));

    // Ordinary whole-shape draws stay in the opaque pass.
    REQUIRE(DrawWholeShapeInPass(SectionClassFilter::OpaqueAndCutout, false));
    REQUIRE_FALSE(DrawWholeShapeInPass(SectionClassFilter::BlendOnly, false));

    // Surface overlays (live tyre tracks, marks) flip: skipped in the opaque
    // pass, painted in the BlendOnly on-surface pass — after the road tiles
    // the pass draws first.  This is the broken-state delta: before the fix
    // the overlay drew in the opaque pass and the road covered it.
    REQUIRE_FALSE(DrawWholeShapeInPass(SectionClassFilter::OpaqueAndCutout, true));
    REQUIRE(DrawWholeShapeInPass(SectionClassFilter::BlendOnly, true));

    // Exactly one pass paints each shape — never zero, never both.
    for (bool overlay : {false, true})
    {
        const int paints = (DrawWholeShapeInPass(SectionClassFilter::OpaqueAndCutout, overlay) ? 1 : 0) +
                           (DrawWholeShapeInPass(SectionClassFilter::BlendOnly, overlay) ? 1 : 0);
        REQUIRE(paints == 1);
    }
}

namespace
{

std::string ReadStripped(const std::filesystem::path& p)
{
    std::ifstream f(p);
    REQUIRE(f.is_open());
    std::stringstream ss;
    ss << f.rdbuf();
    return std::regex_replace(ss.str(), std::regex("//[^\n]*"), "");
}

} // namespace

TEST_CASE("Shape::Draw non-T&L fallback routes through DrawWholeShapeInPass", "[Graphics][Shape][surface][boundary]")
{
    // The truth table above has teeth only while ShapeDraw.cpp actually
    // consults it.  Pin the wiring: the fallback must call
    // DrawWholeShapeInPass with a surface-overlay predicate, and the old
    // unconditional `GSectionFilter != ...BlendOnly` gate must not return.
    const auto shapeDraw = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "Poseidon" / "Graphics" /
                           "Rendering" / "Shape" / "ShapeDraw.cpp";
    const std::string body = ReadStripped(shapeDraw);

    REQUIRE(body.find("DrawWholeShapeInPass(GSectionFilter, surfaceOverlay)") != std::string::npos);
    REQUIRE(body.find("GSectionFilter != SectionClassFilter::BlendOnly") == std::string::npos);
}
