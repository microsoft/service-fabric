

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

MIDL_DEFINE_GUID(IID, LIBID_FabricImageStoreLib,0xaec959c5,0xf990,0x427a,0xbe,0x8f,0x89,0xa0,0x44,0x92,0x94,0x0b);


MIDL_DEFINE_GUID(IID, IID_IFabricNativeImageStoreClient,0xf6b3ceea,0x3577,0x49fa,0x8e,0x67,0x2d,0x0a,0xd1,0x02,0x4c,0x79);


MIDL_DEFINE_GUID(CLSID, CLSID_FabricNativeImageStoreClient,0x49f77ae3,0xac64,0x4f9e,0x93,0x6e,0x40,0x58,0xc5,0x70,0xa3,0x87);


MIDL_DEFINE_GUID(IID, IID_IFabricNativeImageStoreProgressEventHandler,0x62119833,0xad63,0x47d8,0xbb,0x8b,0x16,0x74,0x83,0xf7,0xb0,0x5a);


MIDL_DEFINE_GUID(IID, IID_IFabricNativeImageStoreClient2,0x80fa7785,0x4ad1,0x45b1,0x9e,0x21,0x0b,0x8c,0x03,0x07,0xc2,0x2f);


MIDL_DEFINE_GUID(IID, IID_IFabricNativeImageStoreClient3,0xd5e63df3,0xffb1,0x4837,0x99,0x59,0x2a,0x5b,0x1d,0xa9,0x4f,0x33);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



