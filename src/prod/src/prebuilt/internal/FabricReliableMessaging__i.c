

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

MIDL_DEFINE_GUID(IID, LIBID_FabricReliableMessagingTypeLib,0xd58ddaf2,0xbc17,0x4e87,0xa3,0x66,0x05,0xfc,0x64,0x0e,0x70,0x25);


MIDL_DEFINE_GUID(IID, IID_IFabricReliableSession,0xdf1e6d90,0x3fab,0x4c36,0xae,0x22,0x72,0x13,0x22,0x3f,0x46,0xae);


MIDL_DEFINE_GUID(IID, IID_IFabricReliableInboundSessionCallback,0x496b7bbc,0xb6d3,0x4990,0xad,0xf4,0xd6,0xe0,0x6e,0xed,0xfd,0x7d);


MIDL_DEFINE_GUID(IID, IID_IFabricReliableSessionAbortedCallback,0x978932f4,0xc93f,0x45ac,0xa8,0x04,0xd0,0xfb,0x4b,0x4f,0x71,0x93);


MIDL_DEFINE_GUID(IID, IID_IFabricReliableSessionManager,0x90f13016,0x7c1e,0x48f4,0xa7,0xdb,0x47,0x9c,0x22,0xf8,0x72,0x52);


MIDL_DEFINE_GUID(IID, IID_IFabricReliableSessionPartitionIdentity,0x6f6110e0,0x3eec,0x4d65,0xa7,0x61,0x21,0x8d,0x7f,0x47,0x95,0x15);


MIDL_DEFINE_GUID(IID, IID_IFabricMessageDataFactory,0xb2cc5f0e,0x2bdb,0x433d,0x9f,0x73,0xc7,0xe0,0x96,0x03,0x77,0x70);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



