#pragma once

#include <Poseidon/Core/Types.hpp>
#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/Foundation/Containers/SmallArray.hpp>
#include <Poseidon/Foundation/Containers/BigArray.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>

#include <Poseidon/Foundation/Containers/Array2D.hpp>

namespace Poseidon
{
class RoadLink
{
	enum {NCon=4};
	Vector3 _pos[NCon];
	RoadLink *_con[NCon];
	int _nCon;

	int _locks;
	OLink<Object> _object;

	public:
	RoadLink( Object *object, Vector3 *pos, int c );
	~RoadLink(){}

	__forceinline int NConnections() const {return _nCon;}
	__forceinline const Vector3 *PosConnections() const {return _pos;}
	__forceinline RoadLink * const *Connections() const {return _con;}
	__forceinline void SetConnection( int i, RoadLink *con ){_con[i]=con;}

	void AddConnection( Vector3Par pos, RoadLink *con );
	Vector3 GetCenter() const;
	float NearestConnectionDist2(Vector3Val pos) const;

	bool IsLocked() const {return _locks > 0 || !_object || _object->IsDammageDestroyed();}
	void Lock() {_locks++;}
	void Unlock() {_locks--;}

	Object *GetObject() const {return _object;}
	bool IsInside(Vector3Val pos, float size) const;
	
	USE_FAST_ALLOCATOR
};

class RoadListFull: public SmallArray< SRef<RoadLink> >
{
	public:
	USE_FAST_ALLOCATOR
};

// assume many RoadLists will be empty
class RoadList
{
	SRef<RoadListFull> _list;

	public:
	int Size() const {return ( _list ? _list->Size() : 0 );}
	RoadLink *operator [] ( int i ) const {PoseidonAssert(_list);return (*_list)[i];}
	int Add( RoadLink *object )
	{
		if( !_list ) _list=new RoadListFull;
		return _list->Add(object);
	}
	void Replace( int index, RoadLink *link ){_list->Set(index)=link;}
	void Delete( int index )
	{
		PoseidonAssert( _list );
		_list->Delete(index);
		if( _list->Size()==0 ) _list.Free();
	}
	void Compact()
	{
		if( _list )
		{
			_list->Compact();
			if( _list->Size()==0 ) _list.Free();
		}
	}
	void Clear() {_list.Free();}
};

#define BIG_ARRAY BigArrayNormal
//#define BIG_ARRAY BigArray

typedef StaticArray<Vector3> RoadPathArray;


class RoadNet: public RefCount
{
	// road information kept separate from objects (in object-like manner)

	//BIG_ARRAY<RoadList,LandRange,LandRange> _roads; // all roads
	Array2D<RoadList> _roads;
	//BIG_ARRAY<RoadList,LandRange,LandRange> _roads; // all roads

	inline RoadList &SelectRoadList( float x, float z );

	public:
	RoadNet();
	~RoadNet() override;

	void Scan( Landscape *land ); // scan landscape
	void Connect(); // connect as necessary
	void Optimize(); // merge small straight elements
	void Compact(); // optimize memory picture

	void Build( Landscape *land ); // all steps together

	bool SearchPath( Vector3Par from, Vector3Par to, RoadPathArray &path, float prec = 0) const;
	const RoadLink *IsOnRoad( Vector3Par pos, float size ) const;
	Vector3 GetNearestRoadPoint( Vector3Par pos ) const;
	inline RoadList &GetRoadList( int xx, int zz ); // used for diags

	bool IsLocked( Vector3Par pos, float size ) const;
};

inline RoadList &RoadNet::GetRoadList( int xx, int zz )
{
	if( !InRange(xx,zz) )
	{
		// find nearest in-range square and use it
		if( xx<0 ) xx=0;else if( xx>LandRange-1 ) xx=LandRange-1;
		if( zz<0 ) zz=0;else if( zz>LandRange-1 ) zz=LandRange-1;
		PoseidonAssert( InRange(xx,zz) );
	}
	return _roads(xx,zz);
}

extern SRef<RoadNet> GRoadNet;

}  // namespace Poseidon
