
#include <Poseidon/UI/Controls/UIControlsExtShared.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/World/World.hpp>
#include <limits.h>
#include <stdio.h>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

using namespace Poseidon;
CHTML::CHTML(ControlsContainer* parent, int idc, const ParamEntry& cls)
    : Control(parent, CT_HTML, idc, cls), CHTMLContainer(cls)
{
    if (_filename.GetLength() > 0)
    {
        Load(_filename);
    }
}

void CHTML::OnLButtonDown(float x, float y)
{
    if (!IsInside(x, y))
    {
        return;
    }

    if (_sections.Size() <= 0)
    {
        return;
    }

    // A click should act on the link under the cursor. Normally OnMouseMove has
    // already latched _activeField for the hovered field, but a click can arrive
    // without that hover having run for this control (e.g. the in-mission map's
    // notes, where the per-frame hover dispatch may not have latched the field) —
    // leaving _activeField at -1 and the link dead. Resolve the field
    // from the actual click position so the click never depends on a prior hover.
    int activeField = FindField(x, y);
    if (activeField < 0)
    {
        activeField = _activeField;
    }
    if (activeField < 0)
    {
        return;
    }

    HTMLSection& section = _sections[_currentSection];
    HTMLField& field = section.fields[activeField];

    const char* text = field.href;
    if (*text == 0)
    {
        return;
    }

    if (*text == '#')
    {
        SwitchSection(text + 1);
        return;
    }
    else
    {
        if (_parent)
        {
            _parent->OnHTMLLink(IDC(), field.href);
        }
    }
}

void CHTML::OnMouseMove(float x, float y, bool active)
{
    if (active)
    {
        _activeField = FindField(x, y);
    }
    else
    {
        _activeField = -1;
    }
}

RString CHTML::ActivateHRef(const char* href)
{
    for (int s = 0; s < _sections.Size(); s++)
    {
        HTMLSection& sec = _sections[s];
        for (int f = 0; f < sec.fields.Size(); f++)
        {
            HTMLField& fld = sec.fields[f];
            if (fld.href.GetLength() == 0)
            {
                continue;
            }
            const char* fh = fld.href;
            // tolerate a leading '#' on either side (parsed hrefs keep it)
            const char* a = (*fh == '#') ? fh + 1 : fh;
            const char* b = (href && *href == '#') ? href + 1 : href;
            if (!b || stricmp(a, b) != 0)
            {
                continue;
            }
            // Replay CHTML::OnLButtonDown's action for this link.
            if (*fld.href == '#')
            {
                SwitchSection((const char*)fld.href + 1);
            }
            else if (_parent)
            {
                _parent->OnHTMLLink(IDC(), fld.href);
            }
            RString secName;
            int imgCount = 0;
            if (_currentSection >= 0 && _currentSection < _sections.Size())
            {
                HTMLSection& cur = _sections[_currentSection];
                if (cur.names.Size() > 0)
                    secName = cur.names[0];
                for (int g = 0; g < cur.fields.Size(); g++)
                    if (cur.fields[g].texture1.NotNull())
                        imgCount++;
            }
            char buf[256];
            snprintf(buf, sizeof(buf), "OK:section=%s,img=%d", (const char*)secName, imgCount);
            return RString(buf);
        }
    }
    return RString("FAIL:no_link");
}

bool CHTML::FindLinkScreenPos(const char* href, float& outX, float& outY)
{
    int linkSec = -1;
    int linkFld = -1;
    for (int s = 0; s < _sections.Size() && linkSec < 0; s++)
    {
        HTMLSection& sec = _sections[s];
        for (int f = 0; f < sec.fields.Size(); f++)
        {
            const char* fh = sec.fields[f].href;
            if (!fh || !*fh)
            {
                continue;
            }
            const char* a = (*fh == '#') ? fh + 1 : fh;
            const char* b = (href && *href == '#') ? href + 1 : href;
            if (b && stricmp(a, b) == 0)
            {
                linkSec = s;
                linkFld = f;
                break;
            }
        }
    }
    if (linkSec < 0)
    {
        return false;
    }
    // Make the link's section current so the real hit-test (FindField, which only
    // looks at _currentSection) can see it, then scan for the FIRST screen point
    // that maps to the link. A long link wraps across rows, so its bounding box is
    // L-shaped and the box centre can fall off the link — the first hit is always
    // on it, which is what a click target needs. Centre the point within the row
    // the first hit lands on so it is comfortably inside the glyph run.
    _currentSection = linkSec;
    const float step = 0.004f;
    for (float sy = _y; sy <= _y + _h; sy += step)
    {
        bool rowHit = false;
        float rowMinX = 0, rowMaxX = 0;
        for (float sx = _x; sx <= _x + _w; sx += step)
        {
            if (FindField(sx, sy) != linkFld)
            {
                continue;
            }
            if (!rowHit)
            {
                rowMinX = rowMaxX = sx;
                rowHit = true;
            }
            else
            {
                rowMaxX = sx;
            }
        }
        if (rowHit)
        {
            outX = 0.5f * (rowMinX + rowMaxX);
            outY = sy;
            return true;
        }
    }
    return false;
}

RString CHTML::ProbeClickHRef(const char* href)
{
    float sx = 0, sy = 0;
    if (!FindLinkScreenPos(href, sx, sy))
    {
        // Distinguish "no such link" from "link exists but hit-test never maps a
        // screen point to it" — both matter.
        bool hasLink = false;
        for (int s = 0; s < _sections.Size() && !hasLink; s++)
        {
            HTMLSection& sec = _sections[s];
            for (int f = 0; f < sec.fields.Size(); f++)
            {
                const char* fh = sec.fields[f].href;
                if (!fh || !*fh)
                    continue;
                const char* a = (*fh == '#') ? fh + 1 : fh;
                const char* b = (href && *href == '#') ? href + 1 : href;
                if (b && stricmp(a, b) == 0)
                {
                    hasLink = true;
                    break;
                }
            }
        }
        return RString(hasLink ? "FAIL:findfield_miss" : "FAIL:no_link");
    }
    OnMouseMove(sx, sy);   // real hover -> sets _activeField
    OnLButtonDown(sx, sy); // real click -> activates if the path works
    RString secName;
    if (_currentSection >= 0 && _currentSection < _sections.Size() && _sections[_currentSection].names.Size() > 0)
    {
        secName = _sections[_currentSection].names[0];
    }
    char buf[200];
    snprintf(buf, sizeof(buf), "OK:hit=%.3f,%.3f,section=%s", sx, sy, (const char*)secName);
    return RString(buf);
}

#define DRAW_TABLE_BORDERS 0

void CHTML::OnDraw(float alpha)
{
    const int w = GLOB_ENGINE->Width2D();
    const int h = GLOB_ENGINE->Height2D();

    float xx = toInt(_x * w) + 0.5;
    float yy = toInt(_y * h) + 0.5;
    float ww = toInt(_w * w);
    float hh = toInt(_h * h);

    PackedColor pictureColor = PackedColor(Color(1, 1, 1, 0.6 * alpha));
    PackedColor pictureColorSelected = PackedColor(Color(1, 1, 1, alpha));
    PackedColor bgColor = ModAlpha(_bgColor, alpha);

    MipInfo mip = GLOB_ENGINE->TextBank()->UseMipmap(nullptr, 0, 0);
    GLOB_ENGINE->Draw2D(mip, bgColor, Rect2DPixel(xx, yy, ww, hh));

    if (_sections.Size() <= 0)
    {
        return;
    }
    HTMLSection& section = _sections[_currentSection];

    float top = _y;
    bool bottom = false;

    for (int r = 0; r < section.rows.Size(); r++)
    {
        HTMLRow& row = section.rows[r];
        if (!bottom && row.bottom)
        {
            bottom = true;
            top = _y + _h;
            for (int rr = r; rr < section.rows.Size(); rr++)
            {
                top -= _scale * section.rows[rr].height;
            }
        }
        float left;
        switch (row.align)
        {
            case HARight:
                left = _x + _w - _scale * (row.width + textBorder);
                break;
            case HACenter:
                left = _x + 0.5 * (_w - _scale * row.width);
                break;
            default:
                PoseidonAssert(row.align == HALeft);
                left = _x + _scale * textBorder;
                break;
        }
        for (int f = row.firstField; f <= row.lastField; f++)
        {
            if (f >= section.fields.Size())
            {
                continue;
            }
            HTMLField& field = section.fields[f];
            if (field.nextline)
            {
                continue;
            }
            int from = 0;
            if (f == row.firstField)
            {
                from = row.firstPos;
            }
            int to = INT_MAX;
            if (f == row.lastField)
            {
                to = row.lastPos;
            }
            if (from >= to)
            {
                continue;
            }

            if (field.format == HFImg)
            {
                float l = left;
                if (field.tableWidth > 0)
                {
                    switch (field.align)
                    {
                        case HARight:
                            l += _scale * (field.tableWidth - field.width);
                            break;
                        case HACenter:
                            l += 0.5 * _scale * (field.tableWidth - field.width);
                            break;
                    }
                }
                if (field.text.GetLength() > 0)
                {
                    float t;
                    if (row.height > 0)
                    {
                        t = top + _scale * (row.height - field.height - _sizeP);
                    }
                    else
                    {
                        t = top - _scale * _sizeP;
                    }
                    float lText =
                        l + 0.5 * _scale * (field.width + field.indent - GetTextWidth(_sizeP, _fontP, field.text));
                    PackedColor color;
                    if (field.href.GetLength() == 0)
                    {
                        color = _textColor;
                    }
                    else if (f == _activeField)
                    {
                        color = _activeLinkColor;
                    }
                    else
                    {
                        color = _linkColor;
                    }
                    GEngine->DrawText(Point2DFloat(lText, t), _scale * _sizeP, Rect2DFloat(_x, _y, _w, _h), _fontP,
                                      color, field.text);
                }
                Texture* texture = field.GetTexture();
                if (texture)
                {
                    float t;
                    if (row.height > 0)
                    {
                        t = top + _scale * (row.height - field.height);
                    }
                    else
                    {
                        t = top;
                    }
                    Draw2DPars pars;
                    pars.mip = GEngine->TextBank()->UseMipmap(texture, 0, 0);
                    pars.spec = NoZBuf | IsAlpha | ClampU | ClampV | IsAlphaFog;
                    PackedColor color;
                    if (field.href.GetLength() == 0)
                    {
                        color = pictureColorSelected;
                    }
                    else if (f == _activeField)
                    {
                        color = pictureColorSelected;
                    }
                    else
                    {
                        color = pictureColor;
                    }
                    pars.SetColor(color);
                    pars.SetU(0, 1);
                    pars.SetV(0, 1);
                    Rect2DPixel rect((l + _scale * field.indent) * w, t * h, _scale * field.width * w,
                                     _scale * field.height * h);
                    Rect2DPixel clip(_x * w, _y * h, _w * w, _h * h);
                    GEngine->Draw2D(pars, rect, clip);
                }
                if (field.tableWidth > 0)
                {
#if DRAW_TABLE_BORDERS
                    GEngine->DrawLine(CX(left), CY(top), CX(left + _scale * field.tableWidth), CY(top), PackedBlack,
                                      PackedBlack);
                    GEngine->DrawLine(CX(left + _scale * field.tableWidth), CY(top),
                                      CX(left + _scale * field.tableWidth), CY(top + _scale * row.height), PackedBlack,
                                      PackedBlack);
                    GEngine->DrawLine(CX(left + _scale * field.tableWidth), CY(top + _scale * row.height), CX(left),
                                      CY(top + _scale * row.height), PackedBlack, PackedBlack);
                    GEngine->DrawLine(CX(left), CY(top + _scale * row.height), CX(left), CY(top), PackedBlack,
                                      PackedBlack);
#endif
                    left += _scale * field.tableWidth;
                }
                else
                {
                    left += _scale * field.width;
                }
            }
            else
            {
                Font* font = _fontP;
                float size = _scale * _sizeP;
                switch (field.format)
                {
                    case HFP:
                        if (field.bold)
                        {
                            font = _fontPBold;
                        }
                        else
                        {
                            font = _fontP;
                        }
                        size = _scale * _sizeP;
                        break;
                    case HFH1:
                        if (field.bold)
                        {
                            font = _fontH1Bold;
                        }
                        else
                        {
                            font = _fontH1;
                        }
                        size = _scale * _sizeH1;
                        break;
                    case HFH2:
                        if (field.bold)
                        {
                            font = _fontH2Bold;
                        }
                        else
                        {
                            font = _fontH2;
                        }
                        size = _scale * _sizeH2;
                        break;
                    case HFH3:
                        if (field.bold)
                        {
                            font = _fontH3Bold;
                        }
                        else
                        {
                            font = _fontH3;
                        }
                        size = _scale * _sizeH3;
                        break;
                    case HFH4:
                        if (field.bold)
                        {
                            font = _fontH4Bold;
                        }
                        else
                        {
                            font = _fontH4;
                        }
                        size = _scale * _sizeH4;
                        break;
                    case HFH5:
                        if (field.bold)
                        {
                            font = _fontH5Bold;
                        }
                        else
                        {
                            font = _fontH5;
                        }
                        size = _scale * _sizeH5;
                        break;
                    case HFH6:
                        if (field.bold)
                        {
                            font = _fontH6Bold;
                        }
                        else
                        {
                            font = _fontH6;
                        }
                        size = _scale * _sizeH6;
                        break;
                    default:
                        Fail("Format");
                        break;
                }
                PackedColor color;
                if (field.href.GetLength() == 0)
                {
                    if (field.bold)
                    {
                        color = _boldColor;
                    }
                    else
                    {
                        color = _textColor;
                    }
                }
                else if (f == _activeField)
                {
                    color = _activeLinkColor;
                }
                else
                {
                    color = _linkColor;
                }

                RString text = field.text.Substring(from, to);
                float l = left;
                if (field.tableWidth > 0)
                {
                    switch (field.align)
                    {
                        case HARight:
                            l += (_scale * field.tableWidth - GetTextWidth(size, font, text));
                            break;
                        case HACenter:
                            l += 0.5 * (_scale * field.tableWidth - GetTextWidth(size, font, text));
                            break;
                    }
                }
                float t = top + _scale * row.height - size;
                GLOB_ENGINE->DrawText(Point2DFloat(l + _scale * field.indent, t), size, Rect2DFloat(_x, _y, _w, _h),
                                      font, color, text);
                if (field.tableWidth > 0)
                {
#if DRAW_TABLE_BORDERS
                    GEngine->DrawLine(CX(left), CY(top), CX(left + _scale * field.tableWidth), CY(top), PackedBlack,
                                      PackedBlack);
                    GEngine->DrawLine(CX(left + _scale * field.tableWidth), CY(top),
                                      CX(left + _scale * field.tableWidth), CY(top + _scale * row.height), PackedBlack,
                                      PackedBlack);
                    GEngine->DrawLine(CX(left + _scale * field.tableWidth), CY(top + _scale * row.height), CX(left),
                                      CY(top + _scale * row.height), PackedBlack, PackedBlack);
                    GEngine->DrawLine(CX(left), CY(top + _scale * row.height), CX(left), CY(top), PackedBlack,
                                      PackedBlack);
#endif
                    left += _scale * field.tableWidth;
                }
                else
                {
                    left += GetTextWidth(size, font, text);
                }
            }
        }
        top += _scale * row.height;
    }
}

int CHTML::FindField(float x, float y)
{
    if (!IsInside(x, y))
    {
        return -1;
    }

    return CHTMLContainer::FindField((x - _x) / _scale - textBorder, (y - _y) / _scale);
}

float CHTML::GetPageWidth() const
{
    return (_w - 2 * textBorder) / _scale;
}

float CHTML::GetPageHeight() const
{
    return _h / _scale;
}

float CHTML::GetTextWidth(float size, Font* font, const char* text) const
{
    return GEngine->GetTextWidth(size, font, text);
}

C3DHTML::C3DHTML(ControlsContainer* parent, int idc, const ParamEntry& cls)
    : Control3D(parent, CT_3DHTML, idc, cls), CHTMLContainer(cls)
{
    _width = cls >> "size";
    _invWidth = _width != 0 ? 1.0 / _width : 0;
    _height = -1;
    _invHeight = -1;
}

void C3DHTML::UpdateInfo(ControlObject* object, ControlInObject& info)
{
    Control3D::UpdateInfo(object, info);
    if (_height < 0)
    {
        // first UpdateInfo
        _height = _width * _down.Size() / _right.Size();
        _invHeight = _height != 0 ? 1.0 / _height : 0;
        // _width, _height must be initialized
        if (_filename.GetLength() > 0)
        {
            Load(_filename);
        }
    }
}

void C3DHTML::OnLButtonDown(float x, float y)
{
    if (!IsInside(x, y))
    {
        return;
    }

    if (_sections.Size() <= 0)
    {
        return;
    }

    // Resolve the link from the click position (FindField reads the mesh UV under
    // the cursor) rather than depending on a prior hover having latched it, so a
    // click never drops because _activeField is stale (briefing book).
    int activeField = FindField(x, y);
    if (activeField < 0)
    {
        activeField = _activeField;
    }
    if (activeField < 0)
    {
        return;
    }

    HTMLSection& section = _sections[_currentSection];
    HTMLField& field = section.fields[activeField];

    const char* text = field.href;
    if (*text == 0)
    {
        return;
    }

    if (*text == '#')
    {
        SwitchSection(text + 1);
        return;
    }
    else
    {
        if (_parent)
        {
            _parent->OnHTMLLink(IDC(), field.href);
        }
    }
}

void C3DHTML::OnMouseMove(float x, float y, bool active)
{
    if (active)
    {
        _activeField = FindField(x, y);
    }
    else
    {
        _activeField = -1;
    }
}

void C3DHTML::OnDraw(float alpha)
{
    PackedColor bgColor = ModAlpha(_bgColor, alpha);
    GEngine->Draw3D(_position, _down, _right, ClipAll, bgColor, DisableSun, nullptr);

    if (_sections.Size() <= 0)
    {
        return;
    }
    HTMLSection& section = _sections[_currentSection];

    Vector3 posTop = _position;
    Vector3 right = _invWidth * _right;
    Vector3 down = _invHeight * _down;

    PackedColor pictureColor = PackedColor(Color(1, 1, 1, 0.6 * alpha));
    PackedColor pictureColorSelected = PackedColor(Color(1, 1, 1, alpha));

    bool bottom = false;

    for (int r = 0; r < section.rows.Size(); r++)
    {
        HTMLRow& row = section.rows[r];
        if (!bottom && row.bottom)
        {
            bottom = true;
            posTop = _position + _down;
            for (int rr = r; rr < section.rows.Size(); rr++)
            {
                posTop -= section.rows[rr].height * down;
            }
        }
        Vector3 posLeft;
        switch (row.align)
        {
            case HARight:
                posLeft = posTop + (_width - row.width) * right;
                break;
            case HACenter:
                posLeft = posTop + 0.5 * (_width - row.width) * right;
                break;
            default:
                PoseidonAssert(row.align == HALeft);
                posLeft = posTop;
                break;
        }
        for (int f = row.firstField; f <= row.lastField; f++)
        {
            if (f >= section.fields.Size())
            {
                continue;
            }
            HTMLField& field = section.fields[f];
            if (field.nextline)
            {
                continue;
            }
            int from = 0;
            if (f == row.firstField)
            {
                from = row.firstPos;
            }
            int to = INT_MAX;
            if (f == row.lastField)
            {
                to = row.lastPos;
            }
            if (from >= to)
            {
                continue;
            }

            if (field.format == HFImg)
            {
                Vector3 l = posLeft;
                if (field.tableWidth > 0)
                {
                    switch (field.align)
                    {
                        case HARight:
                            l += (field.tableWidth - field.width) * right;
                            break;
                        case HACenter:
                            l += 0.5 * (field.tableWidth - field.width) * right;
                            break;
                    }
                }
                Texture* texture = field.GetTexture();
                if (texture)
                {
                    PackedColor color;
                    if (field.href.GetLength() == 0)
                    {
                        color = pictureColorSelected;
                    }
                    else if (f == _activeField)
                    {
                        color = pictureColorSelected;
                    }
                    else
                    {
                        color = pictureColor;
                    }
                    Vector3 p;
                    if (row.height > 0)
                    {
                        p = l + (row.height - field.height) * down + field.indent * right;
                    }
                    else
                    {
                        p = l;
                    }
                    Vector3 r = (field.width) * right;
                    Vector3 d = field.height * down;
                    GEngine->Draw3D(p, d, r, ClipAll, color, DisableSun, texture);
                }
                if (field.tableWidth > 0)
                {
#if DRAW_TABLE_BORDERS
                    Vector3& tl = posLeft;
                    Vector3 tr = posLeft + field.tableWidth * right;
                    Vector3 bl = posLeft + row.height * down;
                    Vector3 br = posLeft + row.height * down + field.tableWidth * right;
                    GEngine->DrawLine3D(tl, tr, PackedBlack, DisableSun);
                    GEngine->DrawLine3D(tr, br, PackedBlack, DisableSun);
                    GEngine->DrawLine3D(br, bl, PackedBlack, DisableSun);
                    GEngine->DrawLine3D(bl, tl, PackedBlack, DisableSun);
#endif
                    posLeft += field.tableWidth * right;
                }
                else
                {
                    posLeft += field.width * right;
                }
            }
            else
            {
                Font* font = _fontP;
                float size = _sizeP;
                switch (field.format)
                {
                    case HFP:
                        if (field.bold)
                        {
                            font = _fontPBold;
                        }
                        else
                        {
                            font = _fontP;
                        }
                        size = _sizeP;
                        break;
                    case HFH1:
                        if (field.bold)
                        {
                            font = _fontH1Bold;
                        }
                        else
                        {
                            font = _fontH1;
                        }
                        size = _sizeH1;
                        break;
                    case HFH2:
                        if (field.bold)
                        {
                            font = _fontH2Bold;
                        }
                        else
                        {
                            font = _fontH2;
                        }
                        size = _sizeH2;
                        break;
                    case HFH3:
                        if (field.bold)
                        {
                            font = _fontH3Bold;
                        }
                        else
                        {
                            font = _fontH3;
                        }
                        size = _sizeH3;
                        break;
                    case HFH4:
                        if (field.bold)
                        {
                            font = _fontH4Bold;
                        }
                        else
                        {
                            font = _fontH4;
                        }
                        size = _sizeH4;
                        break;
                    case HFH5:
                        if (field.bold)
                        {
                            font = _fontH5Bold;
                        }
                        else
                        {
                            font = _fontH5;
                        }
                        size = _sizeH5;
                        break;
                    case HFH6:
                        if (field.bold)
                        {
                            font = _fontH6Bold;
                        }
                        else
                        {
                            font = _fontH6;
                        }
                        size = _sizeH6;
                        break;
                    default:
                        Fail("Format");
                        break;
                }
                PackedColor color;
                if (field.href.GetLength() == 0)
                {
                    if (field.bold)
                    {
                        color = _boldColor;
                    }
                    else
                    {
                        color = _textColor;
                    }
                }
                else if (f == _activeField)
                {
                    color = _activeLinkColor;
                }
                else
                {
                    color = _linkColor;
                }

                RString text = field.text.Substring(from, to);
                Vector3 l = posLeft;
                if (field.tableWidth > 0)
                {
                    switch (field.align)
                    {
                        case HARight:
                            l += (field.tableWidth - GetTextWidth(size, font, text)) * right;
                            break;
                        case HACenter:
                            l += 0.5 * (field.tableWidth - GetTextWidth(size, font, text)) * right;
                            break;
                    }
                }
                Vector3 p = l + (row.height - size) * down + field.indent * right;
                Vector3 u = -size * down;
                Vector3 r = 0.75 * u.Size() * right.Normalized();
                GEngine->DrawText3D(p, u, r, ClipAll, font, color, DisableSun, text);
                if (field.tableWidth > 0)
                {
#if DRAW_TABLE_BORDERS
                    Vector3& tl = posLeft;
                    Vector3 tr = posLeft + field.tableWidth * right;
                    Vector3 bl = posLeft + row.height * down;
                    Vector3 br = posLeft + row.height * down + field.tableWidth * right;
                    GEngine->DrawLine3D(tl, tr, PackedBlack, DisableSun);
                    GEngine->DrawLine3D(tr, br, PackedBlack, DisableSun);
                    GEngine->DrawLine3D(br, bl, PackedBlack, DisableSun);
                    GEngine->DrawLine3D(bl, tl, PackedBlack, DisableSun);
#endif
                    posLeft += field.tableWidth * right;
                }
                else
                {
                    posLeft += GetTextWidth(size, font, text) * right;
                }
            }
        }
        posTop += row.height * down;
    }
}

int C3DHTML::FindField(float x, float y)
{
    if (!IsInside(x, y))
    {
        return -1;
    }

    return CHTMLContainer::FindField(_u * _width, _v * _height);
}

float C3DHTML::GetTextWidth(float size, Font* font, const char* text) const
{
    Vector3 dir = 0.75 * size * _right.Normalized();
    return GEngine->GetText3DWidth(dir, font, text).Size();
}
