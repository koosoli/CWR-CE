// I-31 / B-027 — vertex color byte order, real pixel readback.
//
// `triAssertColorbarBytes` draws the colorbar test pattern and
// reads back the centre pixel of each of the 5 bars in a single
// synchronous call — no buffer-swap race.  Expected RGB per bar
// matches `EngineGL33::DrawTestPattern("colorbar")`'s PackedColor
// values:
//
//   bar 0 — 255,   0,   0   (red)     0xFFFF0000
//   bar 1 —   0, 255,   0   (green)   0xFF00FF00
//   bar 2 —   0,   0, 255   (blue)    0xFF0000FF
//   bar 3 — 255, 255,   0   (yellow)  0xFFFFFF00
//   bar 4 — 255,   0, 255   (magenta) 0xFFFF00FF
//
// PackedColor stores `(a<<24)|(r<<16)|(g<<8)|b` as a DWORD — on
// little-endian that's bytes B,G,R,A in memory.  The TLVertex
// color attribute pointer therefore MUST be declared GL_BGRA.  A
// regression to GL_RGBA produces a 255-channel R/B swap on bars
// 0, 2, 3 (bar 4 = R+B is self-symmetric).  Any of the three
// asymmetric bars failing tells the harness immediately.

triAssertEq [(triDisplay), 0]
triAssertColorbarBytes
triClick 106
