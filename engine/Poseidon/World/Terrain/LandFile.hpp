#pragma once

#include <Poseidon/Foundation/Math/Math3DP.hpp>

namespace Poseidon
{
struct WorldHeader
{
	int magic;
	int xRange,zRange;
};

// version 2 

#ifdef _MSC_VER
  #define FILE_MAGIC 'RVW2'
#else
  #define FILE_MAGIC StrToInt("2WVR")
#endif
#define LEN_OBJNAME (64-16)
struct SingleObject
{
	float x,y,z;
	float heading;
	char name[LEN_OBJNAME];
};

// version 3

#ifdef _MSC_VER
  #define FILE_MAGIC_3 'RVW3'
#else
  #define FILE_MAGIC_3 StrToInt("3WVR")
#endif
#define LEN_OBJNAME_3 (96-16)
struct SingleObject3
{
	Matrix4P matrix;
	char name[LEN_OBJNAME_3];
};

#ifdef _MSC_VER
  #define FILE_MAGIC_4 'RVW4'
#else
  #define FILE_MAGIC_4 StrToInt("4WVR")
#endif
#define LEN_OBJNAME_4 (96-20)
struct SingleObject4
{
	Matrix4P matrix;
	int id;
	char name[LEN_OBJNAME_4];
};

}  // namespace Poseidon
