#pragma once

#include <Poseidon/Foundation/Types/LLinks.hpp>
#include <Poseidon/Foundation/Types/EnumDecl.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>

#define OLink LLink
#define OLinkArray LLinkArray
#define RemoveOLinks RemoveLLinks
//#define OLink Link
//#define OLinkArray LinkArray
//#define RemoveOLinks RefCountWithLinks

class NetworkMessageContext;
struct NetworkMessage;
class NetworkMessageFormatBase;
class NetworkMessageFormat;
DECL_ENUM(TMError)
DECL_ENUM(NetworkMessageType)

// message update class for objects
enum NetworkMessageClass
{
	NMCCreate = -1,
	NMCUpdateFirst = 0,	// update values are used as index into array
	NMCUpdateGeneric = NMCUpdateFirst,
	NMCUpdatePosition,
	NMCUpdateDammage, // dammage and hit update
	NMCUpdateN
};

#include <Poseidon/Foundation/Strings/RString.hpp>

// simple network object
// Base class for guaranteed message structures.
class NetworkSimpleObject
{
public:
	virtual ~NetworkSimpleObject() {}
	// Returns message type for this object and message class
	virtual NetworkMessageType GetNMType(NetworkMessageClass cls = NMCCreate) const = 0;
	// Create message format for this object and given class
	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	) {return format;}
	// Message update from / to this object
	virtual TMError TransferMsg(NetworkMessageContext &ctx) = 0;
};

// unique identifier of network objects
struct NetworkId
{
	// Player id of client where network object was created
	int creator;
	// Unique id of network object on given client
	int id;

	NetworkId() {creator = 0; id = 0;}
	NetworkId(int c, int i) {creator = c; id = i;}
	void operator =(const NetworkId &src)
	{
		creator = src.creator;
		id = src.id;
	}
	bool operator ==(const NetworkId &with) const
	{
		return with.creator == creator && with.id == id;
	}
	bool operator !=(const NetworkId &with) const
	{
		return with.creator != creator || with.id != id;
	}
	
	// Null (unassigned) network id
	static NetworkId Null() {return NetworkId();}
	// Check if id value is null (unassigned)
	bool IsNull() const {return creator == 0;}
};

// helper base class for faster TransferMsg
// Base class for the Indices... classes; each holds indices of items in an array.
class NetworkMessageIndices
{
public:
	NetworkMessageIndices() {};
	virtual ~NetworkMessageIndices() {};

	// Create copy (clone) of this object
	virtual NetworkMessageIndices *Clone() const = 0;
	// Initialize members from message format
	virtual void Scan(NetworkMessageFormatBase *format) = 0;
};

// network message indices for NetworkObject
class IndicesNetworkObject : public NetworkMessageIndices
{
public:
	int objectCreator;
	int objectId;
	int objectPosition;
	int guaranteed;
	
	IndicesNetworkObject();
	NetworkMessageIndices *Clone() const override {return new IndicesNetworkObject;}
	void Scan(NetworkMessageFormatBase *format) override;
};

#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/Foundation/Strings/StrFormat.hpp>

// updateable network object
// Base class for all objects with automatic update over the network in multiplayer.
class NetworkObject : public RemoveOLinks, public NetworkSimpleObject
{
protected:
	// time when last update from server arrived
	Poseidon::Foundation::Time _lastUpdateTime;

	// time when prediction should stop (calculated by GetMaxPredictionTime() function)
	Poseidon::Foundation::Time _maxPredictionTime;

public:
	NetworkObject();

	// Set unique id of network object
	virtual void SetNetworkId(NetworkId &id) = 0;
	// Return unique id of network object
	virtual NetworkId GetNetworkId() const = 0;
	// Returns current position of this object
	virtual Vector3 GetCurrentPosition() const = 0;
	// Returns speaker position for this object
	virtual Vector3 GetSpeakerPosition() const {return GetCurrentPosition();}
	// Enable / disable random lip movement (enabled during direct speaking chat)
	virtual void SetRandomLip(bool set = true) {}
	// Return if object is local on this client
	virtual bool IsLocal() const = 0;
	// Set if object is local on this client
	virtual void SetLocal(bool local = true)= 0;
	// Destroy (remote) object
	virtual void DestroyObject() = 0;

	// Get debugging name (debugging only)
	virtual RString GetDebugName() const = 0;

	// How long is client side prediction allowed to predict this object
	virtual float GetMaxPredictionTime(NetworkMessageContext &ctx) const;

	// Check if prediction should be frozen
	virtual bool CheckPredictionFrozen() const;

	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx) override = 0;
	// Calculate difference between given message and current state
	virtual float CalculateError(NetworkMessageContext &ctx);
	// Calculate coeficient error must be multiplied by
	// Coefficient is 1 for near objects, decreasing toward zero for distant ones.
	static float CalculateErrorCoef(Vector3Par position, Vector3Par cameraPosition);
};

inline RString NetworkIdToNetId(const NetworkId& id)
{
	return Format("%d:%d", id.creator, id.id);
}

