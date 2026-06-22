#pragma once

#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>


namespace Poseidon
{
struct RandomShapeInfo
{
	Ref<LODShapeWithShadow> _shape;
	float _probab;
};

#if SUPPORT_RANDOM_SHAPES


class RandomShapeType: public EntityType
{
	typedef EntityType base;

	friend class RandomShape;

	AutoArray<RandomShapeInfo> _shapes;

	public:
	RandomShapeType(const ParamEntry *param);
	~RandomShapeType();

	void Load(const ParamEntry &cfg);

	void InitShape();
	void DeinitShape();

	LODShapeWithShadow *SelectShape(float x) const;
};

class RandomShape: public ObjectTyped
{
	typedef ObjectTyped base;

	public:
	RandomShape(const RandomShapeType *type, int id);

	const RandomShapeType *Type() const
	{
		return static_cast<const RandomShapeType *>(GetEntityType());
	}
	void Draw( int forceLOD, ClipFlags clipFlags, const FrameBase &pos );

	LODShapeWithShadow *SelectShape(Vector3Val pos) const;
	LODShapeWithShadow *GetShapeOnPos(Vector3Val pos) const;
};

#endif
} // namespace Poseidon
