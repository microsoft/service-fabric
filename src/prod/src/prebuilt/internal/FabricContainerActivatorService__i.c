

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

MIDL_DEFINE_GUID(IID, LIBID_FabricContainerActivatorServiceLib,0x6f9891a3,0xd279,0x4e19,0x9a,0x22,0xff,0x47,0xd0,0xad,0x58,0xa6);


MIDL_DEFINE_GUID(IID, IID_IFabricContainerActivatorService,0x111d2880,0x5b3e,0x48b3,0x89,0x5c,0xc6,0x20,0x36,0x83,0x0a,0x14);


MIDL_DEFINE_GUID(IID, IID_IFabricContainerActivatorService2,0x8fdba659,0x674c,0x4464,0xac,0x64,0x21,0xd4,0x10,0x31,0x3b,0x96);


MIDL_DEFINE_GUID(IID, IID_IFabricContainerActivatorServiceAgent,0xb40b7396,0x5d2a,0x471a,0xb6,0xbc,0x6c,0xfa,0xd1,0xcb,0x20,0x61);


MIDL_DEFINE_GUID(IID, IID_IFabricHostingSettingsResult,0x24e88e38,0xdeb8,0x4bd9,0x9b,0x55,0xca,0x67,0xd6,0x13,0x48,0x50);


MIDL_DEFINE_GUID(IID, IID_IFabricContainerApiExecutionResult,0xc40d5451,0x32c4,0x481a,0xb3,0x40,0xa9,0xe7,0x71,0xcc,0x69,0x37);


MIDL_DEFINE_GUID(CLSID, CLSID_FabricContainerActivatorServiceAgent,0x16c96a7a,0xa6d2,0x481c,0xb0,0x57,0x67,0x8a,0xd8,0xfa,0x10,0x5e);


MIDL_DEFINE_GUID(IID, IID_IFabricContainerActivatorServiceAgent2,0xac2bcdde,0x3987,0x4bf0,0xac,0x91,0x98,0x99,0x48,0xfa,0xac,0x85);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



