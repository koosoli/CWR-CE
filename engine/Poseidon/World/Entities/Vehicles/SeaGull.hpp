#pragma once

#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>
#include <Poseidon/World/Simulation/Animation/Animation.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>

#include <Poseidon/World/Scene/Camera/CameraHold.hpp>

namespace Poseidon
{
class SeaGullAuto;


class SeaGull: public CameraHolder
{
	typedef CameraHolder base;

	protected:
	Ref<IWave> _sound;
	SoundPars _soundPars;

	float _wingPhase;
	float _wingSpeed;
	float _wingBase; // which kind of animation - landed/winging
	Foundation::Time _nextCreek;

	// rotor force is wing speed
	float _rpm,_rpmWanted; // landing control
	float _mainRotor,_mainRotorWanted; // main rotor thrust
	float _cyclicForwardWanted;
	float _cyclicAsideWanted; // main rotor thrust direction
	float _wingDive,_wingDiveWanted; // use wings to control forward movement
	float _thrust,_thrustWanted;

	bool _landContact;
		
	public:
	SeaGull();
	~SeaGull() override;
	
	bool IsAnimated( int level ) const override; // appearence changed with Animate
	bool IsAnimatedShadow( int level ) const override; // shadow changed with Animate
	void Animate( int level ) override;
	void Deanimate( int level ) override;

	void Simulate( float deltaT, SimulationImportance prec ) override;
	void Sound( bool inside, float deltaT ) override;
	void UnloadSound() override;

	bool OcclusionFire() const override {return false;}
	bool OcclusionView() const override {return false;}

	float TrackingSpeed() const override {return 100;}
	float OutsideCameraDistance( CameraType camType ) const override {return 3;}
	Vector3 ExternalCameraPosition( CameraType camType ) const override;

	void QuickStart();

	// no load/save
	bool MustBeSaved() const override {return false;}

	USE_CASTING(base)
};

class SeaGullAuto: public SeaGull
{
	typedef SeaGull base;

	protected:
	Vector3 _mouseDirWanted; // mouse control
	Vector3 _pilotSpeed;
	float _pilotHeading;
	float _pilotHeight;
	float _dirCompensate;  // how much we compensate for estimated change
	bool _pilotHeadingSet;
	// esp. when landing

	// helpers for keyboard control
	bool _pressedForward,_pressedBack; // recognize fast speed-up, fast brake
	bool _pressedUp,_pressedDown;

	Foundation::Time _lastPilotTime; // avoid calculating pilot too often

	AutopilotState _state;
	
	public:
	SeaGullAuto( void *pilot );

	void Simulate( float deltaT, SimulationImportance prec ) override;
	void KeyboardPilot(AIUnit *unit, float deltaT );
	void JoystickPilot( float deltaT );

	void AvoidGround( float minHeight );
	void Autopilot
	(
		Vector3Par target, Vector3Par tgtSpeed, // target
		Vector3Par direction, Vector3Par speed // wanted values
	);
	void ResetAutopilot();

	void Draw( int forceLOD, ClipFlags clipFlags, const FrameBase &pos ) override;
	void DrawDiags() override;

	void MakeLanded();
	void MakeAirborne( float height );

	bool IsVirtualX( CameraType camType ) const override;
	void AimDriver(Vector3Val val) override;
	
	virtual void CamControl( float deltaT );

	// CameraHolder implementation
	void Command( RString mode ) override;
	void Commit( float time ) override;

	// Multiplayer support
	NetworkMessageType GetNMType(NetworkMessageClass cls) const override;
	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx) override;
	float CalculateError(NetworkMessageContext &ctx) override;

	USE_FAST_ALLOCATOR
};

}  // namespace Poseidon
