#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TXT_PRELOAD_HPP
#define _TXT_PRELOAD_HPP

#include <Poseidon/Foundation/Types/EnumDecl.hpp>

namespace Poseidon
{
DECL_ENUM(PreloadedTexture)

DEFINE_ENUM_BEG(PreloadedTexture)
TextureWhite, TextureBlack, TextureDefault, TextureLine,

    CursorStrategy, CursorStrategyAttack, CursorStrategyMove, CursorStrategySelect, CursorStrategyGetIn,
    CursorStrategyWatch,

    CursorAim,    // we want to aim somewhere
    CursorWeapon, // current weapon aim
    CursorTarget, // selected target
    CursorLocked, // target locked

    CursorOutArrow, // cursor out of screen

    Corner, DialogBackground, DialogTitle, DialogGroup, TrackTexture, TrackTextureFour,

    SignSideE, SignSideW, SignSideG, FlagSideE, FlagSideW, FlagSideG,

    Compass000, Compass090, Compass180, Compass270,

    /*
        SignUnit0,SignUnitLast=SignUnit0+9,
        SignGroup0,SignGroupLast=SignGroup0+9,
        SignGroupC0,SignGroupCLast=SignGroupC0+9,
    */
    Flare0, FlareLast = Flare0 + 15,

            SkyBright, SkyCloudy, SkyRainy,
            TextureRain, // animated rain texture

    MaxPreloadedTexture DEFINE_ENUM_END(PreloadedTexture)

} // namespace Poseidon
#endif
