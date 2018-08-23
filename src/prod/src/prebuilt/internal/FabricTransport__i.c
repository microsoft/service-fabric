

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

MIDL_DEFINE_GUID(IID, LIBID_FabricTransport_Lib,0x3711066b,0x3b42,0x420c,0xa4,0x2f,0x99,0x5e,0xdb,0x7e,0x3e,0x11);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportConnectionHandler,0xb069692d,0xe8f0,0x4f25,0xa3,0xb6,0xb2,0x99,0x25,0x98,0xa6,0x4c);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportMessage,0xb4357dab,0xef06,0x465f,0xb4,0x53,0x93,0x8f,0x3b,0x0a,0xd4,0xb5);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportMessageHandler,0x6815bdb4,0x1479,0x4c44,0x8b,0x9d,0x57,0xd6,0xd0,0xcc,0x9d,0x64);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportCallbackMessageHandler,0x9ba8ac7a,0x3464,0x4774,0xb9,0xb9,0x1d,0x7f,0x0f,0x19,0x20,0xba);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportListener,0x1b63a266,0x1eeb,0x4f3e,0x88,0x86,0x52,0x14,0x58,0x98,0x0d,0x10);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportClient,0x5b0634fe,0x6a52,0x4bd9,0x80,0x59,0x89,0x2c,0x72,0xc1,0xd7,0x3a);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportClientEventHandler,0x4935ab6f,0xa8bc,0x4b10,0xa6,0x9e,0x7a,0x3b,0xa3,0x32,0x48,0x92);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportClientConnection,0xa54c17f7,0xfe94,0x4838,0xb1,0x4d,0xe9,0xb5,0xc2,0x58,0xe2,0xd0);


MIDL_DEFINE_GUID(IID, IID_IFabricTransportMessageDisposer,0x914097f3,0xa821,0x46ea,0xb3,0xd9,0xfe,0xaf,0xe5,0xf7,0xc4,0xa9);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



