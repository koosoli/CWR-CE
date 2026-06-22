#pragma once


#include <Poseidon/Graphics/Core/Engine.hpp>
namespace Poseidon
{
inline float InsideLeftPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return v.x-rect.x;
}
inline float InsideRightPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return rect.x+rect.w-v.x;
}
inline float InsideTopPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return v.y-rect.y;
}
inline float InsideBottomPixel( const Vertex2DPixel &v, const Rect2DPixel &rect )
{
	return rect.y+rect.h-v.y;
}

inline float InsideLeftAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return v.x-rect.x;
}
inline float InsideRightAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return rect.x+rect.w-v.x;
}
inline float InsideTopAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return v.y-rect.y;
}
inline float InsideBottomAbs( const Vertex2DAbs &v, const Rect2DAbs &rect )
{
	return rect.y+rect.h-v.y;
}

template <class Rect, class Vertex, class InsideF>
int Clip2D
(
	const Rect &rect,
	Vertex *dest, const Vertex *vertices, int n, InsideF inside
)
{
	int dn=0;
	const Vertex *p=vertices+n-1;
	float pIn=inside(*p,rect);
	for( int i=0; i<n; i++ )
	{
		const Vertex *a=vertices+i;
		float aIn=inside(*a,rect);
		if( (aIn<0)!=(pIn<0) )
		{
			// only one is out
			// calculate edge intersection
			float aa=fabs(aIn);
			float pp=fabs(pIn);
			float pFactor=aa/(aa+pp);
			float aFactor=1-pFactor;
			Vertex &d=dest[dn++];
			d.x=a->x*aFactor+p->x*pFactor;
			d.y=a->y*aFactor+p->y*pFactor;
			d.z=a->z*aFactor+p->z*pFactor;
			d.w=a->w*aFactor+p->w*pFactor;
			d.u=a->u*aFactor+p->u*pFactor;
			d.v=a->v*aFactor+p->v*pFactor;
			d.color=PackedColor(a->color.operator Color()*aFactor+p->color.operator Color()*pFactor);
		}
		if( aIn>=0 )
		{
			dest[dn++]=*a;
		}

		p=a;
		pIn=aIn;
	}
	return dn;
}
} // namespace Poseidon
