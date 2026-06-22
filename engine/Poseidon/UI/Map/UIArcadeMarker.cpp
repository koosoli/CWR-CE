#include <Poseidon/UI/Map/UIMap.hpp>
#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/UI/Locale/StringtableExt.hpp>
#include <Poseidon/World/Scene/Camera/CamEffects.hpp>
#include <Poseidon/Game/TitEffects.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{

Control* DisplayArcadeMarker::OnCreateCtrl(int type, int idc, const ParamEntry& cls)
{
    switch (idc)
    {
        case IDC_ARCMARK_TITLE:
        {
            CStatic* text = new CStatic(this, idc, cls);
            if (_index < 0)
            {
                text->SetText(LocalizeString(IDS_ARCMARK_TITLE1));
            }
            else
            {
                text->SetText(LocalizeString(IDS_ARCMARK_TITLE2));
            }
            return text;
        }
        case IDC_ARCMARK_NAME:
        {
            CEdit* edit = new CEdit(this, idc, cls);
            edit->SetText(_marker.name);
            return edit;
        }
        case IDC_ARCMARK_TEXT:
        {
            CEdit* edit = new CEdit(this, idc, cls);
            edit->SetText(_marker.text);
            return edit;
        }
        case IDC_ARCMARK_MARKER:
        {
            CToolBox* ctrl = new CToolBox(this, idc, cls);
            ctrl->SetCurSel(_marker.markerType);
            return ctrl;
        }
        case IDC_ARCMARK_TYPE:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            const ParamEntry& cls = Pars >> "CfgMarkers";
            int n = cls.GetEntryCount();
            int sel = 0;
            for (int i = 0; i < n; i++)
            {
                const ParamEntry& entry = cls.GetEntry(i);
                int index = combo->AddString(entry >> "name");
                combo->SetData(index, entry.GetName());
                if (stricmp(_marker.type, entry.GetName()) == 0)
                {
                    sel = index;
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCMARK_COLOR:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            const ParamEntry& cls = Pars >> "CfgMarkerColors";
            int n = cls.GetEntryCount();
            int sel = 0;
            for (int i = 0; i < n; i++)
            {
                const ParamEntry& entry = cls.GetEntry(i);
                int index = combo->AddString(entry >> "name");
                combo->SetData(index, entry.GetName());
                if (stricmp(_marker.colorName, entry.GetName()) == 0)
                {
                    sel = index;
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCMARK_FILL:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            const ParamEntry& cls = Pars >> "CfgMarkerBrushes";
            int n = cls.GetEntryCount();
            int sel = 0;
            for (int i = 0; i < n; i++)
            {
                const ParamEntry& entry = cls.GetEntry(i);
                int index = combo->AddString(entry >> "name");
                combo->SetData(index, entry.GetName());
                if (stricmp(_marker.fillName, entry.GetName()) == 0)
                {
                    sel = index;
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCMARK_A:
        {
            CEdit* edit = new CEdit(this, idc, cls);
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "%g", _marker.a);
            edit->SetText(buffer);
            return edit;
        }
        case IDC_ARCMARK_B:
        {
            CEdit* edit = new CEdit(this, idc, cls);
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "%g", _marker.b);
            edit->SetText(buffer);
            return edit;
        }
        case IDC_ARCMARK_ANGLE:
        {
            CEdit* edit = new CEdit(this, idc, cls);
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "%g", _marker.angle);
            edit->SetText(buffer);
            return edit;
        }
    }

    return Display::OnCreateCtrl(type, idc, cls);
}

bool DisplayArcadeMarker::CanDestroy()
{
    if (!Display::CanDestroy())
    {
        return false;
    }

    if (_exit == IDC_OK)
    {
        PoseidonAssert(dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_NAME)));
        CEdit* edit = static_cast<CEdit*>(GetCtrl(IDC_ARCMARK_NAME));
        if (edit)
        {
            RString name = edit->GetText();
            if (name.GetLength() == 0)
            {
                CreateMsgBox(MB_BUTTON_OK, LocalizeString(IDS_MSG_MARKER_EMPTY));
                return false;
            }
            int n = _template->markers.Size();
            for (int i = 0; i < n; i++)
            {
                if (i == _index)
                {
                    continue;
                }
                if (stricmp(name, _template->markers[i].name) == 0)
                {
                    CreateMsgBox(MB_BUTTON_OK, LocalizeString(IDS_MSG_MARKER_EXIST));
                    return false;
                }
            }
        }
    }

    return true;
}

void DisplayArcadeMarker::Destroy()
{
    Display::Destroy();

    if (_exit != IDC_OK)
    {
        return;
    }

    CEdit* edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_NAME));
    if (edit)
    {
        _marker.name = edit->GetText();
    }

    edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_TEXT));
    if (edit)
    {
        _marker.text = edit->GetText();
    }

    float defSize = 1;

    CToolBox* ctrl = dynamic_cast<CToolBox*>(GetCtrl(IDC_ARCMARK_MARKER));
    if (ctrl)
    {
        _marker.markerType = (MarkerType)ctrl->GetCurSel();
        if (_marker.markerType != MTIcon)
        {
            defSize = 20;
        }
    }

    CCombo* combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCMARK_TYPE));
    if (combo)
    {
        _marker.type = combo->GetData(combo->GetCurSel());
        _marker.OnTypeChanged();
    }

    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCMARK_COLOR));
    if (combo)
    {
        _marker.colorName = combo->GetData(combo->GetCurSel());
        _marker.OnColorChanged();
    }

    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCMARK_FILL));
    if (combo)
    {
        _marker.fillName = combo->GetData(combo->GetCurSel());
        _marker.OnFillChanged();
    }

    edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_A));
    if (edit)
    {
        _marker.a = edit->GetText() ? atof(edit->GetText()) : defSize;
    }

    edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_B));
    if (edit)
    {
        _marker.b = edit->GetText() ? atof(edit->GetText()) : defSize;
    }

    edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_ANGLE));
    if (edit)
    {
        _marker.angle = edit->GetText() ? atof(edit->GetText()) : 0;
    }
}

void DisplayArcadeMarker::OnToolBoxSelChanged(int idc, int curSel)
{
    if (idc == IDC_ARCMARK_MARKER)
    {
        ChangeType(curSel);
    }
    else
    {
        Display::OnToolBoxSelChanged(idc, curSel);
    }
}

void DisplayArcadeMarker::ChangeType(int curSel)
{
    switch (curSel)
    {
        case MTIcon:
        {
            IControl* ctrl = GetCtrl(IDC_ARCMARK_TYPE);
            if (ctrl)
            {
                ctrl->ShowCtrl(true);
            }
            ctrl = GetCtrl(IDC_ARCMARK_FILL);
            if (ctrl)
            {
                ctrl->ShowCtrl(false);
            }
            CStatic* text = dynamic_cast<CStatic*>(GetCtrl(IDC_ARCMARK_TYPE_TEXT));
            if (text)
            {
                text->SetText(LocalizeString(IDS_ARCMARK_TYPE1));
            }
            if (_oldType != MTIcon)
            {
                CEdit* edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_A));
                if (edit)
                {
                    float value = edit->GetText() ? atof(edit->GetText()) : 1;
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "%g", value / 20.0f);
                    edit->SetText(buffer);
                }
                edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_B));
                if (edit)
                {
                    float value = edit->GetText() ? atof(edit->GetText()) : 1;
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "%g", value / 20.0f);
                    edit->SetText(buffer);
                }
            }
        }
        break;
        case MTRectangle:
        case MTEllipse:
        {
            IControl* ctrl = GetCtrl(IDC_ARCMARK_TYPE);
            if (ctrl)
            {
                ctrl->ShowCtrl(false);
            }
            ctrl = GetCtrl(IDC_ARCMARK_FILL);
            if (ctrl)
            {
                ctrl->ShowCtrl(true);
            }
            CStatic* text = dynamic_cast<CStatic*>(GetCtrl(IDC_ARCMARK_TYPE_TEXT));
            if (text)
            {
                text->SetText(LocalizeString(IDS_ARCMARK_TYPE2));
            }
            if (_oldType == MTIcon)
            {
                CEdit* edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_A));
                if (edit)
                {
                    float value = edit->GetText() ? atof(edit->GetText()) : 1;
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "%g", value * 20.0f);
                    edit->SetText(buffer);
                }
                edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCMARK_B));
                if (edit)
                {
                    float value = edit->GetText() ? atof(edit->GetText()) : 1;
                    char buffer[256];
                    snprintf(buffer, sizeof(buffer), "%g", value * 20.0f);
                    edit->SetText(buffer);
                }
            }
        }
        break;
        default:
            Fail("Bad marker type");
            break;
    }
    _oldType = curSel;
}

DisplayArcadeEffects::DisplayArcadeEffects(ControlsContainer* parent, ArcadeEffects effects, bool advanced)
    : Display(parent)
{
    _enableSimulation = false;
    _enableDisplay = false;
    _effects = effects;
    if (advanced)
    {
        Load("RscDisplayArcadeEffects");
    }
    else
    {
        Load("RscDisplayArcadeEffectsSimple");
        _effects.titleType = TitleText;
        ChangeTitleType(TitleText);
    }

    ChangeTitleType(_effects.titleType);
}

Control* DisplayArcadeEffects::OnCreateCtrl(int type, int idc, const ParamEntry& cls)
{
    switch (idc)
    {
        case IDC_ARCEFF_CONDITION:
        {
            CEdit* edit = new CEdit(this, idc, cls);
            edit->SetText(_effects.condition);
            return edit;
        }
        case IDC_ARCEFF_CAMERA:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int sel = -1;
            int index = combo->AddString(LocalizeString(IDS_CAMEFFECT_NONE));
            combo->SetData(index, "");
            if (_effects.cameraEffect.GetLength() == 0)
            {
                sel = index;
            }
            index = combo->AddString(LocalizeString(IDS_CAMEFFECT_TERMINATE));
            combo->SetData(index, "$TERMINATE$");
            if (stricmp(_effects.cameraEffect, "$TERMINATE$") == 0)
            {
                sel = index;
            }
            RString effect = _effects.cameraEffect;
            if (effect[0] == '@')
            {
                effect = (const char*)effect + 1;
            }
            const ParamEntry* cls = Pars.FindEntry("CfgCameraEffects");
            if (cls)
            {
                const ParamEntry& ar = (*cls) >> "Array";
                int n = ar.GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& entry = ar.GetEntry(i);
                    index = combo->AddString(entry >> "name");
                    combo->SetData(index, entry.GetName());
                    if (stricmp(effect, entry.GetName()) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsCampaign.FindEntry("CfgCameraEffects");
            if (cls)
            {
                const ParamEntry& ar = (*cls) >> "Array";
                int n = ar.GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& entry = ar.GetEntry(i);
                    index = combo->AddString(entry >> "name");
                    combo->SetData(index, entry.GetName());
                    if (stricmp(effect, entry.GetName()) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsMission.FindEntry("CfgCameraEffects");
            if (cls)
            {
                const ParamEntry& ar = (*cls) >> "Array";
                int n = ar.GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& entry = ar.GetEntry(i);
                    index = combo->AddString(entry >> "name");
                    combo->SetData(index, entry.GetName());
                    if (stricmp(effect, entry.GetName()) == 0)
                    {
                        sel = index;
                    }
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCEFF_CAMPOS:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int i;
            for (i = 0; i < NCamEffectPositions; i++)
            {
                combo->AddString(LocalizeString(IDS_CAMEFFECT_TOP + i));
            }
            combo->SetCurSel(_effects.cameraPosition);
            return combo;
        }
        case IDC_ARCEFF_SOUND:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int index = combo->AddString(LocalizeString(IDS_SOUND_NONE));
            combo->SetData(index, "$NONE$");
            int sel = index;
            RString sound = _effects.sound;
            if (sound[0] == '@')
            {
                sound = (const char*)sound + 1;
            }
            const ParamEntry* cls = Pars.FindEntry("CfgSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsCampaign.FindEntry("CfgSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsMission.FindEntry("CfgSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCEFF_VOICE:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int index = combo->AddString(LocalizeString(IDS_SOUND_NONE));
            combo->SetData(index, "");
            int sel = index;
            RString sound = _effects.voice;
            if (sound[0] == '@')
            {
                sound = (const char*)sound + 1;
            }
            const ParamEntry* cls = Pars.FindEntry("CfgSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsCampaign.FindEntry("CfgSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsMission.FindEntry("CfgSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCEFF_SOUND_ENV:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int index = combo->AddString(LocalizeString(IDS_SOUND_NONE));
            combo->SetData(index, "");
            int sel = index;
            RString sound = _effects.soundEnv;
            if (sound[0] == '@')
            {
                sound = (const char*)sound + 1;
            }
            const ParamEntry* cls = Pars.FindEntry("CfgEnvSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsCampaign.FindEntry("CfgEnvSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsMission.FindEntry("CfgEnvSounds");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCEFF_SOUND_DET:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int index = combo->AddString(LocalizeString(IDS_SOUND_NONE));
            combo->SetData(index, "");
            int sel = index;
            RString sound = _effects.soundDet;
            if (sound[0] == '@')
            {
                sound = (const char*)sound + 1;
            }
            const ParamEntry* cls = Pars.FindEntry("CfgSFX");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsCampaign.FindEntry("CfgSFX");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsMission.FindEntry("CfgSFX");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCEFF_MUSIC:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            RString sound = _effects.track;
            int index = combo->AddString(LocalizeString(IDS_MUSIC_NONE));
            combo->SetData(index, "$NONE$");
            int sel = index;
            index = combo->AddString(LocalizeString(IDS_MUSIC_SILENCE));
            combo->SetData(index, "$STOP$");
            if (stricmp("$STOP$", sound) == 0)
            {
                sel = index;
            }
            const ParamEntry* cls = Pars.FindEntry("CfgMusic");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsCampaign.FindEntry("CfgMusic");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsMission.FindEntry("CfgMusic");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), sound) == 0)
                    {
                        sel = index;
                    }
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCEFF_TITTYPE:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            combo->AddString(LocalizeString(IDS_TITTYPE_NONE));
            combo->AddString(LocalizeString(IDS_TITTYPE_OBJECT));
            combo->AddString(LocalizeString(IDS_TITTYPE_RESOURCE));
            combo->AddString(LocalizeString(IDS_TITTYPE_TEXT));
            combo->SetCurSel(_effects.titleType);
            return combo;
        }
        case IDC_ARCEFF_TITEFF:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int i;
            for (i = 0; i < NTitEffects; i++)
            {
                combo->AddString(LocalizeString(IDS_TITEFFECT_PLAIN + i));
            }
            combo->SetCurSel(_effects.titleEffect);
            return combo;
        }
        case IDC_ARCEFF_TITTEXT:
        {
            CEdit* edit = new CEdit(this, idc, cls);
            edit->SetText(_effects.title ? _effects.title : "");
            return edit;
        }
        case IDC_ARCEFF_TITRES:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            int sel = 0;
            const ParamEntry* cls = Res.FindEntry("RscTitles");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), _effects.title) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsCampaign.FindEntry("RscTitles");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), _effects.title) == 0)
                    {
                        sel = index;
                    }
                }
            }
            cls = ExtParsMission.FindEntry("RscTitles");
            if (cls)
            {
                int n = cls->GetEntryCount();
                for (int i = 0; i < n; i++)
                {
                    const ParamEntry& ci = cls->GetEntry(i);
                    if (!ci.IsClass())
                    {
                        continue;
                    }
                    if (!ci.FindEntry("name"))
                    {
                        continue;
                    }
                    RString displayName = ci >> "name";
                    int index = combo->AddString(displayName);
                    combo->SetData(index, ci.GetName());
                    if (stricmp(ci.GetName(), _effects.title) == 0)
                    {
                        sel = index;
                    }
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        case IDC_ARCEFF_TITOBJ:
        {
            CCombo* combo = new CCombo(this, idc, cls);
            const ParamEntry& cls = Pars >> "CfgTitles";
            int sel = 0;
            int i, n = (cls >> "titles").GetSize();
            for (i = 0; i < n; i++)
            {
                RString name = (cls >> "titles")[i];
                RString displayName = cls >> name >> "name";
                int index = combo->AddString(displayName);
                combo->SetData(index, name);
                if (stricmp(name, _effects.title) == 0)
                {
                    sel = index;
                }
            }
            combo->SetCurSel(sel);
            return combo;
        }
        default:
            return Display::OnCreateCtrl(type, idc, cls);
    }
}

void DisplayArcadeEffects::OnComboSelChanged(int idc, int curSel)
{
    if (idc == IDC_ARCEFF_TITTYPE)
    {
        ChangeTitleType(curSel);
    }

    Display::OnComboSelChanged(idc, curSel);
}

void DisplayArcadeEffects::Destroy()
{
    Display::Destroy();

    if (_exit != IDC_OK)
    {
        return;
    }

    CEdit* edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCEFF_CONDITION));
    if (edit)
    {
        _effects.condition = edit->GetText();
    }
    CCombo* combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_CAMERA));
    if (combo)
    {
        _effects.cameraEffect = combo->GetData(combo->GetCurSel());
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_CAMPOS));
    if (combo)
    {
        _effects.cameraPosition = (CamEffectPosition)combo->GetCurSel();
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_SOUND));
    if (combo)
    {
        _effects.sound = combo->GetData(combo->GetCurSel());
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_VOICE));
    if (combo)
    {
        _effects.voice = combo->GetData(combo->GetCurSel());
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_SOUND_ENV));
    if (combo)
    {
        _effects.soundEnv = combo->GetData(combo->GetCurSel());
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_SOUND_DET));
    if (combo)
    {
        _effects.soundDet = combo->GetData(combo->GetCurSel());
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_MUSIC));
    if (combo)
    {
        _effects.track = combo->GetData(combo->GetCurSel());
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_TITTYPE));
    if (combo)
    {
        _effects.titleType = (TitleType)combo->GetCurSel();
    }
    combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_TITEFF));
    if (combo)
    {
        _effects.titleEffect = (TitEffectName)combo->GetCurSel();
    }
    _effects.title = "";
    switch (_effects.titleType)
    {
        case TitleObject:
            combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_TITOBJ));
            if (combo)
            {
                _effects.title = combo->GetData(combo->GetCurSel());
            }
            break;
        case TitleResource:
            combo = dynamic_cast<CCombo*>(GetCtrl(IDC_ARCEFF_TITRES));
            if (combo)
            {
                _effects.title = combo->GetData(combo->GetCurSel());
            }
            break;
        case TitleText:
        {
            CEdit* edit = dynamic_cast<CEdit*>(GetCtrl(IDC_ARCEFF_TITTEXT));
            if (edit)
            {
                _effects.title = edit->GetText();
                if (_effects.title.GetLength() == 0)
                {
                    _effects.titleType = TitleNone;
                }
            }
        }
        break;
    }
}

void DisplayArcadeEffects::ChangeTitleType(int type)
{
    bool showText = false;
    bool showRes = false;
    bool showObj = false;

    switch (type)
    {
        case TitleNone:
            break;
        case TitleObject:
            showObj = true;
            break;
        case TitleResource:
            showRes = true;
            break;
        case TitleText:
            showText = true;
            break;
    }

    IControl* ctrl;
    ctrl = GetCtrl(IDC_ARCEFF_TITTEXT);
    if (ctrl)
    {
        ctrl->ShowCtrl(showText);
    }
    ctrl = GetCtrl(IDC_ARCEFF_TITRES);
    if (ctrl)
    {
        ctrl->ShowCtrl(showRes);
    }
    ctrl = GetCtrl(IDC_ARCEFF_TITOBJ);
    if (ctrl)
    {
        ctrl->ShowCtrl(showObj);
    }
    ctrl = GetCtrl(IDC_ARCEFF_TEXT_TITTEXT);
    if (ctrl)
    {
        ctrl->ShowCtrl(showObj || showRes || showText);
    }
}

} // namespace Poseidon
