/*
   Copyright 2012-2013
   Author: Daniel Turecek (daniel@turecek.info)
 */
#ifndef COMMON_H
#define COMMON_H

// basic data types
typedef int BOOL;
typedef unsigned char byte;
typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;

#ifndef FALSE
#define FALSE  0           
#endif
#ifndef TRUE
#define TRUE   1 
#endif

#ifdef WIN32
#include <windows.h>
typedef INT_PTR INTPTR; 
typedef u32 THREADID;
#define PATH_SEPAR          '\\'
#define PATH_SEPAR_STR      "\\"
#ifdef _MSC_VER
    #define snprintf(a,b,...) _snprintf_s(a,b,b,__VA_ARGS__)
    //#define vsnprintf(a,b,...) vsnprintf_s(a,b,b,__VA_ARGS__)
    #define strncpy(a,b,c) strncpy_s(a,c,b,((c)-1))
#endif
#else
#include <stdint.h>
typedef intptr_t INTPTR;
typedef long THREADID;
typedef void* HMODULE;
typedef void* HINSTANCE;
#define MAXDWORD            0xffffffff
#define PATH_SEPAR          '/'
#define PATH_SEPAR_STR      "/"
#endif

typedef enum _DataType
{
    DT_CHAR     = 0,  // signed char
    DT_BYTE     = 1,  // unsigned char
    DT_I16      = 2,  // signed short
    DT_U16      = 3,  // unsigned short
    DT_I32      = 4,  // int
    DT_U32      = 5,  // unsigned int
    DT_I64      = 6,  // long long
    DT_U64      = 7,  // unsigned long long
    DT_FLOAT    = 8,  // float
    DT_DOUBLE   = 9,  // double
    DT_BOOL     = 10, // BOOL (int)
    DT_STRING   = 11, // const chat *
    DT_LAST     = 12, // end of table
} DataType;

typedef unsigned DEVID;
typedef unsigned EVENTID;
typedef unsigned FRAMEID;

#define DATAFORMAT_FRAMES 1




#ifndef WIN32
    #define EXPORTFUNC extern "C" __attribute__ ((visibility("default")))
#else
    #define EXPORTFUNC extern "C" __declspec(dllexport)
#endif


#endif /* end of include guard: COMMON_H */


