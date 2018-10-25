

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

MIDL_DEFINE_GUID(IID, LIBID_FabricTokenValidationServiceLib,0x43ba0ca0,0x0c91,0x4422,0x8e,0x1b,0xb6,0xb6,0x2d,0x6e,0x79,0x31);


MIDL_DEFINE_GUID(IID, IID_IFabricTokenValidationService,0xa70f8024,0x32f8,0x48ab,0x84,0xcf,0x21,0x19,0xa2,0x55,0x13,0xa9);


MIDL_DEFINE_GUID(IID, IID_IFabricTokenValidationServiceAgent,0x45898312,0x084e,0x4792,0xb2,0x34,0x52,0xef,0xb8,0xd7,0x9c,0xd8);


MIDL_DEFINE_GUID(IID, IID_IFabricTokenClaimResult,0x931ff709,0x4604,0x4e6a,0xbc,0x10,0xa3,0x80,0x56,0xcb,0xf3,0xb4);


MIDL_DEFINE_GUID(IID, IID_IFabricTokenServiceMetadataResult,0x316ea660,0x56ec,0x4459,0xa4,0xad,0x81,0x70,0x78,0x7c,0x5c,0x48);


MIDL_DEFINE_GUID(CLSID, CLSID_FabricTokenValidationServiceAgent,0x44cbab57,0x79c5,0x4961,0x82,0x06,0xd5,0x61,0x27,0x0d,0x1c,0xf1);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



