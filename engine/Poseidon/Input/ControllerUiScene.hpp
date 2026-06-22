#pragma once

#include <Poseidon/Input/ControllerUiInput.hpp>

#include <string>

namespace Poseidon
{

enum class ControllerSceneKind
{
    None,
    Menu,
    Modal,
    TextEntry,
    Gameplay,
    PauseMenu,
    Notebook,
    Map,
    EditorMap,
    EditorDialog,
    Multiplayer,
    ModsCatalog,
    DownloadModal,
    CommandMenu,
    Chat,
    Viewer,
};

enum class ControllerSectionKind
{
    None,
    FocusList,
    Pager,
    SpatialCursor,
    TextField,
    ModalButtons,
    Gameplay,
    Capture,
};

enum ControllerUiCapability : unsigned
{
    CtrlNav = 1u << 0,
    CtrlConfirm = 1u << 1,
    CtrlCancel = 1u << 2,
    CtrlPage = 1u << 3,
    CtrlPointer = 1u << 4,
    CtrlCapture = 1u << 5,
    CtrlPause = 1u << 6,
    CtrlEditorMode = 1u << 7,
    CtrlPreview = 1u << 8,
    CtrlDelete = 1u << 9,
};

struct ControllerUiSection
{
    ControllerSectionKind kind = ControllerSectionKind::None;
    unsigned capabilities = 0;
    int ownerIdc = -1;
};

struct ControllerUiScene
{
    ControllerSceneKind kind = ControllerSceneKind::None;
    ControllerUiSection activeSection;
    unsigned sceneCapabilities = 0;
    bool menuFallbackAllowed = true;
    bool consumesLeftStick = false;
    bool consumesRightStick = false;
    bool consumesConfirmAsPointer = false;
};

ControllerUiSection FocusListSection(int ownerIdc = -1);
ControllerUiSection PagerSection(int ownerIdc = -1);
ControllerUiSection ModalButtonsSection(int ownerIdc = -1);
ControllerUiSection SpatialCursorSection(int ownerIdc = -1);
ControllerUiSection TextFieldSection(int ownerIdc = -1);
ControllerUiSection CaptureSection(int ownerIdc = -1);
ControllerUiSection GameplaySection(int ownerIdc = -1);

ControllerUiScene MenuControllerScene();
ControllerUiScene EditorMapControllerScene(int ownerIdc = -1);
ControllerUiScene EditorDialogControllerScene(int ownerIdc = -1);
ControllerUiScene GameplayControllerScene();
ControllerUiScene PauseMenuControllerScene();

bool ControllerUiSceneAccepts(const ControllerUiScene& scene, ControllerUiAction action);
std::string BuildControllerPromptString(const ControllerUiScene& scene);
const char* ControllerSceneKindName(ControllerSceneKind kind);
const char* ControllerSectionKindName(ControllerSectionKind kind);

} // namespace Poseidon
