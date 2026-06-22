#pragma once

#include <Poseidon/Core/Visual.hpp>



namespace Poseidon
{
class FrameWithInverse: public FrameBase
{
	Matrix4 _invTransform;

	public:
	FrameWithInverse( Matrix4Par trans, Matrix4Par invTrans );
	void CalculateInv() const {}
	void InvDirty() const {}
	const Matrix4 &InvTransform() const {return _invTransform;}
	Matrix4 GetInvTransform() const override {return _invTransform;}

	private:
	#define VIRTUAL_DISABLED {Fail("Disabled");}
	void SetPosition( Vector3Par pos ) override VIRTUAL_DISABLED
	void SetTransform( const Matrix4 &transform ) override VIRTUAL_DISABLED

	void SetOrient( const Matrix3 &dir ) override VIRTUAL_DISABLED
	void SetOrient( Vector3Par dir, Vector3Par up ) override VIRTUAL_DISABLED
	void SetOrientScaleOnly( float scale ) override VIRTUAL_DISABLED

	public:
	const Matrix4 &WorldToModel() const {return InvTransform();}
	const Matrix3 &DirWorldToModel() const {return InvTransform().Orientation();}
	
	Vector3 PositionWorldToModel( Vector3Par v ) const {return Vector3(VFastTransform,InvTransform(),v);}
	Vector3 DirectionWorldToModel( Vector3Par v ) const {return Vector3(VRotate,InvTransform(),v);}

	void PositionWorldToModel( Vector3 &res, Vector3Par v ) const {res.SetFastTransform(InvTransform(),v);}
	void DirectionWorldToModel( Vector3 &res, Vector3Par v ) const {res.SetRotate(InvTransform(),v);}
};

}  // namespace Poseidon
