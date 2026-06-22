#pragma once


#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/AI/AIUnit.hpp>
#include <Poseidon/AI/AIGroup.hpp>
#include <Poseidon/Foundation/Containers/BoolArray.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
namespace Poseidon
{
enum SentenceParamType
{
	SPTWord,			// not used
	SPTText,			// const char * - text will be spelled
	SPTNumber,		// int - number will be spelled
	SPTFormat,		// const char *format, int - number will be spelled in selected format
	SPTUnit,			// AIUnit * - unit's ID
	SPTUnitList,	// AIGroup *, PackedBoolArray - unit's IDs or ALL
	SPTTargetList,	// not used
	SPTWordText
};

struct SentenceParamSimple
{
	SentenceParamType type;
	int number;
	TargetSide side;

	RString text;	// cannot be member of union
};

class SentenceParamSimpleList : public RefCount, public AutoArray<SentenceParamSimple>
{
public:
	typedef AutoArray<SentenceParamSimple> base;
	TargetSide _side;
	
	int Add(RString text, TargetSide side);
};

struct SentenceParam
{
	SentenceParamType type;
#ifdef __GNUC__
	union
	{
		int number;
		AIUnit *unit;
		AIGroup *grp;
	};
	PackedBoolArray list;
	bool wholeCrew;
#else
	union
	{
		struct
		{
			int number;
		};
		AIUnit *unit;
		struct
		{
			AIGroup *grp;
			PackedBoolArray list;
			bool wholeCrew;
		};
	};
#endif
	RString text;														// cannot be member of union
	RString text2;													// cannot be member of union
	Ref<SentenceParamSimpleList> targets; 	// cannot be member of union
};


class SentenceParams : public StaticArray<SentenceParam>
{
	typedef StaticArray<SentenceParam> base;

public:
	int Add(RString text)
	{
		int index = base::Add();
		Set(index).type = SPTText;
		Set(index).text = text;
		return index;
	}
	int Add(int number)
	{
		int index = base::Add();
		Set(index).type = SPTNumber;
		Set(index).number = number;
		return index;
	}
	int Add(RString format, int number)
	{
		int index = base::Add();
		Set(index).type = SPTFormat;
		Set(index).text = format;
		Set(index).number = number;
		return index;
	}
	int Add(AIUnit *unit)
	{
		int index = base::Add();
		Set(index).type = SPTUnit;
		Set(index).unit = unit;
		return index;
	}
	int Add(AIGroup *grp, PackedBoolArray list, bool wholeCrew)
	{
		int index = base::Add();
		Set(index).type = SPTUnitList;
		Set(index).grp = grp;
		Set(index).list = list;
		Set(index).wholeCrew = wholeCrew;
		return index;
	}
	int Add(AISubgroup *subgrp, bool wholeCrew);
	int Add(OLinkArray<AIUnit> &units, bool wholeCrew);
	int Add(Ref<SentenceParamSimpleList> targets);

	void AddWord(RString say, RString write)
	{
		int index = base::Add();
		Set(index).type = SPTWordText;
		Set(index).text = say;
		Set(index).text2 = write;
		//return index;
	}
	void AddAzimutRelDir(Vector3Par dir);
	void AddAzimutDir(Vector3Par dir);
	void AddAzimut(Vector3Par pos);
	void AddDistance(float dist);
	void AddRelativePosition(Vector3Par dir);
};

} // namespace Poseidon
