#pragma once

#include <Poseidon/World/Entities/Vehicles/TankOrCar.hpp>

#include <Poseidon/World/Entities/Vehicles/Transport.hpp>
#include <Poseidon/World/Entities/Weapons/Shots.hpp>

#include <Poseidon/World/Simulation/Animation/RtAnimation.hpp>

#include <Poseidon/World/Entities/Infantry/Wounds.hpp>

namespace Poseidon
{
enum MotorcycleWheel
{
	MCFWheel,
	MCBWheel,
	MaxMCWheels,
	MCNoWheel=MaxMCWheels
};


class MotorcycleType: public TankOrCarType
{
	typedef TankOrCarType base;
	friend class Motorcycle;

	protected:
	
	Matrix4 _toWheelAxis; // transformation
	AnimationWithCenter _drivingWheel;

	Vector3 _steeringPoint;
	float _wheelCircumference;
	float _turnCoef;
	float _terrainCoef;

	AnimationAnimatedTexture _animFire;
	TurretType _turret;
	bool _hasTurret;
	bool _isBicycle;

	AnimationWithCenter _frontWheel;
	AnimationWithCenter _backWheel;
	AnimationRotation _pedals,_pedalR,_pedalL;
	AnimationWithCenter _frontWheelDamper;
	Animation _backWheelDamper;

	Buffer<MotorcycleWheel> _whichWheelContact; // conversion from landcontact
	AnimationWithCenter _support;

	WoundTextureSelections _glassDammageHalf;
	WoundTextureSelections _glassDammageFull;

	AnimationWithCenter *_wheels[MaxMCWheels];

	PlateInfos _plateInfos;

	HitPoint _glassRHit;
	HitPoint _glassLHit;
	HitPoint _bodyHit;
	HitPoint _fuelHit;

	HitPoint _wheelFHit;
	HitPoint _wheelBHit;

	Ref<WeightInfo> _weights;
	Ref<Skeleton> _skeleton;

	public:
	MotorcycleType( const ParamEntry *param );
	void Load(const ParamEntry &par) override;
	void InitShape() override;
};

class Motorcycle: public TankOrCar
{
	typedef TankOrCar base;

	protected:
	Link<IWave> _hornSound;

	Turret _turret;

	WeaponLightSource _mGunFire;
	int _mGunFireFrames;
	Foundation::UITime _mGunFireTime;
	int _mGunFirePhase;
	WeaponCloudsSource _mGunClouds;

	RString _plateNumber;
	
	float _wheelPhase;
	float _dampers[MaxMCWheels];
	
	float _reverseTimeLeft; // force reverse state
	float _forwardTimeLeft; // force forward state

	TrackOptimized _track;
	
	// simulate gear box
	
	// thrust is relative, in range (-1.0,1.0)
	// actually it is engine RPM?
	float _thrustWanted,_thrust; // accelerate 

	// assume steering point the middle of front whee axe (see MotorcycleType)
	// _turn specifies side offset of steering point
	// when car moves 1 m forward - and therefore it can be viewed
	// as tangens of angle of front wheels
	// each time angular velocity change is calculated,
	// actual radial velocity of

	float _turnWanted,_turn; // turn

	float _turnIncreaseSpeed,_turnDecreaseSpeed; // keyboard different from stick/auto
	
	float _support;

	public:
	Motorcycle( VehicleType *name, Person *driver );

	const MotorcycleType *Type() const
	{
		return static_cast<const MotorcycleType *>(GetType());
	}

	Vector3 Friction( Vector3Par speed );

	float Rigid() const override {return 0.3;} // how much energy is transfered in collision
	bool HasTurret() const {return Type()->_hasTurret;}
	bool IsBlocked() const;
	void PlaceOnSurface(Matrix4 &trans) override;

	bool IsStopped() const override;
	void MoveWeapons(float deltaT);
	void SimulateOneIter( float deltaT, SimulationImportance prec );
	void Simulate( float deltaT, SimulationImportance prec ) override;
	Matrix4 InsideCamera( CameraType camType ) const override;
	Vector3 ExternalCameraPosition( CameraType camType ) const override;
	int InsideLOD( CameraType camType ) const override;
	bool IsGunner( CameraType camType ) const override;
	bool IsContinuous( CameraType camType ) const override;
	bool HasFlares( CameraType camType ) const override;
	void SimulateHUD(CameraType camType,float deltaT) override;
	void Draw( int level, ClipFlags clipFlags, const FrameBase &pos ) override;
	void DrawProxies
	(
		int level, ClipFlags clipFlags,
		const Matrix4 &transform, const Matrix4 &invTransform,
		float dist2, float z2, const LightList &lights
	) override;

	void AnimateSpeedIndicator(Matrix4 &trans, int level) override;
	void AnimateMatrix(Matrix4 &mat, int level, int selection) const override;

	RString GetActionName(const UIAction &action) override;
	void PerformAction(const UIAction &action, AIUnit *unit) override;
	void GetActions(UIActions &actions, AIUnit *unit, bool now) override;

	Matrix4 TurretTransform() const;
	Matrix4 GunTurretTransform() const;

	float OutsideCameraDistance( CameraType camType ) const override {return 20;}

	float GetGlassBroken() const;

	void DammageAnimation( int level );
	void DammageDeanimation( int level );

	bool IsAnimated( int level ) const override; // appearence changed with Animate
	bool IsAnimatedShadow( int level ) const override; // shadow changed with Animate
	void Animate( int level ) override;
	void Deanimate( int level ) override;

	Vector3 AnimatePoint( int level, int index ) const override;

	RString GetPlateNumber() const override {return _plateNumber;}
	void SetPlateNumber( RString plate ) override {_plateNumber=plate;}

	void Sound( bool inside, float deltaT ) override;
	void UnloadSound() override;

	bool IsPossibleToGetIn() const override;
	bool IsAbleToMove() const override;
	bool IsCautious() const override;

	float GetEngineVol( float &freq ) const override;
	float GetEnvironVol( float &freq ) const override;

	float DriverAnimSpeed() const override;
	float CommanderAnimSpeed() const override;
	float GunnerAnimSpeed() const override;
	float CargoAnimSpeed(int position) const override;

	float Thrust() const override {return fabs(_thrust);}
	float ThrustWanted() const override {return fabs(_thrustWanted);}

	float TrackingSpeed() const override {return 80;}

	void Eject(AIUnit *unit) override;
	void JoystickPilot( float deltaT );
	void SuspendedPilot(AIUnit *unit, float deltaT ) override;
	void KeyboardPilot(AIUnit *unit, float deltaT ) override;
	void FakePilot( float deltaT ) override;
	void AIPilot(AIUnit *unit, float deltaT ) override;

	RString DiagText() const override;

	float GetPathCost( const GeographyInfo &info, float dist ) const override;
	void FillPathCost( Path &path ) const override;

	float GetFieldCost( const GeographyInfo &info ) const override;
	float GetCost( const GeographyInfo &info ) const override;
	float GetCostTurn( int difDir ) const override;

	bool FireWeapon( int weapon, TargetType *target ) override;
	void FireWeaponEffects(int weapon, const Magazine *magazine,EntityAI *target) override;
	Vector3 GetCameraDirection( CameraType camType ) const override;
	void LimitCursor( CameraType camType, Vector3 &dir ) const override;

	// horn is always aimed
	bool AimWeapon( int weapon, Vector3Par direction ) override;
	bool AimWeapon( int weapon, Target *target ) override;
	Vector3 GetWeaponDirection( int weapon ) const override;
	Vector3 GetWeaponCenter( int weapon ) const override;

	LSError Serialize(ParamArchive &ar) override;
	
	NetworkMessageType GetNMType(NetworkMessageClass cls) const override;
	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx) override;
	float CalculateError(NetworkMessageContext &ctx) override;
};

}  // namespace Poseidon
