#pragma once

#include <Poseidon/World/Scene/Object.hpp>

namespace Poseidon
{
class ObjectLine
{
	public:
	static LODShapeWithShadow *CreateShape();
	static void SetPos( LODShapeWithShadow *lShape, Vector3Par beg, Vector3Par end );
};

class ObjectLineDiag: public ObjectColored
{
	typedef ObjectColored base;
	 
	public:
	ObjectLineDiag( LODShapeWithShadow *shape );
	void Draw( int level, ClipFlags clipFlags, const FrameBase &pos ) override;

	USE_FAST_ALLOCATOR
};

}  // namespace Poseidon
