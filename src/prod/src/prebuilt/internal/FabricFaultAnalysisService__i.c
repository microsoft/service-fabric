

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

MIDL_DEFINE_GUID(IID, LIBID_FabricFaultAnalysisServiceLib,0x4d62281f,0xb4a1,0x4343,0x8c,0xb6,0x3b,0x66,0xef,0xea,0xba,0x63);


MIDL_DEFINE_GUID(IID, IID_IFabricFaultAnalysisService,0xbf9b93a9,0x5b74,0x4a28,0xb2,0x05,0xed,0xf3,0x87,0xad,0xf3,0xdb);


MIDL_DEFINE_GUID(IID, IID_IFabricFaultAnalysisServiceAgent,0xa202ba9d,0x1478,0x42a8,0xad,0x03,0x4a,0x1f,0x15,0xc7,0xd2,0x55);


MIDL_DEFINE_GUID(CLSID, CLSID_FabricFaultAnalysisServiceAgent,0xbbea0807,0x9fc1,0x4ef4,0x81,0xe2,0x99,0x51,0x65,0x11,0x56,0x4d);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



