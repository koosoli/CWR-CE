#pragma once

#include <time.h> // struct tm for FormatLocalizedDate

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/Core/Difficulty.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/IO/Serialization/ParamArchive.hpp>

#ifndef __GNUC__
enum TargetSide;
#endif

namespace Poseidon
{
struct DifficultyDesc; // full definition in Core/UserConfig.hpp

// global variables
// all simulated items have access to them and can use them freely

struct Config
{
	int heapSize;
	int fileHeapSize;
	float horizontZ,objectsZ; // max. distance for drawing landscape and objects
	float tacticalZ,radarZ; // visibility limits 
	float shadowsZ; // max. distance for drawing shadows	
	int maxObjects;
	float trackTimeToLive;
	float invTrackTimeToLive;
	float benchmark;

	int maxLights;

	float lodCoef; // LOD bias for object geometry
	float objectLODLimit;
	float shadowLODLimit;

	int maxCockText,maxLandText,maxObjText,maxAnimText,autoDropText;
	
	int lights;

	int maxSounds;

	bool singleVoice;

#if _ENABLE_CHEATS
	bool super;			//	player is undestructible
#endif

	bool blood;

	int wantBpp,wantW,wantH; // preferred screen parameters

	// Difficulty system (delegated to UserConfig)
	static DifficultyDesc diffDesc[DTN];
	bool IsEnabled(DifficultyType type);
	static void InitDifficulties();
	void LoadDifficulties();
	void SaveDifficulties();
};

struct GameHeader
{
	// game info	
	char filename[80];
	char worldname[80];
	RString filenameReal;

	// player info
	RString playerName;
	RString playerFace;
	RString playerGlasses;
	RString playerSpeaker;
	float playerPitch;
#ifdef __GNUC__
	int playerSide;
#else
	TargetSide playerSide;
#endif
};

const float OneDay=1.0f/365;
const float OneHour=OneDay/24;
const float OneMinute=OneHour/60;
const float OneSecond=OneMinute/60;

class Clock : public SerializeClass
{
protected:
	// use single float representation
	// 1.0 is one year
	float _timeInYear;
	float _changeTime; // actual time relative to _time - _time precision to small
	float _timeOfDay;	
	int _year;

public:
	Clock() : _timeInYear(0), _changeTime(0), _year(1985)
	{
		SetTimeInYear(OneHour*10+OneDay*120);
	}

	float GetTimeInYear() const {return _timeInYear;}
	float GetTimeOfDay() const {return _timeOfDay;}
	int GetYear() const {return _year;}

	void SetTimeInYear( float time )
	{
		_timeInYear = time;
		_changeTime = 0;
		_timeOfDay=fastFmod(_timeInYear,OneDay)*(1.0f/OneDay);
	}
	// Set the time-of-day at full precision from integer fields. _timeOfDay as a
	// [0,1) day fraction keeps ~9ms resolution; deriving it from the year-scale
	// _timeInYear (ULP ~1s) is what made e.g. 07:30:00 read back as :01. Call
	// after SetTimeInYear (which sets the coarse date) to correct the displayed
	// time-of-day without touching _timeInYear (date/season/sun stay identical).
	void SetTimeOfDay( int hour, int minute, int second=0 )
	{
		_changeTime = 0;
		_timeOfDay = (hour*3600 + minute*60 + second) * (1.0f/86400.0f);
	}
	void SetYear(int year)
	{
		_year = year;
	}

	bool AdvanceTime( float deltaT );

	void FormatDate(const char *format, char *buffer);
	static float ScanDateTime(const char *date, const char *time, int &year);

	#ifndef ACCESS_ONLY
	LSError Serialize(ParamArchive &ar) override;
	#else
	LSError Serialize(ParamArchive &ar) {return LSOK;}
	#endif
};

// strftime-style formatting that localizes %a (day) and %b (month) via the stringtable
// (IDS_SUNDAY+wday / IDS_JANUARY+mon) rather than the C locale. Use this instead of strftime
// for any user-facing date so non-English languages and Cyrillic render correctly.
void FormatLocalizedDate(const char *format, const struct tm &tmDate, char *buffer);

struct Globals
{
	Foundation::Time time;
	Foundation::UITime uiTime;
	Clock clock;
	float NetTimeOffset;
	Foundation::Time NetTime() const {return RealToNetTime(time);}
	Foundation::Time GetTime() const {return time;}
	Foundation::Time RealToNetTime(Foundation::Time time) const {return time + NetTimeOffset;}
	Foundation::Time NetToRealTime(Foundation::Time time) const {return time - NetTimeOffset;}
	float dropDown, fullDropDown, fullDropDownChange;
	float drawTreshold, shadowTreshold, reflectTreshold;
	GameHeader header;
	void Init();
	void Clear();
	bool newGame, exit, demo;
};
extern Globals Glob;


Foundation::ErrorMessageLevel GetMaxError();
void ResetErrors();
RString GetMaxErrorMessage();

#define LIGHT_EXPLO 1
#define LIGHT_MISSILE 2
#define LIGHT_STATIC 4

} // namespace Poseidon

// Global-scope aliases for ubiquitously-used names defined in namespace Poseidon
using Poseidon::Glob;
using Poseidon::Globals;
using Poseidon::Clock;
using Poseidon::GameHeader;
using Poseidon::GetMaxError;
using Poseidon::ResetErrors;
using Poseidon::GetMaxErrorMessage;
