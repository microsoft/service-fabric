

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

MIDL_DEFINE_GUID(IID, LIBID_KPhysicalLog_Lib,0x5d817d09,0x53d6,0x4a6a,0x84,0x6a,0x60,0x76,0x10,0x5c,0xf6,0x00);


MIDL_DEFINE_GUID(IID, IID_IKPhysicalLogManager,0xc63c9378,0x6018,0x4859,0xa9,0x1e,0x85,0xc2,0x68,0x43,0x5d,0x99);


MIDL_DEFINE_GUID(IID, IID_IKPhysicalLogContainer,0x1bf056c8,0x57ba,0x4fa1,0xb9,0xc7,0xbd,0x9c,0x47,0x83,0xbf,0x6a);


MIDL_DEFINE_GUID(IID, IID_IKPhysicalLogStream,0x094fda74,0xd14b,0x4316,0xab,0xad,0xac,0xa1,0x33,0xdf,0xcb,0x22);


MIDL_DEFINE_GUID(IID, IID_IKBuffer,0x89ab6225,0x2cb4,0x48b4,0xbe,0x5c,0x04,0xbd,0x44,0x1c,0x77,0x4f);


MIDL_DEFINE_GUID(IID, IID_IKIoBuffer,0x29a7c7a2,0xa96e,0x419b,0xb1,0x71,0x62,0x63,0x62,0x5b,0x62,0x19);


MIDL_DEFINE_GUID(IID, IID_IKIoBufferElement,0x61f6b9bf,0x1d08,0x4d04,0xb9,0x7f,0xe6,0x45,0x78,0x2e,0x79,0x81);


MIDL_DEFINE_GUID(IID, IID_IKArray,0xb5a6bf9e,0x8247,0x40ce,0xaa,0x31,0xac,0xf8,0xb0,0x99,0xbd,0x1c);


MIDL_DEFINE_GUID(CLSID, CLSID_KPhysicalLog,0xeb324718,0x9ae0,0x4b66,0xa3,0x51,0x88,0x19,0xc4,0x90,0x7a,0x79);


MIDL_DEFINE_GUID(IID, IID_IKPhysicalLogStream2,0xE40B452B,0x03F9,0x4b6e,0x82,0x29,0x53,0xC5,0x09,0xB8,0x7F,0x2F);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



