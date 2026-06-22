#pragma once

// Distance format strings (used by HUD cursor drawing)
// Note: Move to stringtable for proper localization (Russian uses Cyrillic "м")
#define STR_POS_DIST "%.0f m"
#define STR_OBJ_DIST "%s (%.0f m)"


namespace Poseidon
{
#if _ENABLE_CHEATS
inline bool tdCheat;
#endif

#define MIN_X (-1.0 * H_PI)
#define MAX_X (1.0 * H_PI)
#define MIN_Y (-0.25 * H_PI)
#define MAX_Y (0.25 * H_PI)
#define TOT_W (MAX_X - MIN_X)
#define TOT_H (MAX_Y - MIN_Y)
#define INV_W (1.0 / TOT_W)
#define INV_H (1.0 / TOT_H)

inline constexpr float dimTime = 10.0f;
inline constexpr float fadeTime = dimTime + 2.0f;
inline constexpr float protectionTime = 0.5f;
inline constexpr float fadeCoef = 1.0f / (fadeTime - dimTime);

} // namespace Poseidon
