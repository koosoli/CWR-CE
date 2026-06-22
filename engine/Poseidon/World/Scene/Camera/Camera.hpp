#pragma once

#include <Poseidon/Core/Visual.hpp>
#include <Poseidon/World/Entities/Vehicles/Plane.hpp>

namespace Poseidon
{
class Camera: public FrameBase
{
	// camera is frame with projection
	friend class Scene;

	private:
	Matrix4 _projection; // result in (0,0) .. (w,h) range
	Matrix4 _projectionNormal; // result in (-1,-1) .. (+1,+1) range
	Matrix4 _scale; // scale scene so that viewing frustum is +,-1
	Matrix4 _invScale; // rescale back again
	
	// temporary storage for object clipping
	Matrix4 _scaledInvTransform;

	// temporary storage for surface drawing
	Matrix3 _camNormalTrans;
	Matrix4 _camInvTrans;

	Coord _cNear,_cFar,_cLeft,_cTop; // perspective parameters
	Coord _invCTop,_invCLeft;

	Coord _cAddNear,_cAddFar; // additional clipping planes

	
	Plane _rClipPlane,_lClipPlane; // world space clipping planes
	Plane _tClipPlane,_bClipPlane;
	Plane _nClipPlane,_fClipPlane; // near and far

	Plane _rGuardPlane,_lGuardPlane; // world space clipping planes
	Plane _tGuardPlane,_bGuardPlane;
	
	Vector3 _speed; // used for sound calculations

	Vector3 _userClipDirWorld; // world space clipping coordinates
	float _userClipValWorld;

	Vector3 _userClipDir; // pre-calculated view space plane equations	
	float _userClipVal;

	bool _userClip;

	public:
	// properties
	Camera();

	const Matrix4 &Projection() const {return _projection;}
	const Matrix4 &ProjectionNormal() const {return _projectionNormal;}
	const Matrix4 &ScaleMatrix() const {return _scale;}
	const Matrix4 &InvScaleMatrix() const {return _invScale;}
	void SetPerspective
	(
		Coord cNear, Coord cFar, Coord cLeft, Coord cTop
	);
	// Perspective from a view FOV with the engine's aspect applied —
	// the one place aspect enters the camera (left = fov * leftFOV,
	// top = fov * topFOV from the engine's AspectSettings).
	void SetPerspectiveForView( Engine *engine, Coord cNear, Coord cFar, Coord fov );
	void Adjust( Engine *engine );
	void SetClipRange( Coord cNear, Coord cFar );
	void SetAdditionalClipping(float cNear, Coord cFar );
	float GetAdditionalClippingNear() const {return _cAddNear;}
	float GetAdditionalClippingFar() const {return _cAddFar;}

	const Plane &GetNearClipPlane() const {return _nClipPlane;}
	const Plane &GetFarClipPlane() const {return _fClipPlane;}
	
	Coord Near() const {return _cNear;}
	Coord Far() const {return _cFar;}
	Coord Left() const {return _cLeft;}
	Coord Top() const {return _cTop;}
	Coord InvTop() const {return _invCTop;}
	Coord InvLeft() const {return _invCLeft;}
	Coord ClipNear() const {return _cNear;}
	Coord ClipFar() const {return _cFar;}

	void CancelUserClip();
	void SetUserClipPars(Vector3Par dir, float val);

	Vector3Val GetUserClipDir( int i ) const {return _userClipDirWorld;}
	float GetUserClipVal( int i ) const {return _userClipValWorld;}
	
	Vector3Val UserClipDir() const {return _userClipDir;}
	float UserClipVal() const {return _userClipVal;}

	bool IsUserClip() const {return _userClip;}
	
	Vector3Val Speed() const {return _speed;} // used for sound calculations
	void SetSpeed( const Vector3 speed ) {_speed=speed;} // used for sound calculations

	// world space clipping
	ClipFlags IsClipped( Vector3Par point, float radius, int userPlane ) const;
	ClipFlags MayBeClipped( Vector3Par point, float radius, int userPlane ) const;
};

}  // namespace Poseidon
