

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

MIDL_DEFINE_GUID(IID, LIBID_FabricGateResourceManagerLib,0x943556f0,0xd940,0x4fd2,0xbd,0xa1,0x36,0x45,0x33,0xbd,0x54,0x75);


MIDL_DEFINE_GUID(IID, IID_IFabricGatewayResourceManager,0xbf9b93a9,0x5b74,0x4a28,0xb2,0x05,0xed,0xf3,0x87,0xad,0xf3,0xdb);


MIDL_DEFINE_GUID(IID, IID_IFabricGatewayResourceManagerAgent,0x029614d9,0x928a,0x489d,0x8e,0xaf,0xc0,0x9f,0x44,0x02,0x8f,0x46);


MIDL_DEFINE_GUID(CLSID, CLSID_FabricGatewayResourceManagerAgent,0x2407ced2,0x3530,0x4944,0x91,0x5e,0xd5,0x6e,0x93,0x92,0xcd,0x5b);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif




