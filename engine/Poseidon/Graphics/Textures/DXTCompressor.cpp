#include <Poseidon/Graphics/Textures/DXTCompressor.hpp>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <climits>
#include <algorithm>

namespace Poseidon
{

namespace DXTCompressor
{

// --- Types matching original BI code ---

typedef uint32_t DWORD;
typedef uint16_t word;
typedef uint8_t byte;

// --- ARGB pixel accessors (original macros, ARGB8888 layout) ---

#define GetA(a) (((a) >> 24) & 0xff)
#define GetR(a) (((a) >> 16) & 0xff)
#define GetG(a) (((a) >> 8) & 0xff)
#define GetB(a) (((a) >> 0) & 0xff)

#define MakeRGB(r, g, b) (((r) << 16) | ((g) << 8) | ((b)))
#define MakeARGB(a, r, g, b) (((a) << 24) | ((r) << 16) | ((g) << 8) | ((b)))

// --- Helpers matching original BI code ---

static inline int biToInt(float f)
{
    return static_cast<int>(f + 0.5f);
}

static inline int biToIntFloor(float f)
{
    return static_cast<int>(f);
}

static inline void biSaturate(int& a, int mn, int mx)
{
    if (a < mn)
        a = mn;
    if (a > mx)
        a = mx;
}

static inline unsigned ConvertPrec(unsigned x, int srcBits, int tgtBits)
{
    const float srcMax = static_cast<float>((1 << srcBits) - 1);
    const float tgtMax = static_cast<float>((1 << tgtBits) - 1);
    return biToInt(x * (tgtMax / srcMax));
}

static inline int Convert565To8888(int dd16)
{
    int r = ((dd16 >> 11) & 0x1f);
    r = ConvertPrec(r, 5, 8);
    int g = ((dd16 >> 5) & 0x3f);
    g = ConvertPrec(g, 6, 8);
    int b = ((dd16 >> 0) & 0x1f);
    b = ConvertPrec(b, 5, 8);
    return MakeARGB(0xff, r, g, b);
}

static inline int Convert8888To565(DWORD dd32)
{
    int r = GetR(dd32), g = GetG(dd32), b = GetB(dd32);
    int ir = ConvertPrec(r, 8, 5);
    int ig = ConvertPrec(g, 8, 6);
    int ib = ConvertPrec(b, 8, 5);
    return (ir << 11) | (ig << 5) | ib;
}

// --- DXT block structures (original BI layout) ---

struct DXTBlock64
{
    word c0, c1;
    word tex0, tex1;
};

struct DXTBlock64AlphaExplicit
{
    word a[4];
};

struct DXTBlock64AlphaImplicit
{
    byte a0, a1;
    byte tex[6];
};

struct DXTBlock128
{
    union
    {
        DXTBlock64AlphaExplicit alphaExp;
        DXTBlock64AlphaImplicit alphaImp;
    };
    DXTBlock64 color;
};

// --- Div3 lookup table (original BI optimization) ---
// note: int. division very slow - precalc. table instead

static struct Div3Init
{
    char _table[512];

    Div3Init()
    {
        for (int i = 256; i < 512; i++)
        {
            _table[i] = (i - 256 + 1) / 3;
        }
        for (int i = 0; i < 256; i++)
        {
            _table[i] = -((256 - i + 1) / 3);
        }
    }
    int operator()(int x) const
    {
        x += 256;
        return _table[x];
    }
} Div3;

// --- Perceptual error weights ---
// note: 5, 9, 2 is quite good approximation (and gives sum 16)

static const int REyeI = 5;
static const int GEyeI = 9;
static const int BEyeI = 2;

static inline int Distance8(int a, int b)
{
    return abs(a - b);
}

static inline int SquareDistance8888(DWORD a, DWORD b)
{
    return (int(GetR(a) - GetR(b)) * int(GetR(a) - GetR(b)) * REyeI * REyeI +
            int(GetG(a) - GetG(b)) * int(GetG(a) - GetG(b)) * GEyeI * GEyeI +
            int(GetB(a) - GetB(b)) * int(GetB(a) - GetB(b)) * BEyeI * BEyeI);
}

// Euclidean distance seems to make more sense than a sum
#define FUNC_DISTANCE8888 SquareDistance8888

static inline int Brightness8888(DWORD a)
{
    return (GetR(a) * REyeI + GetG(a) * GEyeI + GetB(a) * BEyeI);
}

// --- Color handling ---

struct ColorCount
{
    int col;
    int count;
    ColorCount()
    {
        col = 0;
        count = 0;
    }
    explicit ColorCount(int c, int cc = 1)
    {
        col = c;
        count = cc;
    }
};

static void AddColor(ColorCount* colors, int& nColors, int col, int count = 1)
{
    for (int c = 0; c < nColors; c++)
    {
        if (col == colors[c].col)
        {
            colors[c].count += count;
            return;
        }
    }
    colors[nColors] = ColorCount(col, count);
    nColors++;
}

// --- DXT color selection heuristic ---

static void DXTColorsMonocolor(DWORD& c0, DWORD& c1, const ColorCount* colors, int nColors)
{
    // keep brightness but convert to same color tone
    // track min and max brigtness
    int sumR = 0, sumG = 0, sumB = 0;
    int minBrightness = INT_MAX;
    int maxBrightness = 0;
    for (int i = 0; i < nColors; i++)
    {
        DWORD pix = colors[i].col;
        int count = colors[i].count;
        sumR += GetR(pix) * count;
        sumG += GetG(pix) * count;
        sumB += GetB(pix) * count;
        int brightness = Brightness8888(pix);
        if (minBrightness > brightness)
            minBrightness = brightness;
        if (maxBrightness < brightness)
            maxBrightness = brightness;
    }
    // normalize brightness
    // calculate total brightness of average color
    float sumBrightness = float(sumR) * REyeI + float(sumG) * GEyeI + float(sumB) * BEyeI;
    if (sumBrightness < 1e-3 && nColors > 0)
    {
        // pick any color with zero alpha to represent transparent areas
        for (int i = 0; i < nColors; i++)
        {
            if (colors[i].count > 0)
                continue;
            DWORD pix = colors[i].col;
            sumR += GetR(pix);
            sumG += GetG(pix);
            sumB += GetB(pix);
            int brightness = Brightness8888(pix);
            if (minBrightness > brightness)
                minBrightness = brightness;
            if (maxBrightness < brightness)
                maxBrightness = brightness;
        }
        sumBrightness = float(sumR) * REyeI + float(sumG) * GEyeI + float(sumB) * BEyeI;
    }
    if (sumBrightness <= 0.1)
        sumBrightness = 0.1;
    // create a color which has brightness, minBrightness but the same color as sumR, sumG, sumB

    float minFactor = minBrightness / sumBrightness;
    float maxFactor = maxBrightness / sumBrightness;

    int minR = biToInt(minFactor * sumR);
    int minG = biToInt(minFactor * sumG);
    int minB = biToInt(minFactor * sumB);
    biSaturate(minR, 0, 255);
    biSaturate(minG, 0, 255);
    biSaturate(minB, 0, 255);

    int maxR = biToInt(maxFactor * sumR);
    int maxG = biToInt(maxFactor * sumG);
    int maxB = biToInt(maxFactor * sumB);
    biSaturate(maxR, 0, 255);
    biSaturate(maxG, 0, 255);
    biSaturate(maxB, 0, 255);

    c0 = MakeRGB(minR, minG, minB);
    c1 = MakeRGB(maxR, maxG, maxB);
}

static int CmpColorCount(const void* c0, const void* c1)
{
    const ColorCount* cc0 = static_cast<const ColorCount*>(c0);
    const ColorCount* cc1 = static_cast<const ColorCount*>(c1);
    return cc1->count - cc0->count;
}

static int CmpColorValue(const void* c0, const void* c1)
{
    const ColorCount* cc0 = static_cast<const ColorCount*>(c0);
    const ColorCount* cc1 = static_cast<const ColorCount*>(c1);
    return cc1->col - cc0->col;
}

// --- Alpha block compression ---

static void Create8Alphas8(int* alpha, int a0, int a1)
{
    float inv7 = 1.0f / 7;
    alpha[0] = a0;
    alpha[1] = a1;
    alpha[2] = biToIntFloor((6 * a0 + 1 * a1 + 3) * inv7);
    alpha[3] = biToIntFloor((5 * a0 + 2 * a1 + 3) * inv7);
    alpha[4] = biToIntFloor((4 * a0 + 3 * a1 + 3) * inv7);
    alpha[5] = biToIntFloor((3 * a0 + 4 * a1 + 3) * inv7);
    alpha[6] = biToIntFloor((2 * a0 + 5 * a1 + 3) * inv7);
    alpha[7] = biToIntFloor((1 * a0 + 6 * a1 + 3) * inv7);
}

static void Create8Alphas6(int* alpha, int a0, int a1)
{
    float inv5 = 1.0f / 5;
    alpha[0] = a0;
    alpha[1] = a1;
    alpha[2] = biToIntFloor((4 * a0 + 1 * a1 + 2) * inv5);
    alpha[3] = biToIntFloor((3 * a0 + 2 * a1 + 2) * inv5);
    alpha[4] = biToIntFloor((2 * a0 + 3 * a1 + 2) * inv5);
    alpha[5] = biToIntFloor((1 * a0 + 4 * a1 + 2) * inv5);
    alpha[6] = 0;
    alpha[7] = 0xff;
}

static void CompressDXTABlock(DXTBlock64AlphaExplicit& tgt, const DWORD* pixels)
{
    for (int i = 0; i < 4; i++)
    {
        word res = (((GetA(pixels[i * 4 + 0]) >> 4) << 0) | ((GetA(pixels[i * 4 + 1]) >> 4) << 4) |
                    ((GetA(pixels[i * 4 + 2]) >> 4) << 8) | ((GetA(pixels[i * 4 + 3]) >> 4) << 12));
        tgt.a[i] = res;
    }
}

static double CalcErrorNearestAlpha(const ColorCount* source, int nSource, const int* alpha, int nAlphas,
                                    double maxError)
{
    // scan all source colors and select nearest color for each
    double error = 0;
    for (int i = 0; i < nSource; i++)
    {
        int p = source[i].col;
        int nearestDist = INT_MAX;
        for (int c = 0; c < nAlphas; c++)
        {
            int dist = Distance8(alpha[c], p);
            if (nearestDist > dist)
            {
                nearestDist = dist;
            }
        }
        error += nearestDist * source[i].count;
        if (error >= maxError)
            return error;
    }
    return error;
}

static void SelectNearestAlpha(int* codes, const DWORD* pixels, const int* alpha)
{
    // alpha count is always eight
    for (int i = 0; i < 16; i++)
    {
        int a = GetA(pixels[i]);
        int nearest = 0;
        int nearestDist = INT_MAX;
        for (int c = 0; c < 8; c++)
        {
            int dist = Distance8(alpha[c], a);
            if (nearestDist > dist)
            {
                nearestDist = dist;
                nearest = c;
            }
        }
        codes[i] = nearest;
    }
}

static void CompressDXTABlock(DXTBlock64AlphaImplicit& tgt, const DWORD* pixels)
{
    // scan how many unique alpha values are present here
    ColorCount alphas[16];
    int nAlphas = 0;
    for (int i = 0; i < 16; i++)
    {
        int a = GetA(pixels[i]);
        AddColor(alphas, nAlphas, a);
    }
    double bestError = DBL_MAX;

    qsort(alphas, nAlphas, sizeof(*alphas), CmpColorValue);

    // provide some reasonable default values
    int bestA0 = alphas[nAlphas - 1].col;
    int bestA1 = alphas[0].col;

    // Brute force between existing colors
    for (int endAlphas = nAlphas; endAlphas >= 2; endAlphas--)
        for (int startAlphas = 0; startAlphas <= endAlphas - 2; startAlphas++)
        {
            int a0 = alphas[startAlphas].col;
            int a1 = alphas[endAlphas - 1].col;
            if (a0 == a1)
            {
                if (a0 == 0xff)
                    a1--;
                else
                    a0++;
            }
            int alpha[8];
            // try 8 colors first
            Create8Alphas8(alpha, a0, a1);
            double error = CalcErrorNearestAlpha(alphas, nAlphas, alpha, 8, bestError);
            if (bestError > error)
            {
                bestA0 = a0;
                bestA1 = a1;
                bestError = error;
            }
            // try 6 colors
            Create8Alphas6(alpha, a1, a0);
            error = CalcErrorNearestAlpha(alphas, nAlphas, alpha, 8, bestError);
            if (bestError > error)
            {
                bestA0 = a1;
                bestA1 = a0;
                bestError = error;
            }
            if (bestError <= 0)
                goto BestFoundAlpha;
        }
BestFoundAlpha:

    int alpha[8];
    if (bestA0 > bestA1)
    {
        Create8Alphas8(alpha, bestA0, bestA1);
    }
    else
    {
        Create8Alphas6(alpha, bestA0, bestA1);
    }
    int codes[16];
    SelectNearestAlpha(codes, pixels, alpha);

    // encode implicit block
    tgt.a0 = bestA0;
    tgt.a1 = bestA1;
    tgt.tex[0] = (codes[0]) | (codes[1] << 3) | (codes[2] << 6);
    tgt.tex[1] = (codes[2] >> 2) | (codes[3] << 1) | (codes[4] << 4) | (codes[5] << 7);
    tgt.tex[2] = (codes[5] >> 1) | (codes[6] << 2) | (codes[7] << 5);
    tgt.tex[3] = (codes[8]) | (codes[9] << 3) | (codes[10] << 6);
    tgt.tex[4] = (codes[10] >> 2) | (codes[11] << 1) | (codes[12] << 4) | (codes[13] << 7);
    tgt.tex[5] = (codes[13] >> 1) | (codes[14] << 2) | (codes[15] << 5);
}

// --- Color error calculation ---

static double CalcErrorNearest(const ColorCount* source, int nSource, const DWORD* color, int nColors, double maxError)
{
    double error = 0;
    for (int i = 0; i < nSource; i++)
    {
        DWORD p = source[i].col;
        int nearestDist = INT_MAX;
        for (int c = 0; c < nColors; c++)
        {
            int dist = FUNC_DISTANCE8888(color[c], p);
            if (nearestDist > dist)
            {
                nearestDist = dist;
            }
        }
        error += nearestDist * source[i].count;
        if (error >= maxError)
            return error;
    }
    return error;
}

static void SelectNearest(int* codes, const DWORD* pixels, const DWORD* color, int nColors, bool alpha)
{
    for (int i = 0; i < 16; i++)
    {
        int p = pixels[i];
        if (!alpha && GetA(p) < 128)
        {
            codes[i] = 3;
        }
        else
        {
            int nearest = 0;
            int nearestDist = INT_MAX;
            for (int c = 0; c < nColors; c++)
            {
                int dist = FUNC_DISTANCE8888(color[c], p);
                if (nearestDist > dist)
                {
                    nearestDist = dist;
                    nearest = c;
                }
            }
            codes[i] = nearest;
        }
    }
}

// --- Interpolated color creation ---

static void Create4Colors(DWORD* color, DWORD c0RGB, DWORD c1RGB)
{
    int rc0 = GetR(c0RGB);
    int gc0 = GetG(c0RGB);
    int bc0 = GetB(c0RGB);
    int rc1 = GetR(c1RGB);
    int gc1 = GetG(c1RGB);
    int bc1 = GetB(c1RGB);

    int rd3 = Div3(rc1 - rc0);
    int gd3 = Div3(gc1 - gc0);
    int bd3 = Div3(bc1 - bc0);

    int rc2 = rc0 + rd3;
    int gc2 = gc0 + gd3;
    int bc2 = bc0 + bd3;

    int rc3 = rc1 - rd3;
    int gc3 = gc1 - gd3;
    int bc3 = bc1 - bd3;
    int c2RGB = MakeRGB(rc2, gc2, bc2);
    int c3RGB = MakeRGB(rc3, gc3, bc3);
    color[0] = c0RGB | 0xff000000;
    color[1] = c1RGB | 0xff000000;
    color[2] = c2RGB | 0xff000000;
    color[3] = c3RGB | 0xff000000;
}

static void Create3Colors(DWORD* color, DWORD c0RGB, DWORD c1RGB)
{
    int rc0 = GetR(c0RGB);
    int gc0 = GetG(c0RGB);
    int bc0 = GetB(c0RGB);
    int rc1 = GetR(c1RGB);
    int gc1 = GetG(c1RGB);
    int bc1 = GetB(c1RGB);

    DWORD c2RGB = MakeRGB((rc0 + rc1) >> 1, (gc0 + gc1) >> 1, (bc0 + bc1) >> 1);

    color[0] = c0RGB | 0xff000000;
    color[1] = c1RGB | 0xff000000;
    color[2] = c2RGB | 0xff000000;
}

// --- DXT1 block compression (original BI algorithm) ---
// Heuristic + brute force optimal color pair search with perceptual error weighting

static double CompressDXT1Block(DXTBlock64& tgt, const DWORD* pixels, bool alpha = false)
{
    DWORD bestC0 = 0, bestC1 = 0;
    int bestCodes[16];
    double bestError = DBL_MAX;

    bool transparent = false;
    bool transparentOnly = true;
    if (!alpha)
        for (int i = 0; i < 16; i++)
        {
            DWORD col = pixels[i];
            int a = GetA(col);
            if (a < 128)
                transparent = true;
            else
                transparentOnly = false;
        }

    // calculate color counts
    ColorCount colors[16];
    int nColors = 0;
    for (int i = 0; i < 16; i++)
    {
        DWORD pix = pixels[i];
        // do not count transparent pixels
        if (!alpha && GetA(pix) < 0x80)
            continue;
        AddColor(colors, nColors, pix);
    }

    if (!alpha && transparent && transparentOnly)
    {
        // scan the block as if it is not transparent
        // use mono-color - this will get us the average color
        colors[0].col = pixels[0];
        colors[0].count = 1;
        nColors = 1;
        DWORD c0, c1;
        DXTColorsMonocolor(c0, c1, colors, nColors);

        c0 = Convert8888To565(c0);
        c1 = Convert8888To565(c1);
        // note: do indicate transparency we must have: c0<=c1
        if (c0 > c1)
        {
            DWORD t = c0;
            c0 = c1;
            c1 = t;
        }

        bestC0 = c0;
        bestC1 = c1;

        for (int i = 0; i < 16; i++)
        {
            // all pixels transparent - use transparency
            bestCodes[i] = 3;
        }
    }
    else
    {
        // sort colors so that the most common are first
        qsort(colors, nColors, sizeof(*colors), CmpColorCount);

        // only unique 565 colors need to be considered for brute force
        ColorCount colors565[16];
        int nColors565 = 0;
        for (int i = 0; i < nColors; i++)
        {
            int color565 = Convert8888To565(colors[i].col);
            AddColor(colors565, nColors565, color565, colors[i].count);
        }

        // check heuristic algorithm
        {
            DWORD c0RGB, c1RGB;
            DXTColorsMonocolor(c0RGB, c1RGB, colors, nColors);

            DWORD c0 = Convert8888To565(c0RGB);
            DWORD c1 = Convert8888To565(c1RGB);

            // convert back again - source colors need to be exact
            c0RGB = Convert565To8888(c0);
            c1RGB = Convert565To8888(c1);

            double error = 0;

            if (transparent)
            {
                DWORD color[4];
                // note: do indicate transparency we must have: c0<=c1
                if (c0 > c1)
                {
                    DWORD t = c0;
                    c0 = c1;
                    c1 = t;
                    DWORD tRGB = c0RGB;
                    c0RGB = c1RGB;
                    c1RGB = tRGB;
                }
                Create3Colors(color, c0RGB, c1RGB);
                error = CalcErrorNearest(colors, nColors, color, 3, bestError);
            }
            else
            {
                // note: to indicate opacity we must have: c0>c1
                if (c0 < c1)
                {
                    DWORD t = c0;
                    c0 = c1;
                    c1 = t;
                    DWORD tRGB = c0RGB;
                    c0RGB = c1RGB;
                    c1RGB = tRGB;
                }
                DWORD color[4];
                Create4Colors(color, c0RGB, c1RGB);
                error = CalcErrorNearest(colors, nColors, color, 4, bestError);
            }
            if (bestError > error)
            {
                bestError = error;
                bestC0 = c0;
                bestC1 = c1;
            }
        }

        // brute force between used colors
        if (bestError > 0)
        {
            qsort(colors565, nColors565, sizeof(*colors565), CmpColorCount);
            // start with the most popular colors
            for (int i = 0; i < nColors565; i++)
                for (int j = 0; j <= i; j++)
                {
                    DWORD c0 = colors565[i].col;
                    DWORD c1 = colors565[j].col;

                    DWORD c0RGB = Convert565To8888(c0);
                    DWORD c1RGB = Convert565To8888(c1);

                    DWORD color[4];
                    int nc;
                    if (transparent)
                    {
                        // note: do indicate transparency we must have: c0<=c1
                        if (c0 > c1)
                        {
                            DWORD t = c0;
                            c0 = c1;
                            c1 = t;
                            DWORD tRGB = c0RGB;
                            c0RGB = c1RGB;
                            c1RGB = tRGB;
                        }
                        Create3Colors(color, c0RGB, c1RGB);
                        nc = 3;
                    }
                    else
                    {
                        // note: to indicate opacity we must have: c0>c1
                        if (c0 < c1)
                        {
                            DWORD t = c0;
                            c0 = c1;
                            c1 = t;
                            DWORD tRGB = c0RGB;
                            c0RGB = c1RGB;
                            c1RGB = tRGB;
                        }
                        Create4Colors(color, c0RGB, c1RGB);
                        nc = 4;
                    }

                    double error = CalcErrorNearest(colors, nColors, color, nc, bestError);
                    if (bestError > error)
                    {
                        bestError = error;
                        bestC0 = c0;
                        bestC1 = c1;
                    }
                    // sometimes we might get better result with 3-point approximation
                    if (!alpha && nc == 4)
                    {
                        // note: do indicate transparency we must have: c0<=c1
                        if (c0 > c1)
                        {
                            DWORD t = c0;
                            c0 = c1;
                            c1 = t;
                            DWORD tRGB = c0RGB;
                            c0RGB = c1RGB;
                            c1RGB = tRGB;
                        }
                        Create3Colors(color, c0RGB, c1RGB);
                        double error = CalcErrorNearest(colors, nColors, color, 3, bestError);
                        if (bestError > error)
                        {
                            bestError = error;
                            bestC0 = c0;
                            bestC1 = c1;
                        }
                    }
                    if (bestError <= 0)
                        goto BestFound;
                }
        BestFound:;
        }

        // try to improve the result by using brute force search around endpoints
        {
            // bestC0 and bestC1 are 565 encoded
            int rc0 = (bestC0 >> 11) & 0x1f;
            int gc0 = (bestC0 >> 5) & 0x3f;
            int bc0 = (bestC0 >> 0) & 0x1f;
            int rc1 = (bestC1 >> 11) & 0x1f;
            int gc1 = (bestC1 >> 5) & 0x3f;
            int bc1 = (bestC1 >> 0) & 0x1f;

            // by extending range only we get:
            //   2*2*2=8 possibilities for each endpoint
            //   64 possibilities in total
            int rRange = 1;
            int gRange = 1;
            int bRange = 1;

            // only extend range, never lower it
            int r0Min = rc0 <= rc1 ? rc0 - rRange : rc0;
            int r0Max = rc0 > rc1 ? rc0 + rRange : rc0;
            int r1Min = rc1 < rc0 ? rc1 - rRange : rc1;
            int r1Max = rc1 >= rc0 ? rc1 + rRange : rc1;
            biSaturate(r0Min, 0, 0x1f);
            biSaturate(r0Max, 0, 0x1f);
            biSaturate(r1Min, 0, 0x1f);
            biSaturate(r1Max, 0, 0x1f);

            int g0Min = gc0 <= gc1 ? gc0 - gRange : gc0;
            int g0Max = gc0 > gc1 ? gc0 + gRange : gc0;
            int g1Min = gc1 < gc0 ? gc1 - gRange : gc1;
            int g1Max = gc1 >= gc0 ? gc1 + gRange : gc1;
            biSaturate(g0Min, 0, 0x3f);
            biSaturate(g0Max, 0, 0x3f);
            biSaturate(g1Min, 0, 0x3f);
            biSaturate(g1Max, 0, 0x3f);

            int b0Min = bc0 <= bc1 ? bc0 - bRange : bc0;
            int b0Max = bc0 > bc1 ? bc0 + bRange : bc0;
            int b1Min = bc1 < bc0 ? bc1 - bRange : bc1;
            int b1Max = bc1 >= bc0 ? bc1 + bRange : bc1;
            biSaturate(b0Min, 0, 0x1f);
            biSaturate(b0Max, 0, 0x1f);
            biSaturate(b1Min, 0, 0x1f);
            biSaturate(b1Max, 0, 0x1f);

            for (int rn0 = r0Min; rn0 <= r0Max; rn0++)
                for (int gn0 = g0Min; gn0 <= g0Max; gn0++)
                    for (int bn0 = b0Min; bn0 <= b0Max; bn0++)
                        for (int rn1 = r1Min; rn1 <= r1Max; rn1++)
                            for (int gn1 = g1Min; gn1 <= g1Max; gn1++)
                                for (int bn1 = b1Min; bn1 <= b1Max; bn1++)
                                {
                                    DWORD c0 = (rn0 << 11) | (gn0 << 5) | bn0;
                                    DWORD c1 = (rn1 << 11) | (gn1 << 5) | bn1;
                                    DWORD c0RGB = Convert565To8888(c0);
                                    DWORD c1RGB = Convert565To8888(c1);

                                    DWORD color[4];
                                    if (!transparent)
                                    {
                                        // note: to indicate opacity we must have: c0>c1
                                        if (c0 < c1)
                                        {
                                            DWORD t = c0;
                                            c0 = c1;
                                            c1 = t;
                                            DWORD tRGB = c0RGB;
                                            c0RGB = c1RGB;
                                            c1RGB = tRGB;
                                        }
                                        Create4Colors(color, c0RGB, c1RGB);
                                        double error = CalcErrorNearest(colors, nColors, color, 4, bestError);
                                        if (bestError > error)
                                        {
                                            bestError = error;
                                            bestC0 = c0;
                                            bestC1 = c1;
                                        }
                                    }

                                    if (!alpha)
                                    {
                                        if (c0 > c1)
                                        {
                                            DWORD t = c0;
                                            c0 = c1;
                                            c1 = t;
                                            DWORD tRGB = c0RGB;
                                            c0RGB = c1RGB;
                                            c1RGB = tRGB;
                                        }
                                        Create3Colors(color, c0RGB, c1RGB);
                                        double error = CalcErrorNearest(colors, nColors, color, 3, bestError);
                                        if (bestError > error)
                                        {
                                            bestError = error;
                                            bestC0 = c0;
                                            bestC1 = c1;
                                        }
                                    }
                                }
        }
    }

    if (bestC0 > bestC1)
    {
        DWORD color[4];
        DWORD bestC0RGB = Convert565To8888(bestC0);
        DWORD bestC1RGB = Convert565To8888(bestC1);
        Create4Colors(color, bestC0RGB, bestC1RGB);
        SelectNearest(bestCodes, pixels, color, 4, alpha);
    }
    else
    {
        DWORD color[3];
        DWORD bestC0RGB = Convert565To8888(bestC0);
        DWORD bestC1RGB = Convert565To8888(bestC1);
        Create3Colors(color, bestC0RGB, bestC1RGB);
        SelectNearest(bestCodes, pixels, color, 3, alpha);
    }

    // store colors to c0 and c1
    tgt.c0 = bestC0;
    tgt.c1 = bestC1;

    // store 2bit codes
    tgt.tex0 = ((bestCodes[0] << 0) | (bestCodes[1] << 2) | (bestCodes[2] << 4) | (bestCodes[3] << 6) |
                (bestCodes[4] << 8) | (bestCodes[5] << 10) | (bestCodes[6] << 12) | (bestCodes[7] << 14));
    tgt.tex1 =
        ((bestCodes[0 + 8] << 0) | (bestCodes[1 + 8] << 2) | (bestCodes[2 + 8] << 4) | (bestCodes[3 + 8] << 6) |
         (bestCodes[4 + 8] << 8) | (bestCodes[5 + 8] << 10) | (bestCodes[6 + 8] << 12) | (bestCodes[7 + 8] << 14));
    return bestError;
}

// --- RGBA8888 to ARGB8888 conversion for block ---

static void extractBlockARGB(const uint8_t* rgba, int imgW, int imgH, int bx, int by, DWORD block[16])
{
    for (int py = 0; py < 4; ++py)
    {
        for (int px = 0; px < 4; ++px)
        {
            int x = bx * 4 + px;
            int y = by * 4 + py;
            int dst = py * 4 + px;
            if (x < imgW && y < imgH)
            {
                int src = (y * imgW + x) * 4;
                // RGBA -> ARGB
                block[dst] = MakeARGB(rgba[src + 3], rgba[src + 0], rgba[src + 1], rgba[src + 2]);
            }
            else
            {
                block[dst] = 0;
            }
        }
    }
}

// --- Public API ---

size_t CompressedSize(int width, int height, bool isDXT1)
{
    int bw = (width + 3) / 4;
    int bh = (height + 3) / 4;
    return static_cast<size_t>(bw) * bh * (isDXT1 ? 8 : 16);
}

std::vector<uint8_t> CompressDXT1(const uint8_t* rgba, int width, int height)
{
    size_t sz = CompressedSize(width, height, true);
    std::vector<uint8_t> out(sz);
    int bw = (width + 3) / 4;
    int bh = (height + 3) / 4;
    auto* dst = reinterpret_cast<DXTBlock64*>(out.data());

    for (int by = 0; by < bh; ++by)
    {
        for (int bx = 0; bx < bw; ++bx)
        {
            DWORD pixels[16];
            extractBlockARGB(rgba, width, height, bx, by, pixels);
            CompressDXT1Block(*dst, pixels);
            dst++;
        }
    }
    return out;
}

std::vector<uint8_t> CompressDXT3(const uint8_t* rgba, int width, int height)
{
    size_t sz = CompressedSize(width, height, false);
    std::vector<uint8_t> out(sz);
    int bw = (width + 3) / 4;
    int bh = (height + 3) / 4;
    auto* dst = reinterpret_cast<DXTBlock128*>(out.data());

    for (int by = 0; by < bh; ++by)
    {
        for (int bx = 0; bx < bw; ++bx)
        {
            DWORD pixels[16];
            extractBlockARGB(rgba, width, height, bx, by, pixels);
            CompressDXT1Block(dst->color, pixels, true);
            CompressDXTABlock(dst->alphaExp, pixels);
            dst++;
        }
    }
    return out;
}

std::vector<uint8_t> CompressDXT5(const uint8_t* rgba, int width, int height)
{
    size_t sz = CompressedSize(width, height, false);
    std::vector<uint8_t> out(sz);
    int bw = (width + 3) / 4;
    int bh = (height + 3) / 4;
    auto* dst = reinterpret_cast<DXTBlock128*>(out.data());

    for (int by = 0; by < bh; ++by)
    {
        for (int bx = 0; bx < bw; ++bx)
        {
            DWORD pixels[16];
            extractBlockARGB(rgba, width, height, bx, by, pixels);
            CompressDXT1Block(dst->color, pixels, true);
            CompressDXTABlock(dst->alphaImp, pixels);
            dst++;
        }
    }
    return out;
}

} // namespace DXTCompressor

} // namespace Poseidon
