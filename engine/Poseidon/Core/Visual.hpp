#pragma once

#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Core/Types.hpp>


// Frame gives new coordinate system
// base class for camera, objects ...


namespace Poseidon
{
class Frame: public Matrix4 
{
	public:
	Frame():Matrix4(MIdentity){}

	const Matrix4 &Transform() const {return *this;}
	void SetTransform( const Matrix4 &transform ) {this->Matrix4::operator =(transform);}

	Matrix4 CalcInvTransform() const {return InverseScaled();}

	void SetOrient( Vector3Par dir, Vector3Par up )
	{
		SetDirectionAndUp(dir,up);
	}
	void SetOrientScaleOnly( float scale )
	{
		SetOrientation(Matrix3(MScale,scale));
	}

	// model to world and back transformations
	const Matrix4 &ModelToWorld() const {return *this;}
	const Matrix3 &DirModelToWorld() const {return Orientation();}
	
	#ifdef _KNI
	Vector3 PositionModelToWorld( Vector3Par v ) const {return FastTransform(v);}
	Vector3 DirectionModelToWorld( Vector3Par v ) const {return Rotate(v);}
	#else
	Vector3 PositionModelToWorld( Vector3Par v ) const {return Vector3(VFastTransform,*this,v);}
	Vector3 DirectionModelToWorld( Vector3Par v ) const {return Vector3(VRotate,*this,v);}
	#endif

	// faster but less convenient versions
	void PositionModelToWorld( Vector3 &res, Vector3Par v ) const {res.SetFastTransform(*this,v);}
	void DirectionModelToWorld( Vector3 &res, Vector3Par v ) const {res.SetRotate(*this,v);}
	
	bool Equal( const Frame &with ) const;
	bool operator == ( const Frame &with ) const {return Equal(with);}
	bool operator != ( const Frame &with ) const {return !Equal(with);}
};


// frame with possibility of virtual optimization - used as base for Object

class FrameBase: public Frame
{
	protected:
	float _scale; // remmemer scale to avoid its calculation

	public:
	FrameBase();

	// optimized scale calculation
	float Scale() const {return _scale;}
	void SetScale( float scale ) {_scale=scale;Frame::SetScale(scale);}

	// virtually optimized Frame inverse transform
	virtual void SetPosition( Vector3Par pos );
	virtual void SetTransform( const Matrix4 &transform );
	virtual void SetOrient( const Matrix3 &dir );
	virtual void SetOrient( Vector3Par dir, Vector3Par up );
	virtual void SetOrientScaleOnly( float scale );

	virtual Matrix4 GetInvTransform() const;
};
} // namespace Poseidon

// Global-scope aliases for commonly used types defined in namespace Poseidon
using Poseidon::Frame;
using Poseidon::FrameBase;
