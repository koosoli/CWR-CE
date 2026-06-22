#include <PoseidonGL33/GL33BindCache.hpp>
#include <glad/gl.h>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Graphics/Rendering/Frame/RuntimeChecks.hpp>

namespace Poseidon
{
namespace GL33Bind
{

namespace
{
constexpr unsigned int kUnknown = 0xffffffffu;
constexpr int kUnits = 8;

unsigned int g_vao = kUnknown;
unsigned int g_tex[kUnits] = {kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown, kUnknown};
int g_activeUnit = -1;

unsigned long long g_vaoReq = 0, g_vaoBind = 0, g_texReq = 0, g_texBind = 0;

// Per-unit skip counter that paces the B-007 divergence readback (see Tex2D).
unsigned int g_texSkipCtr[kUnits] = {0};
} // namespace

void Vao(unsigned int vao)
{
    ++g_vaoReq;
    if (vao == g_vao)
        return;
    glBindVertexArray(vao);
    g_vao = vao;
    ++g_vaoBind;
}

void ActiveUnit(int unit)
{
    if (unit == g_activeUnit)
        return;
    glActiveTexture(GL_TEXTURE0 + unit);
    g_activeUnit = unit;
}

void Tex2D(int unit, unsigned int tex)
{
    ++g_texReq;
    if (unit >= 0 && unit < kUnits && g_tex[unit] == tex)
    {
        ActiveUnit(unit);
        // B-007 tripwire (active in every build, incl. RWDI/test): the skip is
        // honest only if GL really holds `tex` on this unit. A mismatch is the
        // divergence class from a deleted/recycled handle or a bind outside the
        // cache without Invalidate. Non-fatal: the next non-skip bind
        // self-corrects. The readback is on the renderer's hottest path, so sample
        // 1 of every 64 skips per unit — a divergence is persistent (a poisoned
        // binding stays wrong until rebound), so per-unit sampling still catches it
        // within a sub-frame while keeping glGetIntegerv off the per-skip path.
        if ((g_texSkipCtr[unit]++ & 63u) == 0)
        {
            GLint live = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &live);
            if (auto v = Poseidon::render::frame::DetectBindCacheDivergence(unit, tex, static_cast<unsigned int>(live)))
                LOG_ERROR(Graphics, "GL33 [{}] {}", v->ruleId, v->detail);
        }
        return;
    }
    ActiveUnit(unit);
    glBindTexture(GL_TEXTURE_2D, tex);
    if (unit >= 0 && unit < kUnits)
        g_tex[unit] = tex;
    ++g_texBind;
}

void OnVaoDeleted(unsigned int vao)
{
    if (g_vao == vao)
        g_vao = kUnknown;
}

void OnTexDeleted(unsigned int tex)
{
    for (int i = 0; i < kUnits; i++)
    {
        if (g_tex[i] == tex)
            g_tex[i] = kUnknown;
    }
}

bool IsTexBound(int unit, unsigned int tex)
{
    return unit >= 0 && unit < kUnits && g_tex[unit] == tex;
}

void Invalidate()
{
    g_vao = kUnknown;
    for (int i = 0; i < kUnits; i++)
        g_tex[i] = kUnknown;
    g_activeUnit = -1;
}

unsigned long long VaoRequests() { return g_vaoReq; }
unsigned long long VaoBinds() { return g_vaoBind; }
unsigned long long TexRequests() { return g_texReq; }
unsigned long long TexBinds() { return g_texBind; }

} // namespace GL33Bind
} // namespace Poseidon
