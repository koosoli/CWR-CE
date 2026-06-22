#include <Poseidon/World/Entities/Infantry/Wounds.hpp>
#include <Poseidon/World/Simulation/Animation/Animation.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <stdio.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

void WoundInfo::Register(LODShape* shape)
{
    for (int i = 0; i < Size(); i++)
    {
        // register textures with shape
        Texture* oldTex = Get(i)._healthy;
        Texture* newTex = Get(i)._wounded;
        for (int l = 0; l < shape->NLevels(); l++)
        {
            Shape* level = shape->Level(l);
            level->RegisterTexture(newTex, oldTex);
        }
    }
}

void WoundInfo::Load(LODShape* shape, const ParamEntry& cfg)
{
    if (cfg.GetSize() & 1)
    {
        RptF("Array %s not even", (const char*)cfg.GetContext());
    }
    for (int i = 0; i < cfg.GetSize() - 1; i += 2)
    {
        RString oldName = GetDefaultName(cfg[i + 0], "data\\", ".pac");
        RString newName = GetDefaultName(cfg[i + 1], "data\\", ".pac");
        // search shape for texture
        oldName.Lower();
        Texture* oldTex = shape->FindTexture(oldName);
        if (oldTex)
        {
            newName.Lower();
            Ref<Texture> newTex = GlobLoadTexture(newName);
            WoundPair wp;
            wp._healthy = oldTex;
            wp._wounded = newTex;
            Add(wp);
        }
    }
}

void WoundInfo::LoadAndRegister(LODShape* shape, const ParamEntry& cfg)
{
    Load(shape, cfg);
    Register(shape);
}

void WoundInfo::Unload()
{
    Clear();
}

WoundTextureSelection::WoundTextureSelection() = default;

void WoundTextureSelection::Init(Shape* shape, const Offset* offsets, int nOffsets, Texture* tex, Texture* oldTex)
{
    _tex = tex;
    _oldTex = oldTex;
    _faces.Init(offsets, nOffsets);
    /**/
    _faces.SetNeedsSections(true);
    char name[512];
    snprintf(name, sizeof(name), "Wound %s->%s", tex ? tex->Name() : "<nullptr>",
             oldTex ? oldTex->Name() : "<nullptr>");
    _faces.RescanSections(shape, name);
    /**/
}

WoundTextureSelections::WoundTextureSelections() = default;

void WoundTextureSelections::Init(LODShape* shape, const WoundInfo& info)
{
    AutoArray<Offset> offsets;
    for (int i = 0; i < shape->NLevels(); i++)
    {
        Shape* level = shape->Level(i);
        for (int t = 0; t < info.Size(); t++)
        {
            const WoundPair& wp = info[t];
            offsets.Resize(0);
            for (Offset f = level->BeginFaces(); f < level->EndFaces(); level->NextFace(f))
            {
                const Poly& face = level->Face(f);
                if (face.GetTexture() == wp._healthy)
                {
                    offsets.Add(f);
                }
            }
            if (offsets.Size() > 0)
            {
                WoundTextureSelection& ws = _selection[i].Append();
                ws.Init(level, offsets.Data(), offsets.Size(), wp._wounded, wp._healthy);
            }
        }
        _selection[i].Compact();
    }
}

void WoundTextureSelections::Init(LODShape* shape, const Animation& anim, const WoundInfo& info)
{
    AutoArray<Offset> offsets;
    for (int i = 0; i < shape->NLevels(); i++)
    {
        int selI = anim.GetSelection(i);
        if (selI < 0)
        {
            continue;
        }
        Shape* level = shape->Level(i);
        const NamedSelection& sel = level->NamedSel(selI);
        const FaceSelection& faces = sel.FaceOffsets(level);
        for (int t = 0; t < info.Size(); t++)
        {
            const WoundPair& wp = info[t];
            offsets.Resize(0);
            for (int f = 0; f < faces.Size(); f++)
            {
                const Poly& face = level->Face(faces[f]);
                if (face.GetTexture() == wp._healthy)
                {
                    offsets.Add(faces[f]);
                }
            }
            if (offsets.Size() > 0)
            {
                WoundTextureSelection& ws = _selection[i].Append();
                ws.Init(level, offsets.Data(), offsets.Size(), wp._wounded, wp._healthy);
            }
        }
        _selection[i].Compact();
    }
}

void WoundTextureSelections::Init(LODShape* shape, const WoundInfo& info, const char* name, const char* altName)
{
    if (name)
    {
        Animation anim;
        anim.Init(shape, name, altName);
        Init(shape, anim, info);
    }
    else
    {
        Init(shape, info);
    }
}

void WoundTextureSelections::Init(LODShape* shape, const ParamEntry& cfg, const char* name, const char* altName)
{
    WoundInfo info;
    info.LoadAndRegister(shape, cfg);
    Init(shape, info, name, altName);
}

void WoundTextureSelections::Unload()
{
    for (int i = 0; i < MAX_LOD_LEVELS; i++)
    {
        _selection[i].Clear();
    }
}

void WoundTextureSelection::SetTexture(Shape* shape, Texture* tex) const
{
    for (int f = 0; f < _faces.Size(); f++)
    {
        Poly& face = shape->Face(_faces[f]);
        face.SetTexture(tex);
    }
    for (int f = 0; f < _faces.NSections(); f++)
    {
        ShapeSection& sec = shape->GetSection(_faces.GetSection(f));
        sec.properties.SetTexture(tex);
    }
}

void WoundTextureSelection::Apply(Shape* shape) const
{
    SetTexture(shape, _tex);
}

void WoundTextureSelection::Apply(Shape* shape, Texture* tex) const
{
    SetTexture(shape, tex);
}

void WoundTextureSelection::Restore(Shape* shape) const
{
    SetTexture(shape, _oldTex);
}

void WoundTextureSelections::Apply(LODShape* shape, int level) const
{
    const WoundTextureSelectionArray& array = _selection[level];
    Shape* lShape = shape->Level(level);
    for (int a = 0; a < array.Size(); a++)
    {
        const WoundTextureSelection& sel = array[a];
        sel.Apply(lShape);
    }
}

void WoundTextureSelections::ApplyModified(LODShape* shape, int level, Texture* orig, Texture* origWounded) const
{
    const WoundTextureSelectionArray& array = _selection[level];
    Shape* lShape = shape->Level(level);
    for (int a = 0; a < array.Size(); a++)
    {
        const WoundTextureSelection& sel = array[a];
        if (sel.GetOrigTexture() != orig || !origWounded)
        {
            sel.Apply(lShape);
        }
        else
        {
            sel.Apply(lShape, origWounded);
        }
    }
}

void WoundTextureSelections::Restore(LODShape* shape, int level) const
{
    const WoundTextureSelectionArray& array = _selection[level];
    Shape* lShape = shape->Level(level);
    for (int a = 0; a < array.Size(); a++)
    {
        const WoundTextureSelection& sel = array[a];
        sel.Restore(lShape);
    }
}

} // namespace Poseidon
