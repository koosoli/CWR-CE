#include <PoseidonGL33/EngineGL33.hpp>
#include <Poseidon/Graphics/Textures/TexturePreload.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>
#include <Poseidon/Graphics/Rendering/Primitives/Clip2D.hpp>
#include <Poseidon/World/Scene/Scene.hpp>

void EngineGL33::Draw2D(const Draw2DPars& pars, const Rect2DAbs& rect, const Rect2DAbs& clip)
{
    PoseidonAssert(pars.mip.IsOK());
    if (!pars.mip.IsOK())
    {
        return;
    }

    if (_resetNeeded)
    {
        return;
    }

    // OpenGL uses pixel-corner addressing — vertex positions are taken as
    // pixel coordinates directly (no D3D9 -0.5 shift, no shader half-pixel
    // compensation).
    float xBeg = rect.x, xEnd = xBeg + rect.w;
    float yBeg = rect.y, yEnd = yBeg + rect.h;

    float uBeg = 0;
    float vBeg = 0;
    float uEnd = 1;
    float vEnd = 1;

    float xc = floatMax(clip.x, 0);
    float yc = floatMax(clip.y, 0);
    float xec = floatMin(clip.x + clip.w, _w);
    float yec = floatMin(clip.y + clip.h, _h);

    if (xBeg < xc)
    {
        uBeg = (xc - xBeg) / rect.w;
        xBeg = xc;
    }
    if (xEnd > xec)
    {
        uEnd = 1 - (xEnd - xec) / rect.w;
        xEnd = xec;
    }
    if (yBeg < yc)
    {
        vBeg = (yc - yBeg) / rect.h;
        yBeg = yc;
    }
    if (yEnd > yec)
    {
        vEnd = 1 - (yEnd - yec) / rect.h;
        yEnd = yec;
    }

    if (xBeg >= xEnd || yBeg >= yEnd)
    {
        return;
    }

    TLVertex pos[4];
    pos[0].rhw = 1;
    pos[0].color = pars.colorTL;
    pos[0].specular = PackedColor(0xff000000);
    pos[0].pos[2] = 0.5;

    pos[1].rhw = 1;
    pos[1].color = pars.colorTR;
    pos[1].specular = PackedColor(0xff000000);
    pos[1].pos[2] = 0.5;

    pos[2].rhw = 1;
    pos[2].color = pars.colorBR;
    pos[2].specular = PackedColor(0xff000000);
    pos[2].pos[2] = 0.5;

    pos[3].rhw = 1;
    pos[3].color = pars.colorBL;
    pos[3].specular = PackedColor(0xff000000);
    pos[3].pos[2] = 0.5;

    float uTL = pars.uTL + uBeg * (pars.uTR - pars.uTL) + vBeg * (pars.uBL - pars.uTL);
    float uTR = pars.uTL + uEnd * (pars.uTR - pars.uTL) + vBeg * (pars.uBL - pars.uTL);
    float uBL = pars.uTL + uBeg * (pars.uTR - pars.uTL) + vEnd * (pars.uBL - pars.uTL);
    float uBR = pars.uTL + uEnd * (pars.uTR - pars.uTL) + vEnd * (pars.uBL - pars.uTL);

    float vTL = pars.vTL + uBeg * (pars.vTR - pars.vTL) + vBeg * (pars.vBL - pars.vTL);
    float vTR = pars.vTL + uEnd * (pars.vTR - pars.vTL) + vBeg * (pars.vBL - pars.vTL);
    float vBL = pars.vTL + uBeg * (pars.vTR - pars.vTL) + vEnd * (pars.vBL - pars.vTL);
    float vBR = pars.vTL + uEnd * (pars.vTR - pars.vTL) + vEnd * (pars.vBL - pars.vTL);

    pos[0].pos[0] = xBeg, pos[0].pos[1] = yBeg;
    pos[1].pos[0] = xEnd, pos[1].pos[1] = yBeg;
    pos[2].pos[0] = xEnd, pos[2].pos[1] = yEnd;
    pos[3].pos[0] = xBeg, pos[3].pos[1] = yEnd;
    pos[0].t0.u = uTL, pos[0].t0.v = vTL;
    pos[1].t0.u = uTR, pos[1].t0.v = vTR;
    pos[2].t0.u = uBR, pos[2].t0.v = vBR;
    pos[3].t0.u = uBL, pos[3].t0.v = vBL;

    SwitchRenderMode(RM2DTris);
    BeginScreenPass();

    AddVertices(pos, 4); // note: may flush all queues

    QueuePrepareTriangle(pars.mip, pars.spec);

    Queue2DPoly(pos, 4);
}

void EngineGL33::DrawLine(const Line2DAbs& line, PackedColor c0, PackedColor c1, const Rect2DAbs& clip)
{
    float x0 = line.beg.x;
    float y0 = line.beg.y;
    float x1 = line.end.x;
    float y1 = line.end.y;
    // use line texture
    Texture* tex = GPreloadedTextures.New(TextureLine);
    const MipInfo& mip = TextBank()->UseMipmap(tex, 1, 1);

    // convert line to poly;
    int specFlags = NoZBuf | IsAlpha | ClampU | ClampV | IsAlphaFog;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dSize2 = dx * dx + dy * dy;
    float invDSize = dSize2 > 0 ? InvSqrt(dSize2) : 1;

    // direction perpendicular dx, dy
    // 2D line drawing
    float pdx = +dy * invDSize, pdy = -dx * invDSize;
    float w = 3.0f;
    x0 -= pdx * (w * 0.5);
    x1 -= pdx * (w * 0.5);
    y0 -= pdy * (w * 0.5);
    y1 -= pdy * (w * 0.5);
    float x0Side = x0 + pdx * w, y0Side = y0 + pdy * w;
    float x1Side = x1 + pdx * w, y1Side = y1 + pdy * w;

    Vertex2DAbs vertices[4];
    float off = 0;
    vertices[0].x = x0 - off;
    vertices[0].y = y0 - off;
    vertices[0].u = 0;
    vertices[0].v = 0.25;
    vertices[0].color = c0;

    vertices[1].x = x0Side - off;
    vertices[1].y = y0Side - off;
    vertices[1].u = 0;
    vertices[1].v = 1;
    vertices[1].color = c0;

    vertices[3].x = x1 - off;
    vertices[3].y = y1 - off;
    vertices[3].u = 0.1;
    vertices[3].v = 0.25;
    vertices[3].color = c1;

    vertices[2].x = x1Side - off;
    vertices[2].y = y1Side - off;
    vertices[2].u = 0.1;
    vertices[2].v = 1;
    vertices[2].color = c1;

    DrawPoly(mip, vertices, 4, clip, specFlags);
}

void EngineGL33::DrawPoly(const MipInfo& mip, const Vertex2DPixel* vertices, int n, const Rect2DPixel& clipRect,
                          int specFlags)
{
    if (_resetNeeded)
    {
        return;
    }

    const int maxN = 32;

    // reject poly if fully outside or invalid
    ClipFlags orClip = 0;
    ClipFlags andClip = ClipAll;
    ClipFlags clipV[maxN];
    for (int i = 0; i < n; i++)
    {
        const Vertex2DPixel& vs = vertices[i];
        float x = vs.x;
        float y = vs.y;
        ClipFlags clip = 0;
        if (x < clipRect.x)
        {
            clip |= ClipLeft;
        }
        else if (x > clipRect.x + clipRect.w)
        {
            clip |= ClipRight;
        }
        if (y < clipRect.y)
        {
            clip |= ClipTop;
        }
        else if (y > clipRect.y + clipRect.h)
        {
            clip |= ClipBottom;
        }
        clipV[i] = clip;
        orClip |= clip;
        andClip &= clip;
    }
    if (andClip)
    {
        return;
    }
    Vertex2DPixel clippedVertices1[maxN]; // temporay buffer to keep clipped result
    Vertex2DPixel clippedVertices2[maxN]; // temporay buffer to keep clipped result
    // 2D clipping (with orClip flags)
    if (orClip)
    {
        Vertex2DPixel* free = clippedVertices1;
        Vertex2DPixel* used = clippedVertices2;
        // perform clipping
        for (int i = 0; i < n; i++)
        {
            used[i] = vertices[i];
        }
        if (orClip & ClipTop)
        {
            n = Clip2D(clipRect, free, used, n, InsideTopPixel), swap(free, used);
        }
        if (orClip & ClipBottom)
        {
            n = Clip2D(clipRect, free, used, n, InsideBottomPixel), swap(free, used);
        }
        if (orClip & ClipLeft)
        {
            n = Clip2D(clipRect, free, used, n, InsideLeftPixel), swap(free, used);
        }
        if (orClip & ClipRight)
        {
            n = Clip2D(clipRect, free, used, n, InsideRightPixel), swap(free, used);
        }
        // use result
        if (n < 3)
        {
            return; // nothing to draw
        }
        vertices = used;
    }

    if (n > maxN)
    {
        n = maxN;
        Fail("Poly: Too much vertices");
    }

    TLVertex gv[maxN];

    float x2d = Left2D();
    float y2d = Top2D();

    for (int i = 0; i < n; i++)
    {
        TLVertex* v = &gv[i];
        const Vertex2DPixel& vs = vertices[i];

        v->pos[0] = vs.x + x2d;
        v->pos[1] = vs.y + y2d;
        v->pos[2] = vs.z;
        v->rhw = vs.w;
        v->color = vs.color;
        v->specular = PackedColor(0xff000000);
        v->t0.u = vs.u;
        v->t0.v = vs.v;
    }

    SwitchRenderMode(RM2DTris);
    BeginScreenPass();
    AddVertices(gv, n); // note: may flush all queues
    QueuePrepareTriangle(mip, specFlags);
    Queue2DPoly(gv, n);
}

void EngineGL33::DrawPoly(const MipInfo& mip, const Vertex2DAbs* vertices, int n, const Rect2DAbs& clipRect,
                          int specFlags)
{
    if (_resetNeeded)
    {
        return;
    }

    const int maxN = 32;

    // reject poly if fully outside or invalid
    ClipFlags orClip = 0;
    ClipFlags andClip = ClipAll;
    ClipFlags clipV[maxN];
    for (int i = 0; i < n; i++)
    {
        const Vertex2DAbs& vs = vertices[i];
        float x = vs.x;
        float y = vs.y;
        ClipFlags clip = 0;
        if (x < clipRect.x)
        {
            clip |= ClipLeft;
        }
        else if (x > clipRect.x + clipRect.w)
        {
            clip |= ClipRight;
        }
        if (y < clipRect.y)
        {
            clip |= ClipTop;
        }
        else if (y > clipRect.y + clipRect.h)
        {
            clip |= ClipBottom;
        }
        clipV[i] = clip;
        orClip |= clip;
        andClip &= clip;
    }
    if (andClip)
    {
        return;
    }
    Vertex2DAbs clippedVertices1[maxN]; // temporay buffer to keep clipped result
    Vertex2DAbs clippedVertices2[maxN]; // temporay buffer to keep clipped result
    // 2D clipping (with orClip flags)
    if (orClip)
    {
        Vertex2DAbs* free = clippedVertices1;
        Vertex2DAbs* used = clippedVertices2;
        // perform clipping
        for (int i = 0; i < n; i++)
        {
            used[i] = vertices[i];
        }
        if (orClip & ClipTop)
        {
            n = Clip2D(clipRect, free, used, n, InsideTopAbs), swap(free, used);
        }
        if (orClip & ClipBottom)
        {
            n = Clip2D(clipRect, free, used, n, InsideBottomAbs), swap(free, used);
        }
        if (orClip & ClipLeft)
        {
            n = Clip2D(clipRect, free, used, n, InsideLeftAbs), swap(free, used);
        }
        if (orClip & ClipRight)
        {
            n = Clip2D(clipRect, free, used, n, InsideRightAbs), swap(free, used);
        }
        // use result
        if (n < 3)
        {
            return; // nothing to draw
        }
        vertices = used;
    }

    if (n > maxN)
    {
        n = maxN;
        Fail("Poly: Too much vertices");
    }

    TLVertex gv[maxN];

    for (int i = 0; i < n; i++)
    {
        TLVertex* v = &gv[i];
        const Vertex2DAbs& vs = vertices[i];

        v->pos[0] = vs.x;
        v->pos[1] = vs.y;
        v->pos[2] = vs.z;
        v->rhw = vs.w;
        v->color = vs.color;
        v->specular = PackedColor(0xff000000);
        v->t0.u = vs.u;
        v->t0.v = vs.v;
    }

    SwitchRenderMode(RM2DTris);
    BeginScreenPass();
    AddVertices(gv, n); // note: may flush all queues
    QueuePrepareTriangle(mip, specFlags);
    Queue2DPoly(gv, n);
}

void EngineGL33::DrawLine(int beg, int end)
{
    const TLVertex& v0 = _mesh->GetVertex(beg);
    const TLVertex& v1 = _mesh->GetVertex(end);

    float x0 = v0.pos.X();
    float y0 = v0.pos.Y();
    float x1 = v1.pos.X();
    float y1 = v1.pos.Y();

    float z0 = v0.pos.Z();
    float z1 = v1.pos.Z();
    float w0 = v0.rhw;
    float w1 = v1.rhw;

    // use line texture
    Texture* tex = GPreloadedTextures.New(TextureLine);
    const MipInfo& mip = TextBank()->UseMipmap(tex, 1, 1);

    // convert line to poly;
    int specFlags = NoZWrite | IsAlpha | ClampU | ClampV | IsAlphaFog;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dSize2 = dx * dx + dy * dy;
    float invDSize = dSize2 > 0 ? InvSqrt(dSize2) : 1;

    float dSize = dSize2 * invDSize;

    // direction perpendicular dx, dy
    // 2D line drawing
    float pdx = +dy * invDSize, pdy = -dx * invDSize;
    float w = 3.0f;
    x0 -= pdx * (w * 0.5);
    x1 -= pdx * (w * 0.5);
    y0 -= pdy * (w * 0.5);
    y1 -= pdy * (w * 0.5);
    float x0Side = x0 + pdx * w, y0Side = y0 + pdy * w;
    float x1Side = x1 + pdx * w, y1Side = y1 + pdy * w;

    Vertex2DAbs vertices[4];
    float off = 0.0f;
    vertices[0].x = x0 - off;
    vertices[0].y = y0 - off;
    vertices[0].z = z0;
    vertices[0].w = w0;
    vertices[0].u = 0;
    vertices[0].v = 0.25;
    vertices[0].color = v0.color;

    vertices[1].x = x0Side - off;
    vertices[1].y = y0Side - off;
    vertices[1].z = z0;
    vertices[1].w = w0;
    vertices[1].u = 0;
    vertices[1].v = 1;
    vertices[1].color = v0.color;

    vertices[2].x = x1Side - off;
    vertices[2].y = y1Side - off;
    vertices[2].z = z1;
    vertices[2].w = w1;
    vertices[2].u = dSize;
    vertices[2].v = 1;
    vertices[2].color = v1.color;

    vertices[3].x = x1 - off;
    vertices[3].y = y1 - off;
    vertices[3].z = z1;
    vertices[3].w = w1;
    vertices[3].u = dSize;
    vertices[3].v = 0.25;
    vertices[3].color = v1.color;

    Rect2DAbs clip(0, 0, _w, _h);

    DrawPoly(mip, vertices, 4, clip, specFlags);
}
