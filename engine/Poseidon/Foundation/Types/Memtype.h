#ifndef _MEMTYPE_H
#define _MEMTYPE_H

#ifdef _WIN32
typedef unsigned long DWORD;
#else
typedef unsigned int DWORD;
#endif
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef long LONG;

typedef unsigned short word;
#ifdef _WIN32
typedef unsigned long dword;
#else
typedef unsigned int dword;
#endif
typedef unsigned char byte;

#ifndef FALSE
	typedef int BOOL;
	#define FALSE 0
	#define TRUE 1
#endif

#ifndef NULL
	#define NULL 0
#endif

#endif
