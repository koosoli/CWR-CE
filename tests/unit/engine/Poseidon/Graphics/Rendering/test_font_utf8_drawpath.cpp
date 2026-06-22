// Regression: Engine::DrawText must walk UTF-8 codepoints, not bytes.
//
// The .fxy bitmap atlas path at fontDraw.cpp:86 walks
// `for (c = *text++; ...) c &= 0xff` and indexes font->_infos[c - 32],
// so any UTF-8 multibyte sequence (Czech ř = 0xC5 0x99, Russian
// Cyrillic, …) draws TWO glyphs per codepoint from whatever happens to
// live at those upper Win1250 atlas slots.  Visible in the field as
// "double characters render over the one" — progress bar, briefing
// pane, MP lobby.
//
// The FreeType path (Font::IsFreeType() == true) decodes codepoints
// properly, so the regression boils down to "any Font that the engine
// creates must end up in FreeType mode whenever a slot mapping + TTF
// exists, regardless of when relative to FontSystem::Initialize()".
//
// Earlier, Font::Load gated FreeType eligibility on
// FontSystem::Instance().IsAvailable() — fonts loaded during
// InitializeWorld (e.g. ProgressSystem) were born bitmap because
// InitializeSubsystems (where FontSystem::Initialize ran) came later
// in GameApplication::Run.  The durable fix decouples Font::Load from
// FontSystem init order — eligibility now depends only on whether a
// slot mapping exists and the TTF loads.  These tests pin that
// contract so the coupling can't sneak back in.

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Graphics/Rendering/Draw/FontSystem.hpp>
#include <Poseidon/Graphics/Rendering/Draw/Font.hpp>

#include <filesystem>
#include <string>
#include <Poseidon/Foundation/Types/Pointers.hpp>

using Poseidon::FontSystem;

namespace
{

class ScopedCwd
{
  public:
    explicit ScopedCwd(const std::filesystem::path& dir) : _prev(std::filesystem::current_path())
    {
        std::filesystem::create_directories(dir);
        std::filesystem::current_path(dir);
    }
    ~ScopedCwd() { std::filesystem::current_path(_prev); }

  private:
    std::filesystem::path _prev;
};

} // namespace

TEST_CASE("Font::Load yields an empty Font when no asset is available", "[font][utf8][regression]")
{
    // Tetris-style boot: no slot-0 TTFs on disk, no .fxy either,
    // FontSystem off.  The Font must end up empty — neither FreeType
    // nor bitmap.  DrawText then short-circuits at the top because
    // `font->Height() <= 0` / `font->_nChars <= 0`, so the bitmap
    // byte-walk at fontDraw.cpp:86 is never reached.
    auto tmp = std::filesystem::temp_directory_path() / "ofpr-font-utf8-empty";
    std::filesystem::remove_all(tmp);
    ScopedCwd cwd(tmp);

    FontSystem::Instance().Shutdown();

    Ref<Font> font = new Font();
    font->Load("cwrBodyB24");

    CHECK_FALSE(font->IsFreeType());
    CHECK(font->FTRenderer() == nullptr);
    CHECK(font->MaxHeight() == 0);
    CHECK(font->Height() == 0.0f);
}
