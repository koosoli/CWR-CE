#pragma once

#include <Poseidon/AI/VehicleAI.hpp>
#include <Poseidon/Graphics/Rendering/Effects/Smokes.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <Poseidon/Audio/DynSound.hpp>

namespace Poseidon
{
class Fireplace: public VehicleWithAI
{
	typedef VehicleWithAI base;

protected:
	Ref<LightPointOnVehicle> _light;
	Color _lightColor;
	SmokeSource _smoke;
	Ref<SoundObject> _sound;
	bool _burning;

public:
	Fireplace(VehicleType *name);

	bool Burning() const {return _burning;}
	void Inflame(bool fire = true) {_burning = fire;}

	void Simulate( float deltaT, SimulationImportance prec ) override;

	void Sound( bool inside, float deltaT ) override;
	void UnloadSound() override;

	bool QIsManual() const override {return false;}

	bool IsAnimated( int level ) const override;
	bool IsAnimatedShadow( int level ) const override;
	void Animate( int level ) override;
	void Deanimate( int level ) override;

	void GetActions(UIActions &actions, AIUnit *unit, bool now) override;
	RString GetActionName(const UIAction &action) override;
	void PerformAction(const UIAction &action, AIUnit *unit) override;

	LSError Serialize(ParamArchive &ar) override;

	NetworkMessageType GetNMType(NetworkMessageClass cls) const override;
	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx) override;
	float CalculateError(NetworkMessageContext &ctx) override;
	
	USE_CASTING(base)
};

}  // namespace Poseidon
