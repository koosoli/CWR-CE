#pragma once

#include <Poseidon/IO/ParamFileExt.hpp>
#include <Poseidon/World/Entities/Vehicles/Vehicle.hpp>


namespace Poseidon
{
class SoundEntry: public SoundPars
{
	// inherited: sfxfile, vol, frq
	friend class DynSound;
	friend class DynSoundSource;
	friend class DynSoundObject;
	protected:
	float _probab;
	float _min_delay,_mid_delay,_max_delay;
};

void FindSFX(RString name, SoundEntry& emptySound, Foundation::AutoArray<SoundEntry>& sounds);

struct DynSoundName
{
	RString name;
	RString location;
};

class DynSound: public RefCountWithLinks
{
	friend class DynSoundBank;

	RString _name;
	AutoArray<SoundEntry> _sounds;
	SoundEntry _emptySound;

	public:
	DynSound();
	DynSound( const char *name );
	void Load( const char *name );

	static SoundEntry LoadEntry( const ParamEntry &entry );

	const SoundEntry &SelectSound( float probab ) const;
	bool IsOneLoopingSound() const;
	const char *GetName() const {return _name;}

};


} // namespace Poseidon
#include <Poseidon/Foundation/Containers/BankArray.hpp>
namespace Poseidon
{


template <>
struct BankTraits<DynSound>
{
	typedef const char *NameType;
	static int CompareNames(const char *n1, const char *n2)
	{
		return strcmpi(n1,n2);
	}
	// store only links - this guarantees releasing when mission changes etc.
	typedef LinkArray<DynSound> ContainerType;
};

class DynSoundBank: public BankArray<DynSound>
{
};

extern DynSoundBank DynSounds;

struct TitlesItem
{
	Poseidon::Foundation::Time time;
	RString text;
};

class SoundObject: public RemoveOLinks
{
	protected:
	Ref<IWave> _sound;
	OLink<Object> _object;
	OLink<AIUnit> _sender;
	RString _soundName;
	bool _hasObject;
	bool _looped;
	bool _paused;
	bool _waiting;
	bool _forceTitles;

	AutoArray <TitlesItem> _titles;
	int _index;

	Ref<Font> _titlesFont;
	float _titlesSize;

	float _maxTitlesDistance;
	float _speed;

	void LoadSound();
	void StartSound();
	
	public:
	SoundObject(RString name, Object *source, bool looped = false, float maxTitlesDistance = 100.0f, float speed = 1.0f);
	bool Simulate(float deltaT, SimulationImportance prec);

	void SimulateTitles();

	IWave *GetWave() {return _sound;}
	void Pause(bool pause = true) {_paused = pause;}

	bool IsWaiting() const {return _waiting;}
	const AIUnit *GetSender() const;
};

class SoundOnVehicle : public Vehicle
{
protected:
	Ref<SoundObject> _sound;

public:
	SoundOnVehicle(const char *name, Object *source, float maxTitlesDistance = 100.0f, float speed = 1.0f);
	void Simulate( float deltaT, SimulationImportance prec ) override;
	bool MustBeSaved() const override {return false;}
	bool Invisible() const override {return true;}
};

class DynSoundObject: public RefCount
{
	protected:
	Ref<DynSound> _dynSound;
	Link<IWave> _sound;

	float _timeToLive; // how long current sound will be played

	public:
	DynSoundObject( const char *name );
	~DynSoundObject() override;

	void Simulate( Object *source, float deltaT, SimulationImportance prec );
	void StopSound();
};

class DynSoundSource: public Vehicle
{
	Ref<DynSoundObject> _dynSound;

	public:
	DynSoundSource( const char *name );
	~DynSoundSource() override;

	void Simulate( float deltaT, SimulationImportance prec ) override;
	void StopSound();

	// no load/save
	bool MustBeSaved() const override {return false;}
};



} // namespace Poseidon
