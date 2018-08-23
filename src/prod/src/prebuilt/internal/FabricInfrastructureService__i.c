

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

MIDL_DEFINE_GUID(IID, LIBID_FabricInfrastructureServiceLib,0xe51e79a7,0xcce3,0x45b4,0xb8,0xc1,0x58,0x57,0x34,0x4d,0xf2,0x24);


MIDL_DEFINE_GUID(IID, IID_IFabricInfrastructureService,0xd948b384,0xfd16,0x48f3,0xa4,0xad,0xd6,0xe6,0x8c,0x6b,0xf2,0xbb);


MIDL_DEFINE_GUID(IID, IID_IFabricInfrastructureServiceAgent,0x2416a4e2,0x9313,0x42ce,0x93,0xc9,0xb4,0x99,0x76,0x48,0x40,0xce);


MIDL_DEFINE_GUID(IID, IID_IFabricInfrastructureTaskQueryResult,0x209e28bb,0x4f8b,0x4f03,0x88,0xc9,0x7b,0x54,0xf2,0xcd,0x29,0xf9);


MIDL_DEFINE_GUID(CLSID, CLSID_FabricInfrastructureServiceAgent,0x8400d67e,0x7f19,0x45f3,0xbe,0xc2,0xb1,0x37,0xda,0x42,0x10,0x6a);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



