#ifndef __BASIC_TYPES_H__
#define __BASIC_TYPES_H__
#include "autoconf.h"
#ifndef _func_enter_
#define _func_enter_	do{}while(0)
#endif
#ifndef _func_exit_
#define _func_exit_	do{}while(0)
#endif
#define SUCCESS	0
#define FAIL	-1

#ifndef TRUE
	#define _TRUE	1
#else
	#define _TRUE	TRUE	
#endif
		
#ifndef FALSE		
	#define _FALSE	0
#else
	#define _FALSE	FALSE	
#endif

#ifdef PLATFORM_WINDOWS

	typedef signed char s8;
	typedef unsigned char u8;

	typedef signed short s16;
	typedef unsigned short u16;

	typedef signed long s32;
	typedef unsigned long u32;
	
	typedef unsigned int	uint;
	typedef	signed int		sint;


	typedef signed long long s64;
	typedef unsigned long long u64;

	#ifdef NDIS50_MINIPORT
	
		#define NDIS_MAJOR_VERSION       5
		#define NDIS_MINOR_VERSION       0

	#endif

	#ifdef NDIS51_MINIPORT

		#define NDIS_MAJOR_VERSION       5
		#define NDIS_MINOR_VERSION       1

	#endif

	typedef NDIS_PROC proc_t;

	typedef LONG atomic_t;

#endif
#ifdef PLATFORM_LINUX

	#include <linux/types.h>
	#define IN
	#define OUT
	#define VOID void
	#define NDIS_OID uint
	#define NDIS_STATUS uint

	typedef	signed int sint;

	#ifndef	PVOID
	typedef void * PVOID;
	//#define PVOID	(void *)
	#endif

        #define UCHAR u8
	#define USHORT u16
	#define UINT u32
	#define ULONG u32	

	typedef void (*proc_t)(void*);

	typedef 	__kernel_size_t	SIZE_T;	
	typedef	__kernel_ssize_t	SSIZE_T;
	#define FIELD_OFFSET(s,field)	((SSIZE_T)&((s*)(0))->field)
	
#endif

#ifdef PLATFORM_FREEBSD

	typedef signed char s8;
	typedef unsigned char u8;

	typedef signed short s16;
	typedef unsigned short u16;

	typedef signed int s32;
	typedef unsigned int u32;
	
	typedef unsigned int	uint;
	typedef	signed int		sint;
	typedef long atomic_t;

	typedef signed long long s64;
	typedef unsigned long long u64;
	#define IN
	#define OUT
	#define VOID void
	#define NDIS_OID uint
	#define NDIS_STATUS uint
	
	#ifndef	PVOID
	typedef void * PVOID;
	//#define PVOID	(void *)
	#endif
	typedef u32 dma_addr_t;
    #define UCHAR u8
	#define USHORT u16
	#define UINT u32
	#define ULONG u32	

	typedef void (*proc_t)(void*);
  
  typedef unsigned int __kernel_size_t;
  typedef int __kernel_ssize_t;
  
	typedef 	__kernel_size_t	SIZE_T;	
	typedef	__kernel_ssize_t	SSIZE_T;
	#define FIELD_OFFSET(s,field)	((SSIZE_T)&((s*)(0))->field)
	
#endif


#define _RND(sz, r) ((((sz)+((r)-1))/(r))*(r))
#define RND4(x)	(((x >> 2) + (((x & 3) == 0) ?  0: 1)) << 2)
__inline static u32 _RND4(u32 sz)
{

	u32	val;

	val = ((sz >> 2) + ((sz & 3) ? 1: 0)) << 2;
	
	return val;

}


#define SIZE_PTR SIZE_T
#define SSIZE_PTR SSIZE_T

// Get the N-bytes aligment offset from the current length
#define N_BYTE_ALIGMENT(__Value, __Aligment) ((__Aligment == 1) ? (__Value) : (((__Value + __Aligment - 1) / __Aligment) * __Aligment))

#endif //__BASIC_TYPES_H__


