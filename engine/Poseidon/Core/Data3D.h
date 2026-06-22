#pragma once

#pragma pack(push,4)

typedef float DataReal;

// simple structures to hold data

#include <Poseidon/Foundation/Math/Math3DP.hpp>

namespace Poseidon
{

typedef Vector3P DataVec;

typedef DataVec DataPoint;
typedef DataVec DataNormal;

// special flags
#define POINT_ONLAND    0x1
#define POINT_UNDERLAND 0x2
#define POINT_ABOVELAND 0x4
#define POINT_KEEPLAND  0x8
#define POINT_LAND_MASK 0xf

#define POINT_DECAL      0x100
#define POINT_VDECAL     0x200
#define POINT_DECAL_MASK 0x300

#define POINT_NOLIGHT    0x10 // active colors
#define POINT_AMBIENT    0x20
#define POINT_FULLLIGHT  0x40
#define POINT_HALFLIGHT  0x80
#define POINT_LIGHT_MASK 0xf0

#define POINT_NOFOG     0x1000 // active colors
#define POINT_SKYFOG    0x2000
#define POINT_FOG_MASK  0x3000

#define POINT_USER_MASK  0xff0000
#define POINT_USER_STEP  0x010000

#define POINT_SPECIAL_MASK   0xf000000
#define POINT_SPECIAL_HIDDEN 0x1000000

#define FACE_NOLIGHT             0x1 // active colors
#define FACE_AMBIENT             0x2
#define FACE_FULLLIGHT           0x4
#define FACE_BOTHSIDESLIGHT     0x20 // Objektiv normal calculation
#define FACE_SKYLIGHT           0x80
#define FACE_REVERSELIGHT   0x100000 // Objektiv normal calculation
#define FACE_FLATLIGHT      0x200000 // Objektiv normal calculation
#define FACE_LIGHT_MASK     0x3000a7

#define FACE_ISSHADOW     0x8
#define FACE_NOSHADOW    0x10
#define FACE_SHADOW_MASK 0x18

#define FACE_Z_BIAS_MASK 0x300
#define FACE_Z_BIAS_STEP 0x100

#define FACE_FANSTRIP_MASK    0xf0000
#define FACE_BEGIN_FAN        0x10000
#define FACE_BEGIN_STRIP      0x20000
#define FACE_CONTINUE_FAN     0x40000
#define FACE_CONTINUE_STRIP   0x80000

#define FACE_DISABLE_TEXMERGE 0x1000000

#define FACE_USER_MASK  0xfe000000
#define FACE_USER_STEP  0x02000000
#define FACE_USER_SHIFT 25



struct DataVertex
{
	int point,normal;
	DataReal mapU,mapV;
};


#define MAX_DATA_POLY 4

struct DataFace
{
	// all faces are white - color is done via textures
	char texture[32]; // warning - name length is limited!
	int n;
	DataVertex vs[MAX_DATA_POLY];
};


struct DataHeader
{
	char magic[4]; // "SP3D"
	int nPos,nNorm,nFace;
};

// extended format
class DataPointEx: public DataVec
{
	public:
	int flags;
	void SetPoint( const Vector3P &src )
	{
		(*this)[0]=src[0];
		(*this)[1]=src[1];
		(*this)[2]=src[2];
	}
};


class DataFaceEx: public DataFace
{
	public:
	int flags;
};


class DataHeaderEx
{
	public:
	char magic[4]; // "SP3X"
	int headSize;
	int version;
	int nPos,nNorm,nFace;
	int flags;
};


} // namespace Poseidon

#pragma pack(pop)
