

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 8.00.0613 */
/* @@MIDL_FILE_HEADING(  ) */



#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_FabricCommonLib,0xccd250b2,0x7c94,0x47ac,0xa4,0x97,0x5d,0x52,0x28,0xcd,0xdc,0xcd);


MIDL_DEFINE_GUID(IID, IID_IFabricAsyncOperationCallback,0x86f08d7e,0x14dd,0x4575,0x84,0x89,0xb1,0xd5,0xd6,0x79,0x02,0x9c);


MIDL_DEFINE_GUID(IID, IID_IFabricAsyncOperationContext,0x841720bf,0xc9e8,0x4e6f,0x9c,0x3f,0x6b,0x7f,0x4a,0xc7,0x3b,0xcd);


MIDL_DEFINE_GUID(IID, IID_IFabricStringResult,0x4ae69614,0x7d0f,0x4cd4,0xb8,0x36,0x23,0x01,0x70,0x00,0xd1,0x32);


MIDL_DEFINE_GUID(IID, IID_IFabricStringListResult,0xafab1c53,0x757b,0x4b0e,0x8b,0x7e,0x23,0x7a,0xee,0xe6,0xbf,0xe9);


MIDL_DEFINE_GUID(IID, IID_IFabricGetReplicatorStatusResult,0x30E10C61,0xA710,0x4F99,0xA6,0x23,0xBB,0x14,0x03,0x26,0x51,0x86);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



