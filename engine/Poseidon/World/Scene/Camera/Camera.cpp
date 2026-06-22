#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Graphics/Textures/TextureBank.hpp>

#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>

#pragma optimize("t", on) // optimize for speed instead of size
#pragma optimize("", on)  // restore default setting

namespace Poseidon
{
Camera::Camera()
    : _projection(MIdentity), _scale(MIdentity), _invScale(MIdentity), _scaledInvTransform(MIdentity),
      _camNormalTrans(M3Identity), _camInvTrans(MIdentity), _userClip(false)
{
}

void Camera::SetPerspective(Coord cNear, Coord cFar, Coord cLeft, Coord cTop)
{
    _cNear = cNear, _cFar = cFar, _cLeft = cLeft, _cTop = cTop;
    _cAddNear = cNear;
    _cAddFar = cFar;
}

void Camera::SetPerspectiveForView(Engine* engine, Coord cNear, Coord cFar, Coord fov)
{
    Poseidon::AspectSettings as;
    engine->GetAspectSettings(as);
    SetPerspective(cNear, cFar, fov * as.leftFOV, fov * as.topFOV);
}

void Camera::Adjust(Engine* engine)
{
    _invCLeft = 1 / _cLeft;
    _invCTop = 1 / _cTop;

    _projectionNormal = MZero;
    _projectionNormal(0, 0) = _invCLeft;
    _projectionNormal(1, 1) = _invCTop;
    float ccFar = floatMax(500, _cFar * 1.01);
    if (::Poseidon::GEngine->HasWBuffer() && ::Poseidon::GEngine->IsWBuffer())
    {
        // far is critical for precision of w-buffer
        ccFar = floatMax(100, _cFar * 1.1);
    }

    float q = ccFar / (ccFar - _cNear);
    _projectionNormal(2, 2) = q;
    _projectionNormal.SetPosition(Vector3(0, 0, -q * _cNear));

    _projection.SetZero();
    // see DX docs: What Is the Projection Transformation?
    // (note: DX uses row vectors)
    // our setup of DX matrix
    // |1/fovW     0   0    0    |   |x|   |   don't care     |
    // |   0    1/fowH 0    0    |   |y|   |   don't care     |
    // |   0       0   q -q*cNear| * |z| = | q*z - q*cNear    |
    // |   0       0   1    0    |   |1|   |       z          |

    // perspective:       z = (q*z - q*cNear)/z = q - q*cNear/z

    // The 3D world renders into a sub-rectangle of the framebuffer (the
    // AspectSettings world rect).  Default (0,0,1,1) == full window, in
    // which case x0/y0 are 0 and w/h are the full dimensions.  A centered
    // sub-rect crops the world without stretch: the FOV (cLeft/cTop)
    // already matches the rect aspect, so the projection just scales NDC
    // to the rect size and offsets it.
    const int fw = engine->Width();
    const int fh = engine->Height();
    AspectSettings asv;
    engine->GetAspectSettings(asv);
    int x0 = toInt(asv.worldLeft * fw);
    int y0 = toInt(asv.worldTop * fh);
    int w = toInt(asv.worldRight * fw) - x0;
    int h = toInt(asv.worldBottom * fh) - y0;
    if (w <= 0)
    {
        x0 = 0;
        w = fw;
    }
    if (h <= 0)
    {
        y0 = 0;
        h = fh;
    }
    // y is negative - positive should go up
    // cLeft and cTop is ignored now - therefore sides of viewing frustum is viewport independent

    // our setup of custom matrix
    // |  w/2   0   x0+w/2   0    |   |x|   |  don't care      |
    // |   0   h/2  y0+h/2   0    |   |y|   |  don't care      |
    // |   0    0    q -q*cNear| * |z| = | q*z - q*cNear    |
    // |   0    0    1    0    |   |1|   |       z          |

    // guarantee sky and clouds are visible
    _projection(0, 0) = w * 0.5f;
    _projection(1, 1) = -h * 0.5f;
    _projection(2, 2) = q;
    _projection(0, 2) = x0 + w * 0.5f;
    _projection(1, 2) = y0 + h * 0.5f;
    _projection.SetPosition(Vector3(0, 0, -q * _cNear));

    _scale = Matrix4(MScale, _invCLeft, _invCTop, 1);
    // after easy clipping rescale viewing frustum to get correct screen coordinates
    _invScale = Matrix4(MScale, _cLeft, _cTop, 1);

    _scaledInvTransform = _scale * GetInvTransform();
    _camNormalTrans = Matrix3(MNormalTransform, _scaledInvTransform.Orientation());

    _camInvTrans = Matrix4(MInverseScaled, _scaledInvTransform);

    // calculate world space clipping planes
    Vector3 pl(+1, 0, _cLeft);
    Vector3 pt(0, +1, _cTop);
    pl.Normalize();
    pt.Normalize();
    Vector3 pr(-pl[0], 0, pl[2]);
    Vector3 pb(0, -pt[1], pt[2]);

    Vector3Val pos = Position();

    DirectionModelToWorld(pr, pr);
    DirectionModelToWorld(pl, pl);
    DirectionModelToWorld(pb, pb);
    DirectionModelToWorld(pt, pt);

    _rClipPlane = Plane(pr, pos);
    _lClipPlane = Plane(pl, pos);
    _tClipPlane = Plane(pt, pos);
    _bClipPlane = Plane(pb, pos);

    float nearClip = floatMax(_cNear, _cAddNear);
    float farClip = floatMin(_cFar, _cAddFar);

    _nClipPlane = Plane(Direction(), pos + Direction() * nearClip);
    _fClipPlane = Plane(-Direction(), pos + Direction() * farClip);

#if 1
    // calculate world space guard plane equations

    // (-1,-1) is projected to (0,0)
    // (+1,+1) is projected to (w,h)
    // (a,b) -> ( (a+1)*w/2, (b+1)*h/2 )

    // what is projected to (gx,gy)
    // a*w/2 + w/2 = gx,  a = (gx - w/2)/(w/2), a = gx/(w/2)-1
    // b*h/2 + h/2 = gy,  b = (gy - h/2)/(h/2), b = gy/(h/2)-1
    float invW2 = 2.0f / w;
    float invH2 = 2.0f / h;

    float minX = (engine->MinGuardX() - x0) * invW2 - 1;
    float maxX = (engine->MaxGuardX() - x0) * invW2 - 1;
    float minY = (engine->MinGuardY() - y0) * invH2 - 1;
    float maxY = (engine->MaxGuardY() - y0) * invH2 - 1;

    // minX, minY is negative

    Vector3 gl(+1, 0, -_cLeft * minX);
    Vector3 gt(0, +1, -_cTop * minY);
    Vector3 gr(-1, 0, +_cLeft * maxX);
    Vector3 gb(0, -1, +_cTop * maxY);

    // left: we want to find plane that goes through points:
    // o,gl,gl+VUp
    // normal of such plane is:
    // gl x (gl+VUp)

    gl.Normalize();
    gt.Normalize();
    gr.Normalize();
    gb.Normalize();

    DirectionModelToWorld(gr, gr);
    DirectionModelToWorld(gl, gl);
    DirectionModelToWorld(gb, gb);
    DirectionModelToWorld(gt, gt);

    _rGuardPlane = Plane(gr, pos);
    _lGuardPlane = Plane(gl, pos);
    _tGuardPlane = Plane(gt, pos);
    _bGuardPlane = Plane(gb, pos);
#else
    _rGuardPlane = _rClipPlane;
    _lGuardPlane = _lClipPlane;
    _tGuardPlane = _tClipPlane;
    _bGuardPlane = _bClipPlane;
#endif
}

void Camera::SetAdditionalClipping(float cNear, Coord cFar)
{
    _cAddFar = cFar;
    _cAddNear = cNear;

    Adjust(::Poseidon::GEngine);
}

void Camera::SetClipRange(Coord cNear, Coord cFar)
{
    _cNear = cNear;
    _cFar = cFar;
    // reset addional clipping planes
    _cAddNear = cNear;
    _cAddFar = cFar;
    Adjust(::Poseidon::GEngine);
}

// world space clipping
ClipFlags Camera::IsClipped(Vector3Par point, float radius, int userPlane) const
{
    ClipFlags clip = ClipNone;
    if (_nClipPlane.Distance(point) <= -radius)
    {
        clip |= ClipFront;
    }
    if (_fClipPlane.Distance(point) <= -radius)
    {
        clip |= ClipBack;
    }
    if (_rClipPlane.Distance(point) <= -radius)
    {
        clip |= ClipRight;
    }
    if (_lClipPlane.Distance(point) <= -radius)
    {
        clip |= ClipLeft;
    }
    if (_tClipPlane.Distance(point) <= -radius)
    {
        clip |= ClipTop;
    }
    if (_bClipPlane.Distance(point) <= -radius)
    {
        clip |= ClipBottom;
    }

    return clip;
}

ClipFlags Camera::MayBeClipped(Vector3Par point, float radius, int userPlane) const
{
    ClipFlags clip = ClipAll;
    float dn = _nClipPlane.Distance(point);
    float df = _fClipPlane.Distance(point);
    if (dn >= +radius)
    {
        clip &= ~ClipFront;
    }
    if (df >= +radius)
    {
        clip &= ~ClipBack;
    }
#define CHECK_GUARD_CLIP 0
#if CHECK_GUARD_CLIP
    ClipFlags vClip = clip;
    float dr = _rClipPlane.Distance(point);
    float dl = _lClipPlane.Distance(point);
    float dt = _tClipPlane.Distance(point);
    float db = _bClipPlane.Distance(point);
    if (dr >= +radius)
        vClip &= ~ClipRight;
    if (dl >= +radius)
        vClip &= ~ClipLeft;
    if (dt >= +radius)
        vClip &= ~ClipTop;
    if (db >= +radius)
        vClip &= ~ClipBottom;
#endif

    // we know we are clipping
    // check against guard planes to known which planes we must clip

    float gr = _rGuardPlane.Distance(point);
    float gl = _lGuardPlane.Distance(point);
    float gt = _tGuardPlane.Distance(point);
    float gb = _bGuardPlane.Distance(point);
    if (gr >= +radius)
    {
        clip &= ~ClipRight;
    }
    if (gl >= +radius)
    {
        clip &= ~ClipLeft;
    }
    if (gt >= +radius)
    {
        clip &= ~ClipTop;
    }
    if (gb >= +radius)
    {
        clip &= ~ClipBottom;
    }

#if CHECK_GUARD_CLIP
    // if front clipping, some clipping flags may be wrong
    if ((clip & 1) == 0 && (clip & ~vClip) != 0)
    {
        LOG_DEBUG(Graphics, "Incorrect clip flags {:2x},{:2x}", clip, vClip);
    }
#endif

    return clip;
}

void Camera::CancelUserClip()
{
    _userClip = false;
}

void Camera::SetUserClipPars(Vector3Par dir, float val)
{
    _userClip = true;

    _userClipDirWorld = dir;
    _userClipValWorld = val;

    // recalculate user clipping planes to view space
    // how normals are converted?
    Matrix4Val viewTransform = _scale * GetInvTransform();       // from world to view
    Matrix4Val invViewTransform = viewTransform.InverseScaled(); // from view to world
    //  let:  n=_userClipDir[i], d=_userClipVal[i]
    //        x=checked point
    //        M=invViewTransform.Orientation()
    //        m=invViewTransform.Position()

    // then:
    //       world space clipping equation is:
    //       n.x + d=0
    //
    // world space check can be applied to Mx+m
    //       n.(Mx+m) + d =0
    //       n.Mx+n.m +d =0
    // dot product can be rewritten using transpose n.x = T(n)x
    //       T(n)Mx +n.m+d =0
    //       ( T(n)M ) x +n.m+d=0
    Matrix3Val matrixM = invViewTransform.Orientation();
    Vector3Val vectorM = invViewTransform.Position();
    Vector3Val n = _userClipDirWorld;
    float d = _userClipValWorld;
    _userClipVal = n * vectorM + d;
    _userClipDir = n * matrixM;
}

} // namespace Poseidon
