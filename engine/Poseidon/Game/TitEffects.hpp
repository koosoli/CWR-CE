#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Types/EnumDecl.hpp>
#include <Poseidon/World/World.hpp>
namespace Poseidon { class ParamEntry; }
namespace Poseidon { class Font; }

namespace Poseidon
{
} // namespace Poseidon


DEFINE_ENUM_BEG(TitEffectName)
	TitPlain,TitPlainDown,
	TitBlack,TitBlackFaded,
	TitBlackOut,TitBlackIn,
	TitWhiteOut,TitWhiteIn,

	NTitEffects
DEFINE_ENUM_END(TitEffectName)

namespace Poseidon { class TitleEffect; }
using Poseidon::TitleEffect;

TitleEffect *CreateTitleEffect( TitEffectName name, RString text, float speed=1, Ref<Font> font = nullptr, float size = 0 );
TitleEffect *CreateTitleEffectObj
(
	TitEffectName name, const ParamEntry &entry, float speed=1
);
TitleEffect *CreateTitleEffectRsc
(
	TitEffectName name, const ParamEntry &entry, float speed=1
);

// Split on-screen title/subtitle text at the literal two-byte marker "\n"
// (0x5C 0x6E) into one entry per line. A raw newline byte (0x0A) is NOT a
// separator -- it stays in the line and is drawn as a glyph. This is the split
// cutscene / in-game subtitles run (TitleEffectBasic::Init, fed by DynSound
// from CfgSounds titles[]) and that titleText / cutText use.
void SplitTitleLines( RString text, AutoArray<RString> &out );

// Drop-shadow offset (normalized screen units) for on-screen subtitle/title
// text, given the viewport pixel dimensions. The horizontal component is
// scaled by the aspect ratio so the shadow drops an equal number of pixels in
// X and Y: offsetX*width2D == offsetY*height2D. A flat normalized offset smears
// the shadow ~1.8x further sideways than down on a 16:9 display, doubling each
// glyph and hurting readability over the map. Kept tight (~2px @ 1080p) so it
// reads as an edge shadow, not a second copy of the text.
struct TitleShadowOffset { float x; float y; };
TitleShadowOffset SubtitleShadowOffset( int width2D, int height2D );

