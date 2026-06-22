#pragma once

#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
namespace Poseidon::Foundation
{
class AbstractTime
{
protected:
	int _time;	// time in milliseconds
public:
	__forceinline AbstractTime() {_time = 0;}
	__forceinline AbstractTime(int time) {_time = time;}

	__forceinline int toInt() const {return _time;}
	__forceinline float toFloat() const {return 1e-3f * _time;}
	__forceinline int MsFrac() const
	{ // millisecond fraction
		return _time%1000;
	}
protected:
	float Diff( const AbstractTime &x ) const;
};

#define UITimeVal const UITime &

class UITime : public AbstractTime
{
public:
	__forceinline UITime() : AbstractTime() {}
	__forceinline explicit UITime(int time) : AbstractTime(time) {}
	__forceinline UITime(UITimeVal src) {_time = src._time;}

#ifdef NDEBUG
	__forceinline  void operator +=(float diff);
	__forceinline  void operator -=(float diff);
	__forceinline  float operator -(UITimeVal src) const;
	__forceinline  UITime operator -(float diff) const;
	__forceinline  UITime operator +(float diff) const;
#else
	void operator +=(float diff);
	void operator -=(float diff);
	float operator -(UITimeVal src) const;
	UITime operator -(float diff) const;
	UITime operator +(float diff) const;
#endif

	__forceinline void operator =(UITimeVal src) {_time = src._time;}
	__forceinline bool operator ==(UITimeVal arg) const {return _time == arg._time;}
	__forceinline bool operator !=(UITimeVal arg) const {return _time != arg._time;}
	__forceinline bool operator <(UITimeVal arg) const {return _time < arg._time;}
	__forceinline bool operator >(UITimeVal arg) const {return _time > arg._time;}
	__forceinline bool operator <=(UITimeVal arg) const {return _time <= arg._time;}
	__forceinline bool operator >=(UITimeVal arg) const {return _time >= arg._time;}

	__forceinline float Diff(UITimeVal x ) const {return AbstractTime::Diff(x);}
};

#define TimeVal const Time &

// in-game time (full, millisecond resolution)
class Time : public AbstractTime
{
public:
	__forceinline Time() : AbstractTime() {}
	__forceinline explicit Time(int time) : AbstractTime(time) {}
	__forceinline Time(TimeVal src) {_time = src._time;}

	__forceinline Time Floor() const {return Time(_time-MsFrac());}
	__forceinline Time AddMs(int ms) const {return Time(_time+ms);}

#ifdef NDEBUG
	__forceinline void operator +=(float diff);
	__forceinline void operator -=(float diff);
	__forceinline float operator -(TimeVal src) const;
	__forceinline Time operator -(float diff) const;
	__forceinline Time operator +(float diff) const;
#else
	void operator +=(float diff);
	void operator -=(float diff);
	float operator -(TimeVal src) const;
	Time operator -(float diff) const;
	Time operator +(float diff) const;
#endif

	__forceinline void operator =(TimeVal src) {_time = src._time;}
	__forceinline bool operator ==(TimeVal arg) const {return _time == arg._time;}
	__forceinline bool operator !=(TimeVal arg) const {return _time != arg._time;}
	__forceinline bool operator <(TimeVal arg) const {return _time < arg._time;}
	__forceinline bool operator >(TimeVal arg) const {return _time > arg._time;}
	__forceinline bool operator <=(TimeVal arg) const {return _time <= arg._time;}
	__forceinline bool operator >=(TimeVal arg) const {return _time >= arg._time;}

	__forceinline float Diff( TimeVal x ) const {return AbstractTime::Diff(x);}
};

#define UITIME_MAX		::Poseidon::Foundation::UITime(INT_MAX)
#define UITIME_MIN		::Poseidon::Foundation::UITime(-INT_MAX)
#define TIME_MAX			::Poseidon::Foundation::Time(INT_MAX)
#define TIME_MIN			::Poseidon::Foundation::Time(-INT_MAX)

#ifdef NDEBUG

__forceinline  void UITime::operator +=(float diff) {_time += ::toLargeInt(1e3f * diff);}
__forceinline  void UITime::operator -=(float diff) {_time -= ::toLargeInt(1e3f * diff);}
__forceinline  float UITime::operator -(UITimeVal src) const {return 1e-3f * (_time - src._time);}
__forceinline  UITime UITime::operator -(float diff) const {UITime ret = *this; ret -= diff; return ret;}
__forceinline  UITime UITime::operator +(float diff) const {UITime ret = *this; ret += diff; return ret;}

__forceinline  void Time::operator +=(float diff) {_time += ::toLargeInt(1e3f * diff);}
__forceinline  void Time::operator -=(float diff) {_time -= ::toLargeInt(1e3f * diff);}
__forceinline  float Time::operator -(TimeVal src) const {return 1e-3f * (_time - src._time);}
__forceinline  Time Time::operator -(float diff) const {Time ret = *this; ret -= diff; return ret;}
__forceinline  Time Time::operator +(float diff) const {Time ret = *this; ret += diff; return ret;}

#endif


#define TIMESEC_MAX ::Poseidon::Foundation::TimeSec(SHRT_MAX)
#define TIMESEC_MIN ::Poseidon::Foundation::TimeSec(-SHRT_MAX)

// in-game time (compact, second resolution)
class TimeSec 
{
	short _time;

	public:
	__forceinline explicit TimeSec(short time) {_time=time;}
	explicit TimeSec(Time time)
	{
		int timeSec =time.toInt()/1000;
		saturate(timeSec,-SHRT_MAX,SHRT_MAX);
		_time = timeSec;
	}
	operator Time() const {return Time(_time*1000);}

	__forceinline void operator =(TimeSec src) {_time = src._time;}
	__forceinline bool operator ==(TimeSec arg) const {return _time == arg._time;}
	__forceinline bool operator !=(TimeSec arg) const {return _time != arg._time;}
	__forceinline bool operator <(TimeSec arg) const {return _time < arg._time;}
	__forceinline bool operator >(TimeSec arg) const {return _time > arg._time;}
	__forceinline bool operator <=(TimeSec arg) const {return _time <= arg._time;}
	__forceinline bool operator >=(TimeSec arg) const {return _time >= arg._time;}
};


} // namespace Poseidon::Foundation
