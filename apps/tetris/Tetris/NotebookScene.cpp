#include "NotebookScene.hpp"

#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Math/Math3DP.hpp>
#include <Poseidon/Foundation/Math/MathOpt.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

using namespace Poseidon;
namespace Poseidon
{
RString FindShape(RString name);
}
using Poseidon::FindShape;

// Same constants as ControlsContainer / ControlObject — match the
// options-menu camera scaling so the notebook fills the screen exactly
// like RscOptionsShell.  Defined in engine/poseidon/UI/Controls/uiControls.cpp.
extern const float CameraZoom;
extern const float InvCameraZoom;

namespace
{
// Match RscOptionsShell's OptTplNotebook template.
constexpr float kSin30 = 0.5f;
constexpr float kCos30 = 0.8660254f;

const char* kNotebookModel = "notebook.p3d";
const char* kNotebookAnim = "notebook.rtm";

// Z is multiplied by CameraZoom in the resource constructor — see
// ControlObject::ControlObject in uiControls.cpp:317.  Apply the same
// scaling here so the on-screen size matches the options menu.
Vector3 NotebookClosePosition()
{
    return Vector3(0.0f, -0.175f, 0.250f * CameraZoom);
}
Vector3 NotebookFarPosition()
{
    return Vector3(0.0f, -0.040f, 0.600f * CameraZoom);
}
} // namespace

NotebookScene::NotebookScene() : Object(nullptr, -1) {}

NotebookScene::~NotebookScene() = default;

bool NotebookScene::Load()
{
    RString modelPath = FindShape(RString(kNotebookModel));
    if (modelPath.GetLength() == 0)
    {
        LOG_ERROR(Core, "NotebookScene: cannot find model {}", kNotebookModel);
        return false;
    }

    _shape = Shapes.New(modelPath, false, false);
    if (!_shape || _shape->NLevels() <= 0)
    {
        LOG_ERROR(Core, "NotebookScene: shape {} failed to load", (const char*)modelPath);
        return false;
    }

    // Match ControlObject + ControlObjectContainerAnim setup so the
    // engine treats this exactly like the options menu notebook.
    _shape->LevelOpaque(0)->MakeCockpit();
    _shape->OrSpecial(BestMipmap | NoDropdown | DisableSun);
    _shape->SetAutoCenter(false);
    _shape->CalculateBoundingSphere();
    _shape->AllowAnimation();

    // Build skeleton + open animation.
    _skeleton = new Skeleton();
    AnimationRTName name;
    name.name = GetAnimationName(RString(kNotebookAnim));
    name.skeleton = _skeleton;
    _animation = new AnimationRT(name, false);
    _animation->Prepare(_shape, _skeleton, _weights, false);

    // Static frame transform — OptTplNotebook's direction/up vectors.
    SetPosition(NotebookFarPosition());
    SetOrient(Vector3(0.0f, kSin30, kCos30), Vector3(0.0f, kCos30, -kSin30));
    SetScale(1.0f);

    _loaded = true;
    LOG_INFO(Core, "NotebookScene: loaded {} + {}", kNotebookModel, kNotebookAnim);
    return true;
}

void NotebookScene::Tick(float deltaT)
{
    if (!_loaded)
        return;

    // Animation phase: 0 → 1 over (1 / _animSpeed) seconds.
    if (_phase < 1.0f)
    {
        _phase += deltaT * _animSpeed;
        if (_phase > 1.0f)
            _phase = 1.0f;
    }

    // Zoom interpolation positionBack → position over _zoomDuration.
    // Linear is good enough; the options menu uses a cubic spline but
    // for a static demo the difference is invisible.
    if (_zoomT < 1.0f)
    {
        _zoomT += deltaT / _zoomDuration;
        if (_zoomT > 1.0f)
            _zoomT = 1.0f;
    }

    const Vector3 from = NotebookFarPosition();
    const Vector3 to = NotebookClosePosition();
    SetPosition(from + (to - from) * _zoomT);
}

void NotebookScene::Animate(int level)
{
    if (_animation && _shape)
        _animation->Apply(_weights, _shape, level, _phase);
}

void NotebookScene::RenderScene()
{
    if (!_loaded || !GScene || !GEngine)
        return;

    // Clear Z so the scene renders on top of whatever the engine's
    // background pass left behind (the options menu does the same).
    GEngine->Clear(true, false);

    LightList lights(true);
    lights.Resize(1);
    Ref<LightPoint> light = new LightPoint(Color(1, 1, 0.8f, 1), Color(0.5f, 0.5f, 0.4f, 1));
    lights[0] = light;
    light->SetPosition(Vector3(1, 1, 0));
    light->SetBrightness(8.0f * Square(CameraZoom));
    GScene->SetActiveLights(lights);

    // Camera identical to ControlsContainer::OnDraw — 0.5 × InvCameraZoom
    // FOV scale, near=0.1, far=100.
    const Camera savedCamera = *GScene->GetCamera();
    const float fov = 0.5f * InvCameraZoom;
    Camera cam;
    cam.SetPerspectiveForView(GEngine, 0.1f, 100.0f, fov);
    cam.Adjust(GEngine);
    GScene->SetCamera(cam);
    GEngine->UpdateFrameCamera();

    SetConstantColor(PackedColor(Color(1, 1, 1, 1)));
    Object::Draw(0, ClipAll | ClipUser0, *this);

    GScene->SetCamera(savedCamera);
    GEngine->UpdateFrameCamera();
}
