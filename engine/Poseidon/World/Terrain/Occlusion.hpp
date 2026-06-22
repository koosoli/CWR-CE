#pragma once

#if _MSC_FULL_VER>=12008804 && !defined(_M_X64) && !defined(_M_AMD64)
#define _COMPILER_CAN_MMX 1
#endif

#if defined(__ICL) && !defined(_M_X64) && !defined(_M_AMD64)
#define _COMPILER_CAN_MMX 1
#endif

#define SIMD 0
#define SIMD2 0

#include <Poseidon/Foundation/Math/Fixed.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <Poseidon/Dev/Debug/DebugWin.hpp>

namespace Poseidon
{
#if SIMD || SIMD2
	// unsigned 8b SSE occlusions
	typedef unsigned char OccZType;
#else
	// signed 8b MMX occlusions
	typedef signed char OccZType;
#endif




class OcclusionPoly
{
	int _n;
	Vector3 _v[MaxPoly];

	public:
	OcclusionPoly(){_n=0;}
	void Copy( const OcclusionPoly &src );

	OcclusionPoly( const OcclusionPoly &src ){Copy(src);}
	void operator =( const OcclusionPoly &src ){Copy(src);}

	int N() const {return _n;}
	Vector3Val operator [] ( int index ) const {return _v[index];}
	Vector3Val Get( int index ) const {return _v[index];}

	void Add( Vector3Par v )
	{
		if (_n<MaxPoly)
		{
			_v[_n++]=v;
		}
		else
		{
			LOG_DEBUG(World, "Too complex occlusion");
		}
	}
	void Clear() {_n=0;}

	// clipping with single plane
	bool Clip( OcclusionPoly &res, Vector3Par normal, Coord d ) const;

	// clipping with all planes
	void Clip( float cNear, float cFar, ClipFlags clip );

	void Perspective();

	void Transform(const Matrix4 &trans);

	void SumXYVolume(float &volume, Vector3 &cov) const;
	void SumCrossProducts(Vector3 &sum) const;
	void SumPositions(Vector3 &sum,int &count) const;
};

class Occlusion
{
	#if _ENABLE_CHEATS
	mutable Link<Dev::DebugMemWindow> _debugWin;
	#endif

	APtr<OccZType> _data;
	int _w,_h;
	float _w2,_h2; // precalculate w/2, h/2

	// note - ordered by column - accessed and rendered by column
	OccZType Get( int x, int y ) const {return _data[x*_h+y];}
	OccZType &Set( int x, int y ) {return _data[x*_h+y];}
	
	__forceinline OccZType &operator () ( int x, int y ) {return Set(x,y);}
	__forceinline OccZType operator () ( int x, int y ) const {return Get(x,y);}

	public:
	Occlusion( int w, int h );
	~Occlusion();

	bool TestRect( const OccZType *col, int w, int h, OccZType maxz ) const;
	bool TestProjectedBBox( const Vector3 &min, const Vector3 &max ) const;
	// project, clip and test bbox
	bool TestBBox
	(
		const Matrix4 &trans, const Vector3 *minMax, ClipFlags clip
	) const;

	void DebugPoly( const OcclusionPoly &poly, Dev::DebugPixel color ) const;

	// horizontal fill
	void RenderSpan( OccZType *tgt, int yBeg, int width, OccZType z );

	// render projected triangle
	void RenderTri
	(
		const Vector3 *vip, const Vector3 *vlp, const Vector3 *vrp
	);

	// predefined callbacks
	void RenderProjectedPoly( const OcclusionPoly &poly );
	void RenderPoly( const OcclusionPoly &poly, ClipFlags clip );
	bool TestPoint( Vector3Par pos ) const;
	bool TestPointWSpace( Vector3Par pos ) const;
	float TestSphereWSpace(Vector3Par pos, float radius) const;
	// sample a pixel-space rectangle [x-sizeX,x+sizeX]x[y-sizeY,y+sizeY], clamped to the buffer
	float SampleCoverage( int x, int y, int sizeX, int sizeY, OccZType nearestZ ) const;
	// project, clip and render
	void RenderComponent
	(
		const Matrix4 &transform,
		// model space camera
		Vector3Val camDir,Vector3Val camPos,
		Shape *shape, ConvexComponent &components,
		ClipFlags orClip
	);
	void RenderShape
	(
		Matrix4 &trans, Shape *shape, const ConvexComponents &components,
		ClipFlags orClip
	);

	void Clear();

	void OutputDebug();
};

}  // namespace Poseidon
