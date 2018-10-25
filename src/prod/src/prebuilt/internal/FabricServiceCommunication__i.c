

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

MIDL_DEFINE_GUID(IID, LIBID_FabricServiceCommunication_Lib,0x3711066b,0x3b42,0x420c,0xa4,0x2f,0x99,0x5e,0xdb,0x7e,0x3e,0x11);


MIDL_DEFINE_GUID(IID, IID_IFabricCommunicationMessageSender,0xfdf2bcd7,0x14f9,0x463f,0x9b,0x70,0xae,0x3b,0x5f,0xf9,0xd8,0x3f);


MIDL_DEFINE_GUID(IID, IID_IFabricServiceConnectionHandler,0xb069692d,0xe8f0,0x4f25,0xa3,0xb6,0xb2,0x99,0x25,0x98,0xa6,0x4c);


MIDL_DEFINE_GUID(IID, IID_IFabricServiceCommunicationClient,0x255ecbe8,0x96b8,0x4f47,0x9e,0x2c,0x12,0x35,0xdb,0xa3,0x22,0x0a);


MIDL_DEFINE_GUID(IID, IID_IFabricServiceCommunicationClient2,0x73b2cac5,0x4278,0x475b,0x82,0xe6,0x1e,0x33,0xeb,0xe2,0x07,0x67);


MIDL_DEFINE_GUID(IID, IID_IFabricServiceCommunicationListener,0xad5d9f82,0xd62c,0x4819,0x99,0x38,0x66,0x85,0x40,0x24,0x8e,0x97);


MIDL_DEFINE_GUID(IID, IID_IFabricServiceCommunicationMessage,0xdc6e168a,0xdbd4,0x4ce1,0xa3,0xdc,0x5f,0x33,0x49,0x4f,0x49,0x72);


MIDL_DEFINE_GUID(IID, IID_IFabricClientConnection,0x60ae1ab3,0x5f00,0x404d,0x8f,0x89,0x96,0x48,0x5c,0x8b,0x01,0x3e);


MIDL_DEFINE_GUID(IID, IID_IFabricServiceConnectionEventHandler,0x77f434b1,0xf9e9,0x4cb1,0xb0,0xc4,0xc7,0xea,0x29,0x84,0xaa,0x8d);


MIDL_DEFINE_GUID(IID, IID_IFabricCommunicationMessageHandler,0x7e010010,0x80b2,0x453c,0xaa,0xb3,0xa7,0x3f,0x07,0x90,0xdf,0xac);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



