#pragma once

#include <Poseidon/AI/VehicleAI.hpp>

namespace Poseidon
{
class LaserTargetType: public EntityAIType
{
	friend class LaserTarget;

	typedef EntityAIType base;

	public:
	LaserTargetType( const ParamEntry *param );
	void Load(const ParamEntry &par) override;
	void InitShape() override;
};

class LaserTarget: public EntityAI
{
	typedef EntityAI base;
	Foundation::Time _lastActivation;
	OLink<Object> _designatedTarget;

	void CheckDesignatedTarget(Matrix4Par pos);

	public:
	LaserTarget(EntityAIType *name, bool fullCreate=true);

	bool IgnoreObstacle(Object *obstacle, ObjIntersect type) const override;

	void Init(Matrix4Par pos) override;
	void Move(Matrix4Par transform) override;
	void Move(Vector3Par position) override;

	void Simulate( float deltaT, SimulationImportance prec ) override;

	void Draw( int level, ClipFlags clipFlags, const FrameBase &pos ) override;
	void DrawDiags() override;

	float VisibleSize() const override;
	float VisibleSizeRequired() const override;
	Vector3 AimingPosition() const override;
	LSError Serialize(ParamArchive &ar) override;

	NetworkMessageType GetNMType(NetworkMessageClass cls) const override;
	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx) override;
	float CalculateError(NetworkMessageContext &ctx) override;

	bool QIsManual() const override {return false;}

	USE_CASTING(base)
};

}  // namespace Poseidon
