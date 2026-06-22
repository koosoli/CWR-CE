#pragma once
#include <Poseidon/Network/NetworkObject.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/World/Scene/Object.hpp>
#include <Poseidon/AI/EntityAI.hpp>


namespace Poseidon
{
struct OperInfoResult
{
	Vector3 _pos;
	float _cost;
	OLink<Object> _house;
	int _index;

	LSError Serialize(ParamArchive &ar);
	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx);
};

class IndicesPath;

class Path: public AutoArray<OperInfoResult>, public SerializeClass 
{
	typedef AutoArray<OperInfoResult> base;

	int _maxIndex; // max. index that is valid
	int _operIndex;
	Foundation::Time _searchTime; // time when the path was constructed (Glob.time)

	bool _onRoad;
	
	float Distance( int index, Vector3Par pos ) const;
	int FindNearest( Vector3Par pos ) const;
	int FindNext( Vector3Par pos ) const;

	public:
	Path();

	// corrected to be in pos plane
	Vector3 PosAtCost( float cost, Vector3Par pos ) const;
	bool InHouseAtCost( float cost, Vector3Par pos ) const;

	// direct calculation
	float CostAtPos( Vector3Par pos ) const;
	float SpeedAtCost( float cost ) const;
	Vector3 PosAtCost( float cost ) const;
	Vector3 NearestPos( Vector3Par pos ) const;

	Vector3 Begin() const;
	Vector3 End() const;
	float EndCost() const;
	
	int GetOperIndex() const {return _operIndex;}
	void SetOperIndex( int index ){_operIndex=index;}

	int GetMaxIndex() const {return _maxIndex;}	
	void SetMaxIndex( int index ){_maxIndex=index;}

	Foundation::Time GetSearchTime() const {return _searchTime;}	
	void SetSearchTime( Foundation::Time time ){_searchTime=time;}

	bool GetOnRoad() const {return _onRoad;}
	void SetOnRoad( bool onRoad ) {_onRoad=onRoad;}

	LSError Serialize(ParamArchive &ar) override;

	static void CreateFormat
	(
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx, IndicesPath *indices);
	float CalculateError(NetworkMessageContext &ctx, IndicesPath *indices);
	
	void Optimize( VehicleWithAI *vehicle ); // drop unnecessary points
	void AvoidCollision( VehicleWithAI *vehicle ); // change path to avoid collisions
};

}  // namespace Poseidon
