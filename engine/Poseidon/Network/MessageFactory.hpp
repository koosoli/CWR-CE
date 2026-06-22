#define MSG_FORMAT(type,name,datatype, compres, defval, description, xfer) \
	format.Add(#name,datatype,compres,defval,description);

#define MSG_TRANSFER(type,name,datatype, compres, defval, description, xfer) \
	TMCHECK(ctx.xfer(indices->name, _##name))

// def. message
#define MSG_DEF(type,name,datatype, compres, defval, description, xfer) \
	type _##name;

// def. indices
#define MSG_IDEF(type,name,datatype, compres, defval, description, xfer) \
	int name;

// scan. indices
#define MSG_ISCAN(type,name,datatype, compres, defval, description, xfer) \
	SCAN(name)

// init indices
#define MSG_IINIT(type,name,datatype, compres, defval, description, xfer) \
	name = -1;

#define DECLARE_NET_MESSAGE(MessageName,MSG_FORMAT_DEF) \
	struct MessageName##Message: public NetworkSimpleObject \
	{ \
		MSG_FORMAT_DEF(MSG_DEF) \
		\
		NetworkMessageType GetNMType(NetworkMessageClass cls) const {return NMT##MessageName;} \
		static NetworkMessageFormat &CreateFormat \
		( \
			NetworkMessageClass cls, \
			NetworkMessageFormat &format \
		); \
		TMError TransferMsg(NetworkMessageContext &ctx); \
	}; \
	 \
	struct Indices##MessageName: public NetworkMessageIndices \
	{ \
		MSG_FORMAT_DEF(MSG_IDEF) \
		Indices##MessageName(); \
		NetworkMessageIndices *Clone() const {return new Indices##MessageName;} \
		void Scan(NetworkMessageFormatBase *format); \
	};

#define DEFINE_NET_MESSAGE(MessageName,MSG_FORMAT_DEF) \
	Indices##MessageName::Indices##MessageName() \
	{ \
		MSG_FORMAT_DEF(MSG_IINIT) \
	} \
	\
	void Indices##MessageName::Scan(NetworkMessageFormatBase *format) \
	{ \
		MSG_FORMAT_DEF(MSG_ISCAN) \
	} \
	\
	NetworkMessageIndices *GetIndices##MessageName() {return new Indices##MessageName();} \
 \
	NetworkMessageFormat &MessageName##Message::CreateFormat \
	( \
		NetworkMessageClass cls, \
		NetworkMessageFormat &format \
	) \
	{ \
		MSG_FORMAT_DEF(MSG_FORMAT) \
		return format; \
	} \
 \
	TMError MessageName##Message::TransferMsg(NetworkMessageContext &ctx) \
	{ \
		NET_ERROR(dynamic_cast<const Indices##MessageName *>(ctx.GetIndices())) \
		const Indices##MessageName *indices = static_cast<const Indices##MessageName *>(ctx.GetIndices()); \
		MSG_FORMAT_DEF(MSG_TRANSFER) \
		return TMOK; \
	}

