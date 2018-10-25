// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------



/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0613 */
/* @@MIDL_FILE_HEADING(  ) */



/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __kphysicallog__h__
#define __kphysicallog__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IKPhysicalLogManager_FWD_DEFINED__
#define __IKPhysicalLogManager_FWD_DEFINED__
typedef interface IKPhysicalLogManager IKPhysicalLogManager;

#endif 	/* __IKPhysicalLogManager_FWD_DEFINED__ */


#ifndef __IKPhysicalLogContainer_FWD_DEFINED__
#define __IKPhysicalLogContainer_FWD_DEFINED__
typedef interface IKPhysicalLogContainer IKPhysicalLogContainer;

#endif 	/* __IKPhysicalLogContainer_FWD_DEFINED__ */


#ifndef __IKPhysicalLogStream_FWD_DEFINED__
#define __IKPhysicalLogStream_FWD_DEFINED__
typedef interface IKPhysicalLogStream IKPhysicalLogStream;

#endif 	/* __IKPhysicalLogStream_FWD_DEFINED__ */


#ifndef __IKBuffer_FWD_DEFINED__
#define __IKBuffer_FWD_DEFINED__
typedef interface IKBuffer IKBuffer;

#endif 	/* __IKBuffer_FWD_DEFINED__ */


#ifndef __IKIoBuffer_FWD_DEFINED__
#define __IKIoBuffer_FWD_DEFINED__
typedef interface IKIoBuffer IKIoBuffer;

#endif 	/* __IKIoBuffer_FWD_DEFINED__ */


#ifndef __IKIoBufferElement_FWD_DEFINED__
#define __IKIoBufferElement_FWD_DEFINED__
typedef interface IKIoBufferElement IKIoBufferElement;

#endif 	/* __IKIoBufferElement_FWD_DEFINED__ */


#ifndef __IKArray_FWD_DEFINED__
#define __IKArray_FWD_DEFINED__
typedef interface IKArray IKArray;

#endif 	/* __IKArray_FWD_DEFINED__ */


#ifndef __KPhysicalLog_FWD_DEFINED__
#define __KPhysicalLog_FWD_DEFINED__

#ifdef __cplusplus
typedef class KPhysicalLog KPhysicalLog;
#else
typedef struct KPhysicalLog KPhysicalLog;
#endif /* __cplusplus */

#endif 	/* __KPhysicalLog_FWD_DEFINED__ */


#ifndef __IKIoBuffer_FWD_DEFINED__
#define __IKIoBuffer_FWD_DEFINED__
typedef interface IKIoBuffer IKIoBuffer;

#endif 	/* __IKIoBuffer_FWD_DEFINED__ */


#ifndef __IKIoBufferElement_FWD_DEFINED__
#define __IKIoBufferElement_FWD_DEFINED__
typedef interface IKIoBufferElement IKIoBufferElement;

#endif 	/* __IKIoBufferElement_FWD_DEFINED__ */


#ifndef __IKBuffer_FWD_DEFINED__
#define __IKBuffer_FWD_DEFINED__
typedef interface IKBuffer IKBuffer;

#endif 	/* __IKBuffer_FWD_DEFINED__ */


#ifndef __IKArray_FWD_DEFINED__
#define __IKArray_FWD_DEFINED__
typedef interface IKArray IKArray;

#endif 	/* __IKArray_FWD_DEFINED__ */


#ifndef __IKPhysicalLogManager_FWD_DEFINED__
#define __IKPhysicalLogManager_FWD_DEFINED__
typedef interface IKPhysicalLogManager IKPhysicalLogManager;

#endif 	/* __IKPhysicalLogManager_FWD_DEFINED__ */


#ifndef __IKPhysicalLogContainer_FWD_DEFINED__
#define __IKPhysicalLogContainer_FWD_DEFINED__
typedef interface IKPhysicalLogContainer IKPhysicalLogContainer;

#endif 	/* __IKPhysicalLogContainer_FWD_DEFINED__ */


#ifndef __IKPhysicalLogStream_FWD_DEFINED__
#define __IKPhysicalLogStream_FWD_DEFINED__
typedef interface IKPhysicalLogStream IKPhysicalLogStream;

#endif 	/* __IKPhysicalLogStream_FWD_DEFINED__ */


#ifndef __IKPhysicalLogStream2_FWD_DEFINED__
#define __IKPhysicalLogStream2_FWD_DEFINED__
typedef interface IKPhysicalLogStream2 IKPhysicalLogStream2;

#endif 	/* __IKPhysicalLogStream2_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "FabricTypes.h"
#include "FabricCommon.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_kphysicallog__0000_0000 */
/* [local] */ 

#if ( _MSC_VER >= 1020 )
#pragma once
#endif








extern RPC_IF_HANDLE __MIDL_itf_kphysicallog__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_kphysicallog__0000_0000_v0_0_s_ifspec;


#ifndef __KPhysicalLog_Lib_LIBRARY_DEFINED__
#define __KPhysicalLog_Lib_LIBRARY_DEFINED__

/* library KPhysicalLog_Lib */
/* [version][helpstring][uuid] */ 


#pragma pack(push, 8)
typedef GUID KTL_LOG_STREAM_ID;

typedef GUID KTL_LOG_STREAM_TYPE_ID;

typedef GUID KTL_LOG_ID;

typedef GUID KTL_DISK_ID;

typedef ULONGLONG KTL_LOG_ASN;

typedef /* [uuid][v1_enum] */  DECLSPEC_UUID("8aafa011-ad17-413b-9769-cb3d121bf524") 
enum KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION
    {
        DispositionPending	= 0,
        DispositionPersisted	= 1,
        DispositionNone	= 2
    } 	KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION;

typedef /* [uuid][v1_enum] */  DECLSPEC_UUID("10c7e359-bd5b-4e9e-bab5-0442a7297af1") 
enum KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE
    {
        LogSpaceUtilization	= 0
    } 	KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE;

typedef struct KIOBUFFER_ELEMENT_DESCRIPTOR
    {
    ULONG Size;
    ULONGLONG ElementBaseAddress;
    } 	KIOBUFFER_ELEMENT_DESCRIPTOR;

typedef struct KPHYSICAL_LOG_STREAM_RecordMetadata
    {
    KTL_LOG_ASN Asn;
    ULONGLONG Version;
    KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION Disposition;
    ULONG Size;
    LONGLONG Debug_LsnValue;
    } 	KPHYSICAL_LOG_STREAM_RecordMetadata;









#pragma pack(pop)

EXTERN_C const IID LIBID_KPhysicalLog_Lib;

#ifndef __IKPhysicalLogManager_INTERFACE_DEFINED__
#define __IKPhysicalLogManager_INTERFACE_DEFINED__

/* interface IKPhysicalLogManager */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKPhysicalLogManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c63c9378-6018-4859-a91e-85c268435d99")
    IKPhysicalLogManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCreateLogContainer( 
            /* [in] */ LPCWSTR fullyQualifiedLogFilePath,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ LPCWSTR logType,
            /* [in] */ LONGLONG logSize,
            /* [in] */ ULONG maxAllowedStreams,
            /* [in] */ ULONG maxRecordSize,
            /* [in] */ DWORD creationFlags,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCreateLogContainer( 
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ IKPhysicalLogContainer **const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginOpenLogContainerById( 
            /* [in] */ KTL_DISK_ID diskId,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginOpenLogContainer( 
            /* [in] */ LPCWSTR fullyQualifiedLogFilePath,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpenLogContainer( 
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ IKPhysicalLogContainer **const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeleteLogContainer( 
            /* [in] */ LPCWSTR fullyQualifiedLogFilePath,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeleteLogContainerById( 
            /* [in] */ KTL_DISK_ID diskId,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeleteLogContainer( 
            /* [in] */ IFabricAsyncOperationContext *const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnumerateLogContainers( 
            /* [in] */ KTL_LOG_ID diskId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnumerateLogContainers( 
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ IKArray **const result) = 0;
        
#if !defined(PLATFORM_UNIX)
        virtual HRESULT STDMETHODCALLTYPE BeginGetVolumeIdFromPath( 
            /* [in] */ LPCWSTR Path,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetVolumeIdFromPath( 
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ GUID *const result) = 0;
#endif
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKPhysicalLogManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKPhysicalLogManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKPhysicalLogManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKPhysicalLogManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationContext *const context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationContext *const context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCreateLogContainer )( 
            IKPhysicalLogManager * This,
            /* [in] */ LPCWSTR fullyQualifiedLogFilePath,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ LPCWSTR logType,
            /* [in] */ LONGLONG logSize,
            /* [in] */ ULONG maxAllowedStreams,
            /* [in] */ ULONG maxRecordSize,
            /* [in] */ DWORD creationFlags,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCreateLogContainer )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ IKPhysicalLogContainer **const result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpenLogContainerById )( 
            IKPhysicalLogManager * This,
            /* [in] */ KTL_DISK_ID diskId,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpenLogContainer )( 
            IKPhysicalLogManager * This,
            /* [in] */ LPCWSTR fullyQualifiedLogFilePath,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpenLogContainer )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ IKPhysicalLogContainer **const result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteLogContainer )( 
            IKPhysicalLogManager * This,
            /* [in] */ LPCWSTR fullyQualifiedLogFilePath,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteLogContainerById )( 
            IKPhysicalLogManager * This,
            /* [in] */ KTL_DISK_ID diskId,
            /* [in] */ KTL_LOG_ID logId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteLogContainer )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationContext *const context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginEnumerateLogContainers )( 
            IKPhysicalLogManager * This,
            /* [in] */ KTL_LOG_ID diskId,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *EndEnumerateLogContainers )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ IKArray **const result);
        
#if !defined(PLATFORM_UNIX)
        HRESULT ( STDMETHODCALLTYPE *BeginGetVolumeIdFromPath )( 
            IKPhysicalLogManager * This,
            /* [in] */ LPCWSTR Path,
            /* [in] */ IFabricAsyncOperationCallback *const callback,
            /* [out] */ IFabricAsyncOperationContext **const context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetVolumeIdFromPath )( 
            IKPhysicalLogManager * This,
            /* [in] */ IFabricAsyncOperationContext *const context,
            /* [out] */ GUID *const result);
#endif
        
        END_INTERFACE
    } IKPhysicalLogManagerVtbl;

    interface IKPhysicalLogManager
    {
        CONST_VTBL struct IKPhysicalLogManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKPhysicalLogManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKPhysicalLogManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKPhysicalLogManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKPhysicalLogManager_BeginOpen(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,callback,context) ) 

#define IKPhysicalLogManager_EndOpen(This,context)	\
    ( (This)->lpVtbl -> EndOpen(This,context) ) 

#define IKPhysicalLogManager_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IKPhysicalLogManager_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IKPhysicalLogManager_BeginCreateLogContainer(This,fullyQualifiedLogFilePath,logId,logType,logSize,maxAllowedStreams,maxRecordSize,creationFlags,callback,context)	\
    ( (This)->lpVtbl -> BeginCreateLogContainer(This,fullyQualifiedLogFilePath,logId,logType,logSize,maxAllowedStreams,maxRecordSize,creationFlags,callback,context) ) 

#define IKPhysicalLogManager_EndCreateLogContainer(This,context,result)	\
    ( (This)->lpVtbl -> EndCreateLogContainer(This,context,result) ) 

#define IKPhysicalLogManager_BeginOpenLogContainerById(This,diskId,logId,callback,context)	\
    ( (This)->lpVtbl -> BeginOpenLogContainerById(This,diskId,logId,callback,context) ) 

#define IKPhysicalLogManager_BeginOpenLogContainer(This,fullyQualifiedLogFilePath,logId,callback,context)	\
    ( (This)->lpVtbl -> BeginOpenLogContainer(This,fullyQualifiedLogFilePath,logId,callback,context) ) 

#define IKPhysicalLogManager_EndOpenLogContainer(This,context,result)	\
    ( (This)->lpVtbl -> EndOpenLogContainer(This,context,result) ) 

#define IKPhysicalLogManager_BeginDeleteLogContainer(This,fullyQualifiedLogFilePath,logId,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteLogContainer(This,fullyQualifiedLogFilePath,logId,callback,context) ) 

#define IKPhysicalLogManager_BeginDeleteLogContainerById(This,diskId,logId,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteLogContainerById(This,diskId,logId,callback,context) ) 

#define IKPhysicalLogManager_EndDeleteLogContainer(This,context)	\
    ( (This)->lpVtbl -> EndDeleteLogContainer(This,context) ) 

#define IKPhysicalLogManager_BeginEnumerateLogContainers(This,diskId,callback,context)	\
    ( (This)->lpVtbl -> BeginEnumerateLogContainers(This,diskId,callback,context) ) 

#define IKPhysicalLogManager_EndEnumerateLogContainers(This,context,result)	\
    ( (This)->lpVtbl -> EndEnumerateLogContainers(This,context,result) ) 

#if !defined(PLATFORM_UNIX)
#define IKPhysicalLogManager_BeginGetVolumeIdFromPath(This,Path,callback,context)	\
    ( (This)->lpVtbl -> BeginGetVolumeIdFromPath(This,Path,callback,context) ) 

#define IKPhysicalLogManager_EndGetVolumeIdFromPath(This,context,result)	\
    ( (This)->lpVtbl -> EndGetVolumeIdFromPath(This,context,result) ) 
#endif

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKPhysicalLogManager_INTERFACE_DEFINED__ */


#ifndef __IKPhysicalLogContainer_INTERFACE_DEFINED__
#define __IKPhysicalLogContainer_INTERFACE_DEFINED__

/* interface IKPhysicalLogContainer */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKPhysicalLogContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1bf056c8-57ba-4fa1-b9c7-bd9c4783bf6a")
    IKPhysicalLogContainer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsFunctional( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCreateLogStream( 
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ KTL_LOG_STREAM_TYPE_ID logStreamType,
            /* [in] */ LPCWSTR optionalLogStreamAlias,
            /* [in] */ LPCWSTR path,
            /* [in] */ IKBuffer *const optionalSecurityInfo,
            /* [in] */ LONGLONG maximumStreamSize,
            /* [in] */ ULONG maximumBlockSize,
            /* [in] */ DWORD creationFlags,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCreateLogStream( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IKPhysicalLogStream **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeleteLogStream( 
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeleteLogStream( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginOpenLogStream( 
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpenLogStream( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IKPhysicalLogStream **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAssignAlias( 
            /* [in] */ LPCWSTR alias,
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAssignAlias( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRemoveAlias( 
            /* [in] */ LPCWSTR alias,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRemoveAlias( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginResolveAlias( 
            /* [in] */ LPCWSTR alias,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndResolveAlias( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_STREAM_ID *const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKPhysicalLogContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKPhysicalLogContainer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKPhysicalLogContainer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKPhysicalLogContainer * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsFunctional )( 
            IKPhysicalLogContainer * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCreateLogStream )( 
            IKPhysicalLogContainer * This,
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ KTL_LOG_STREAM_TYPE_ID logStreamType,
            /* [in] */ LPCWSTR optionalLogStreamAlias,
            /* [in] */ LPCWSTR path,
            /* [in] */ IKBuffer *const optionalSecurityInfo,
            /* [in] */ LONGLONG maximumStreamSize,
            /* [in] */ ULONG maximumBlockSize,
            /* [in] */ DWORD creationFlags,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCreateLogStream )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IKPhysicalLogStream **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteLogStream )( 
            IKPhysicalLogContainer * This,
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteLogStream )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpenLogStream )( 
            IKPhysicalLogContainer * This,
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpenLogStream )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IKPhysicalLogStream **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAssignAlias )( 
            IKPhysicalLogContainer * This,
            /* [in] */ LPCWSTR alias,
            /* [in] */ KTL_LOG_STREAM_ID logStreamId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndAssignAlias )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRemoveAlias )( 
            IKPhysicalLogContainer * This,
            /* [in] */ LPCWSTR alias,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRemoveAlias )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginResolveAlias )( 
            IKPhysicalLogContainer * This,
            /* [in] */ LPCWSTR alias,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndResolveAlias )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_STREAM_ID *const result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IKPhysicalLogContainer * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IKPhysicalLogContainerVtbl;

    interface IKPhysicalLogContainer
    {
        CONST_VTBL struct IKPhysicalLogContainerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKPhysicalLogContainer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKPhysicalLogContainer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKPhysicalLogContainer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKPhysicalLogContainer_IsFunctional(This)	\
    ( (This)->lpVtbl -> IsFunctional(This) ) 

#define IKPhysicalLogContainer_BeginCreateLogStream(This,logStreamId,logStreamType,optionalLogStreamAlias,path,optionalSecurityInfo,maximumStreamSize,maximumBlockSize,creationFlags,callback,context)	\
    ( (This)->lpVtbl -> BeginCreateLogStream(This,logStreamId,logStreamType,optionalLogStreamAlias,path,optionalSecurityInfo,maximumStreamSize,maximumBlockSize,creationFlags,callback,context) ) 

#define IKPhysicalLogContainer_EndCreateLogStream(This,context,result)	\
    ( (This)->lpVtbl -> EndCreateLogStream(This,context,result) ) 

#define IKPhysicalLogContainer_BeginDeleteLogStream(This,logStreamId,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteLogStream(This,logStreamId,callback,context) ) 

#define IKPhysicalLogContainer_EndDeleteLogStream(This,context)	\
    ( (This)->lpVtbl -> EndDeleteLogStream(This,context) ) 

#define IKPhysicalLogContainer_BeginOpenLogStream(This,logStreamId,callback,context)	\
    ( (This)->lpVtbl -> BeginOpenLogStream(This,logStreamId,callback,context) ) 

#define IKPhysicalLogContainer_EndOpenLogStream(This,context,result)	\
    ( (This)->lpVtbl -> EndOpenLogStream(This,context,result) ) 

#define IKPhysicalLogContainer_BeginAssignAlias(This,alias,logStreamId,callback,context)	\
    ( (This)->lpVtbl -> BeginAssignAlias(This,alias,logStreamId,callback,context) ) 

#define IKPhysicalLogContainer_EndAssignAlias(This,context)	\
    ( (This)->lpVtbl -> EndAssignAlias(This,context) ) 

#define IKPhysicalLogContainer_BeginRemoveAlias(This,alias,callback,context)	\
    ( (This)->lpVtbl -> BeginRemoveAlias(This,alias,callback,context) ) 

#define IKPhysicalLogContainer_EndRemoveAlias(This,context)	\
    ( (This)->lpVtbl -> EndRemoveAlias(This,context) ) 

#define IKPhysicalLogContainer_BeginResolveAlias(This,alias,callback,context)	\
    ( (This)->lpVtbl -> BeginResolveAlias(This,alias,callback,context) ) 

#define IKPhysicalLogContainer_EndResolveAlias(This,context,result)	\
    ( (This)->lpVtbl -> EndResolveAlias(This,context,result) ) 

#define IKPhysicalLogContainer_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IKPhysicalLogContainer_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKPhysicalLogContainer_INTERFACE_DEFINED__ */


#ifndef __IKPhysicalLogStream_INTERFACE_DEFINED__
#define __IKPhysicalLogStream_INTERFACE_DEFINED__

/* interface IKPhysicalLogStream */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKPhysicalLogStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("094fda74-d14b-4316-abad-aca133dfcb22")
    IKPhysicalLogStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsFunctional( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryLogStreamId( 
            /* [out] */ KTL_LOG_STREAM_ID *const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryReservedMetadataSize( 
            /* [out] */ ULONG *const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginQueryRecordRange( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndQueryRecordRange( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_ASN *const optionalLowestAsn,
            /* [out] */ KTL_LOG_ASN *const optionalHighestAsn,
            /* [out] */ KTL_LOG_ASN *const optionalLogTruncationAsn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginWrite( 
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ ULONGLONG version,
            /* [in] */ ULONG metadataSize,
            /* [in] */ IKIoBuffer *const metadataBuffer,
            /* [in] */ IKIoBuffer *const ioBuffer,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndWrite( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRead( 
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRead( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONGLONG *const optionalVersion,
            /* [out] */ ULONG *const metadataSize,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReadContaining( 
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReadContaining( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_ASN *const containingAsn,
            /* [out] */ ULONGLONG *const containingVersion,
            /* [out] */ ULONG *const metadataSize,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginQueryRecord( 
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndQueryRecord( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONGLONG *const optionalVersion,
            /* [out] */ KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION *const optionalDisposition,
            /* [out] */ ULONG *const optionalIoBufferSize,
            /* [out] */ ULONGLONG *const optionalDebugInfo1) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginQueryRecords( 
            /* [in] */ KTL_LOG_ASN lowestAsn,
            /* [in] */ KTL_LOG_ASN highestAsn,
            /* [in] */ IKArray *const resultArray,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndQueryRecords( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Truncate( 
            /* [in] */ KTL_LOG_ASN truncationPoint,
            /* [in] */ KTL_LOG_ASN preferredTruncationPoint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginWaitForNotification( 
            /* [in] */ KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE notificationType,
            /* [in] */ ULONGLONG notificationValue,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndWaitForNotification( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginIoctl( 
            /* [in] */ ULONG controlCode,
            /* [in] */ IKBuffer *const inBuffer,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndIoctl( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONG *const result,
            /* [out] */ IKBuffer **const outBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKPhysicalLogStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKPhysicalLogStream * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKPhysicalLogStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKPhysicalLogStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsFunctional )( 
            IKPhysicalLogStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryLogStreamId )( 
            IKPhysicalLogStream * This,
            /* [out] */ KTL_LOG_STREAM_ID *const result);
        
        HRESULT ( STDMETHODCALLTYPE *QueryReservedMetadataSize )( 
            IKPhysicalLogStream * This,
            /* [out] */ ULONG *const result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginQueryRecordRange )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndQueryRecordRange )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_ASN *const optionalLowestAsn,
            /* [out] */ KTL_LOG_ASN *const optionalHighestAsn,
            /* [out] */ KTL_LOG_ASN *const optionalLogTruncationAsn);
        
        HRESULT ( STDMETHODCALLTYPE *BeginWrite )( 
            IKPhysicalLogStream * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ ULONGLONG version,
            /* [in] */ ULONG metadataSize,
            /* [in] */ IKIoBuffer *const metadataBuffer,
            /* [in] */ IKIoBuffer *const ioBuffer,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndWrite )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRead )( 
            IKPhysicalLogStream * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRead )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONGLONG *const optionalVersion,
            /* [out] */ ULONG *const metadataSize,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReadContaining )( 
            IKPhysicalLogStream * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReadContaining )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_ASN *const containingAsn,
            /* [out] */ ULONGLONG *const containingVersion,
            /* [out] */ ULONG *const metadataSize,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *BeginQueryRecord )( 
            IKPhysicalLogStream * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndQueryRecord )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONGLONG *const optionalVersion,
            /* [out] */ KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION *const optionalDisposition,
            /* [out] */ ULONG *const optionalIoBufferSize,
            /* [out] */ ULONGLONG *const optionalDebugInfo1);
        
        HRESULT ( STDMETHODCALLTYPE *BeginQueryRecords )( 
            IKPhysicalLogStream * This,
            /* [in] */ KTL_LOG_ASN lowestAsn,
            /* [in] */ KTL_LOG_ASN highestAsn,
            /* [in] */ IKArray *const resultArray,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndQueryRecords )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *Truncate )( 
            IKPhysicalLogStream * This,
            /* [in] */ KTL_LOG_ASN truncationPoint,
            /* [in] */ KTL_LOG_ASN preferredTruncationPoint);
        
        HRESULT ( STDMETHODCALLTYPE *BeginWaitForNotification )( 
            IKPhysicalLogStream * This,
            /* [in] */ KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE notificationType,
            /* [in] */ ULONGLONG notificationValue,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndWaitForNotification )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginIoctl )( 
            IKPhysicalLogStream * This,
            /* [in] */ ULONG controlCode,
            /* [in] */ IKBuffer *const inBuffer,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndIoctl )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONG *const result,
            /* [out] */ IKBuffer **const outBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IKPhysicalLogStream * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IKPhysicalLogStreamVtbl;

    interface IKPhysicalLogStream
    {
        CONST_VTBL struct IKPhysicalLogStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKPhysicalLogStream_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKPhysicalLogStream_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKPhysicalLogStream_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKPhysicalLogStream_IsFunctional(This)	\
    ( (This)->lpVtbl -> IsFunctional(This) ) 

#define IKPhysicalLogStream_QueryLogStreamId(This,result)	\
    ( (This)->lpVtbl -> QueryLogStreamId(This,result) ) 

#define IKPhysicalLogStream_QueryReservedMetadataSize(This,result)	\
    ( (This)->lpVtbl -> QueryReservedMetadataSize(This,result) ) 

#define IKPhysicalLogStream_BeginQueryRecordRange(This,callback,context)	\
    ( (This)->lpVtbl -> BeginQueryRecordRange(This,callback,context) ) 

#define IKPhysicalLogStream_EndQueryRecordRange(This,context,optionalLowestAsn,optionalHighestAsn,optionalLogTruncationAsn)	\
    ( (This)->lpVtbl -> EndQueryRecordRange(This,context,optionalLowestAsn,optionalHighestAsn,optionalLogTruncationAsn) ) 

#define IKPhysicalLogStream_BeginWrite(This,asn,version,metadataSize,metadataBuffer,ioBuffer,callback,context)	\
    ( (This)->lpVtbl -> BeginWrite(This,asn,version,metadataSize,metadataBuffer,ioBuffer,callback,context) ) 

#define IKPhysicalLogStream_EndWrite(This,context)	\
    ( (This)->lpVtbl -> EndWrite(This,context) ) 

#define IKPhysicalLogStream_BeginRead(This,asn,callback,context)	\
    ( (This)->lpVtbl -> BeginRead(This,asn,callback,context) ) 

#define IKPhysicalLogStream_EndRead(This,context,optionalVersion,metadataSize,resultingMetadata,resultingIoBuffer)	\
    ( (This)->lpVtbl -> EndRead(This,context,optionalVersion,metadataSize,resultingMetadata,resultingIoBuffer) ) 

#define IKPhysicalLogStream_BeginReadContaining(This,asn,callback,context)	\
    ( (This)->lpVtbl -> BeginReadContaining(This,asn,callback,context) ) 

#define IKPhysicalLogStream_EndReadContaining(This,context,containingAsn,containingVersion,metadataSize,resultingMetadata,resultingIoBuffer)	\
    ( (This)->lpVtbl -> EndReadContaining(This,context,containingAsn,containingVersion,metadataSize,resultingMetadata,resultingIoBuffer) ) 

#define IKPhysicalLogStream_BeginQueryRecord(This,asn,callback,context)	\
    ( (This)->lpVtbl -> BeginQueryRecord(This,asn,callback,context) ) 

#define IKPhysicalLogStream_EndQueryRecord(This,context,optionalVersion,optionalDisposition,optionalIoBufferSize,optionalDebugInfo1)	\
    ( (This)->lpVtbl -> EndQueryRecord(This,context,optionalVersion,optionalDisposition,optionalIoBufferSize,optionalDebugInfo1) ) 

#define IKPhysicalLogStream_BeginQueryRecords(This,lowestAsn,highestAsn,resultArray,callback,context)	\
    ( (This)->lpVtbl -> BeginQueryRecords(This,lowestAsn,highestAsn,resultArray,callback,context) ) 

#define IKPhysicalLogStream_EndQueryRecords(This,context)	\
    ( (This)->lpVtbl -> EndQueryRecords(This,context) ) 

#define IKPhysicalLogStream_Truncate(This,truncationPoint,preferredTruncationPoint)	\
    ( (This)->lpVtbl -> Truncate(This,truncationPoint,preferredTruncationPoint) ) 

#define IKPhysicalLogStream_BeginWaitForNotification(This,notificationType,notificationValue,callback,context)	\
    ( (This)->lpVtbl -> BeginWaitForNotification(This,notificationType,notificationValue,callback,context) ) 

#define IKPhysicalLogStream_EndWaitForNotification(This,context)	\
    ( (This)->lpVtbl -> EndWaitForNotification(This,context) ) 

#define IKPhysicalLogStream_BeginIoctl(This,controlCode,inBuffer,callback,context)	\
    ( (This)->lpVtbl -> BeginIoctl(This,controlCode,inBuffer,callback,context) ) 

#define IKPhysicalLogStream_EndIoctl(This,context,result,outBuffer)	\
    ( (This)->lpVtbl -> EndIoctl(This,context,result,outBuffer) ) 

#define IKPhysicalLogStream_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IKPhysicalLogStream_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKPhysicalLogStream_INTERFACE_DEFINED__ */


#ifndef __IKBuffer_INTERFACE_DEFINED__
#define __IKBuffer_INTERFACE_DEFINED__

/* interface IKBuffer */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKBuffer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89ab6225-2cb4-48b4-be5c-04bd441c774f")
    IKBuffer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBuffer( 
            /* [out] */ BYTE **const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QuerySize( 
            /* [out] */ ULONG *const result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKBufferVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKBuffer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKBuffer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKBuffer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IKBuffer * This,
            /* [out] */ BYTE **const result);
        
        HRESULT ( STDMETHODCALLTYPE *QuerySize )( 
            IKBuffer * This,
            /* [out] */ ULONG *const result);
        
        END_INTERFACE
    } IKBufferVtbl;

    interface IKBuffer
    {
        CONST_VTBL struct IKBufferVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKBuffer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKBuffer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKBuffer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKBuffer_GetBuffer(This,result)	\
    ( (This)->lpVtbl -> GetBuffer(This,result) ) 

#define IKBuffer_QuerySize(This,result)	\
    ( (This)->lpVtbl -> QuerySize(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKBuffer_INTERFACE_DEFINED__ */


#ifndef __IKIoBuffer_INTERFACE_DEFINED__
#define __IKIoBuffer_INTERFACE_DEFINED__

/* interface IKIoBuffer */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKIoBuffer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("29a7c7a2-a96e-419b-b171-6263625b6219")
    IKIoBuffer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddIoBufferElement( 
            /* [in] */ IKIoBufferElement *const element) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryNumberOfIoBufferElements( 
            /* [out] */ ULONG *const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QuerySize( 
            /* [out] */ ULONG *const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE First( 
            /* [out] */ void **const resultToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ void *const currentToken,
            /* [out] */ void **const resultToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddIoBuffer( 
            /* [in] */ IKIoBuffer *const toAdd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddIoBufferReference( 
            /* [in] */ IKIoBuffer *const sourceIoBuffer,
            /* [in] */ ULONG sourceStartingOffset,
            /* [in] */ ULONG size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryElementInfo( 
            /* [in] */ void *const token,
            /* [out] */ BYTE **const resultBuffer,
            /* [out] */ ULONG *const resultSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetElement( 
            /* [in] */ void *const fromToken,
            /* [out] */ IKIoBufferElement **const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetElements( 
            /* [out] */ ULONG *const numberOfElements,
            /* [out] */ IKArray **const result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKIoBufferVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKIoBuffer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKIoBuffer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKIoBuffer * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddIoBufferElement )( 
            IKIoBuffer * This,
            /* [in] */ IKIoBufferElement *const element);
        
        HRESULT ( STDMETHODCALLTYPE *QueryNumberOfIoBufferElements )( 
            IKIoBuffer * This,
            /* [out] */ ULONG *const result);
        
        HRESULT ( STDMETHODCALLTYPE *QuerySize )( 
            IKIoBuffer * This,
            /* [out] */ ULONG *const result);
        
        HRESULT ( STDMETHODCALLTYPE *First )( 
            IKIoBuffer * This,
            /* [out] */ void **const resultToken);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IKIoBuffer * This,
            /* [in] */ void *const currentToken,
            /* [out] */ void **const resultToken);
        
        HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IKIoBuffer * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddIoBuffer )( 
            IKIoBuffer * This,
            /* [in] */ IKIoBuffer *const toAdd);
        
        HRESULT ( STDMETHODCALLTYPE *AddIoBufferReference )( 
            IKIoBuffer * This,
            /* [in] */ IKIoBuffer *const sourceIoBuffer,
            /* [in] */ ULONG sourceStartingOffset,
            /* [in] */ ULONG size);
        
        HRESULT ( STDMETHODCALLTYPE *QueryElementInfo )( 
            IKIoBuffer * This,
            /* [in] */ void *const token,
            /* [out] */ BYTE **const resultBuffer,
            /* [out] */ ULONG *const resultSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetElement )( 
            IKIoBuffer * This,
            /* [in] */ void *const fromToken,
            /* [out] */ IKIoBufferElement **const result);
        
        HRESULT ( STDMETHODCALLTYPE *GetElements )( 
            IKIoBuffer * This,
            /* [out] */ ULONG *const numberOfElements,
            /* [out] */ IKArray **const result);
        
        END_INTERFACE
    } IKIoBufferVtbl;

    interface IKIoBuffer
    {
        CONST_VTBL struct IKIoBufferVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKIoBuffer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKIoBuffer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKIoBuffer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKIoBuffer_AddIoBufferElement(This,element)	\
    ( (This)->lpVtbl -> AddIoBufferElement(This,element) ) 

#define IKIoBuffer_QueryNumberOfIoBufferElements(This,result)	\
    ( (This)->lpVtbl -> QueryNumberOfIoBufferElements(This,result) ) 

#define IKIoBuffer_QuerySize(This,result)	\
    ( (This)->lpVtbl -> QuerySize(This,result) ) 

#define IKIoBuffer_First(This,resultToken)	\
    ( (This)->lpVtbl -> First(This,resultToken) ) 

#define IKIoBuffer_Next(This,currentToken,resultToken)	\
    ( (This)->lpVtbl -> Next(This,currentToken,resultToken) ) 

#define IKIoBuffer_Clear(This)	\
    ( (This)->lpVtbl -> Clear(This) ) 

#define IKIoBuffer_AddIoBuffer(This,toAdd)	\
    ( (This)->lpVtbl -> AddIoBuffer(This,toAdd) ) 

#define IKIoBuffer_AddIoBufferReference(This,sourceIoBuffer,sourceStartingOffset,size)	\
    ( (This)->lpVtbl -> AddIoBufferReference(This,sourceIoBuffer,sourceStartingOffset,size) ) 

#define IKIoBuffer_QueryElementInfo(This,token,resultBuffer,resultSize)	\
    ( (This)->lpVtbl -> QueryElementInfo(This,token,resultBuffer,resultSize) ) 

#define IKIoBuffer_GetElement(This,fromToken,result)	\
    ( (This)->lpVtbl -> GetElement(This,fromToken,result) ) 

#define IKIoBuffer_GetElements(This,numberOfElements,result)	\
    ( (This)->lpVtbl -> GetElements(This,numberOfElements,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKIoBuffer_INTERFACE_DEFINED__ */


#ifndef __IKIoBufferElement_INTERFACE_DEFINED__
#define __IKIoBufferElement_INTERFACE_DEFINED__

/* interface IKIoBufferElement */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKIoBufferElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("61f6b9bf-1d08-4d04-b97f-e645782e7981")
    IKIoBufferElement : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBuffer( 
            /* [out] */ BYTE **const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QuerySize( 
            /* [out] */ ULONG *const result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKIoBufferElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKIoBufferElement * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKIoBufferElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKIoBufferElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            IKIoBufferElement * This,
            /* [out] */ BYTE **const result);
        
        HRESULT ( STDMETHODCALLTYPE *QuerySize )( 
            IKIoBufferElement * This,
            /* [out] */ ULONG *const result);
        
        END_INTERFACE
    } IKIoBufferElementVtbl;

    interface IKIoBufferElement
    {
        CONST_VTBL struct IKIoBufferElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKIoBufferElement_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKIoBufferElement_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKIoBufferElement_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKIoBufferElement_GetBuffer(This,result)	\
    ( (This)->lpVtbl -> GetBuffer(This,result) ) 

#define IKIoBufferElement_QuerySize(This,result)	\
    ( (This)->lpVtbl -> QuerySize(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKIoBufferElement_INTERFACE_DEFINED__ */


#ifndef __IKArray_INTERFACE_DEFINED__
#define __IKArray_INTERFACE_DEFINED__

/* interface IKArray */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKArray;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b5a6bf9e-8247-40ce-aa31-acf8b099bd1c")
    IKArray : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStatus( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG *const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetArrayBase( 
            /* [out] */ void **const result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppendGuid( 
            /* [in] */ GUID *const toAdd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AppendRecordMetadata( 
            /* [in] */ KPHYSICAL_LOG_STREAM_RecordMetadata *const toAdd) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKArrayVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKArray * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKArray * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IKArray * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IKArray * This,
            /* [out] */ ULONG *const result);
        
        HRESULT ( STDMETHODCALLTYPE *GetArrayBase )( 
            IKArray * This,
            /* [out] */ void **const result);
        
        HRESULT ( STDMETHODCALLTYPE *AppendGuid )( 
            IKArray * This,
            /* [in] */ GUID *const toAdd);
        
        HRESULT ( STDMETHODCALLTYPE *AppendRecordMetadata )( 
            IKArray * This,
            /* [in] */ KPHYSICAL_LOG_STREAM_RecordMetadata *const toAdd);
        
        END_INTERFACE
    } IKArrayVtbl;

    interface IKArray
    {
        CONST_VTBL struct IKArrayVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKArray_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKArray_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKArray_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKArray_GetStatus(This)	\
    ( (This)->lpVtbl -> GetStatus(This) ) 

#define IKArray_GetCount(This,result)	\
    ( (This)->lpVtbl -> GetCount(This,result) ) 

#define IKArray_GetArrayBase(This,result)	\
    ( (This)->lpVtbl -> GetArrayBase(This,result) ) 

#define IKArray_AppendGuid(This,toAdd)	\
    ( (This)->lpVtbl -> AppendGuid(This,toAdd) ) 

#define IKArray_AppendRecordMetadata(This,toAdd)	\
    ( (This)->lpVtbl -> AppendRecordMetadata(This,toAdd) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKArray_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_KPhysicalLog;

#ifdef __cplusplus

class DECLSPEC_UUID("eb324718-9ae0-4b66-a351-8819c4907a79")
KPhysicalLog;
#endif


#ifndef __Ktl_MODULE_DEFINED__
#define __Ktl_MODULE_DEFINED__


/* module Ktl */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT KCreatePhysicalLogManager( 
    /* [out] */ __RPC__deref_out_opt IKPhysicalLogManager **const result);

/* [entry] */ HRESULT KCreatePhysicalLogManagerInproc( 
    /* [out] */ __RPC__deref_out_opt IKPhysicalLogManager **const result);

/* [entry] */ HRESULT KCreateEmptyKIoBuffer( 
    /* [out] */ __RPC__deref_out_opt IKIoBuffer **const result);

/* [entry] */ HRESULT KCreateSimpleKIoBuffer( 
    /* [in] */ ULONG size,
    /* [out] */ __RPC__deref_out_opt IKIoBuffer **const result);

/* [entry] */ HRESULT KCreateKIoBufferElement( 
    /* [in] */ ULONG size,
    /* [out] */ __RPC__deref_out_opt IKIoBufferElement **const result);

/* [entry] */ HRESULT KCreateKBuffer( 
    /* [in] */ ULONG size,
    /* [out] */ __RPC__deref_out_opt IKBuffer **const result);

/* [entry] */ HRESULT KCreateGuidIKArray( 
    /* [in] */ ULONG initialSize,
    /* [out] */ __RPC__deref_out_opt IKArray **const result);

/* [entry] */ HRESULT KCreateLogRecordMetadataIKArray( 
    /* [in] */ ULONG initialSize,
    /* [out] */ __RPC__deref_out_opt IKArray **const result);

/* [entry] */ HRESULT KCopyMemory( 
    /* [in] */ ULONGLONG sourcePtr,
    /* [in] */ ULONGLONG destPtr,
    /* [in] */ ULONG size);


/* [entry] */ ULONGLONG KCrc64(
    /* [in] */ const VOID* Source,
    /* [in] */ ULONG Size,
    /* [in] */ ULONGLONG InitialCrc);

/* [entry] */ HRESULT QueryUserBuildInformation(
    /* [out] */ ULONG* const BuildNumber,
    /* [out] */ byte* const IsFreeBuild);

#endif /* __Ktl_MODULE_DEFINED__ */
#endif /* __KPhysicalLog_Lib_LIBRARY_DEFINED__ */

#ifndef __IKPhysicalLogStream2_INTERFACE_DEFINED__
#define __IKPhysicalLogStream2_INTERFACE_DEFINED__

/* interface IKPhysicalLogStream2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IKPhysicalLogStream2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E40B452B-03F9-4b6e-8229-53C509B87F2F")
    IKPhysicalLogStream2 : public IKPhysicalLogStream
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginMultiRecordRead( 
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ ULONG BytesToRead,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndMultiRecordRead( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IKPhysicalLogStream2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IKPhysicalLogStream2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IKPhysicalLogStream2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsFunctional )( 
            IKPhysicalLogStream2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryLogStreamId )( 
            IKPhysicalLogStream2 * This,
            /* [out] */ KTL_LOG_STREAM_ID *const result);
        
        HRESULT ( STDMETHODCALLTYPE *QueryReservedMetadataSize )( 
            IKPhysicalLogStream2 * This,
            /* [out] */ ULONG *const result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginQueryRecordRange )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndQueryRecordRange )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_ASN *const optionalLowestAsn,
            /* [out] */ KTL_LOG_ASN *const optionalHighestAsn,
            /* [out] */ KTL_LOG_ASN *const optionalLogTruncationAsn);
        
        HRESULT ( STDMETHODCALLTYPE *BeginWrite )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ ULONGLONG version,
            /* [in] */ ULONG metadataSize,
            /* [in] */ IKIoBuffer *const metadataBuffer,
            /* [in] */ IKIoBuffer *const ioBuffer,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndWrite )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRead )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRead )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONGLONG *const optionalVersion,
            /* [out] */ ULONG *const metadataSize,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReadContaining )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReadContaining )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ KTL_LOG_ASN *const containingAsn,
            /* [out] */ ULONGLONG *const containingVersion,
            /* [out] */ ULONG *const metadataSize,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *BeginQueryRecord )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndQueryRecord )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONGLONG *const optionalVersion,
            /* [out] */ KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION *const optionalDisposition,
            /* [out] */ ULONG *const optionalIoBufferSize,
            /* [out] */ ULONGLONG *const optionalDebugInfo1);
        
        HRESULT ( STDMETHODCALLTYPE *BeginQueryRecords )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KTL_LOG_ASN lowestAsn,
            /* [in] */ KTL_LOG_ASN highestAsn,
            /* [in] */ IKArray *const resultArray,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndQueryRecords )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *Truncate )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KTL_LOG_ASN truncationPoint,
            /* [in] */ KTL_LOG_ASN preferredTruncationPoint);
        
        HRESULT ( STDMETHODCALLTYPE *BeginWaitForNotification )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE notificationType,
            /* [in] */ ULONGLONG notificationValue,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndWaitForNotification )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginIoctl )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ ULONG controlCode,
            /* [in] */ IKBuffer *const inBuffer,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndIoctl )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ ULONG *const result,
            /* [out] */ IKBuffer **const outBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginMultiRecordRead )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ KTL_LOG_ASN asn,
            /* [in] */ ULONG BytesToRead,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndMultiRecordRead )( 
            IKPhysicalLogStream2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IKIoBuffer **const resultingMetadata,
            /* [out] */ IKIoBuffer **const resultingIoBuffer);
        
        END_INTERFACE
    } IKPhysicalLogStream2Vtbl;

    interface IKPhysicalLogStream2
    {
        CONST_VTBL struct IKPhysicalLogStream2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IKPhysicalLogStream2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IKPhysicalLogStream2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IKPhysicalLogStream2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IKPhysicalLogStream2_IsFunctional(This)	\
    ( (This)->lpVtbl -> IsFunctional(This) ) 

#define IKPhysicalLogStream2_QueryLogStreamId(This,result)	\
    ( (This)->lpVtbl -> QueryLogStreamId(This,result) ) 

#define IKPhysicalLogStream2_QueryReservedMetadataSize(This,result)	\
    ( (This)->lpVtbl -> QueryReservedMetadataSize(This,result) ) 

#define IKPhysicalLogStream2_BeginQueryRecordRange(This,callback,context)	\
    ( (This)->lpVtbl -> BeginQueryRecordRange(This,callback,context) ) 

#define IKPhysicalLogStream2_EndQueryRecordRange(This,context,optionalLowestAsn,optionalHighestAsn,optionalLogTruncationAsn)	\
    ( (This)->lpVtbl -> EndQueryRecordRange(This,context,optionalLowestAsn,optionalHighestAsn,optionalLogTruncationAsn) ) 

#define IKPhysicalLogStream2_BeginWrite(This,asn,version,metadataSize,metadataBuffer,ioBuffer,callback,context)	\
    ( (This)->lpVtbl -> BeginWrite(This,asn,version,metadataSize,metadataBuffer,ioBuffer,callback,context) ) 

#define IKPhysicalLogStream2_EndWrite(This,context)	\
    ( (This)->lpVtbl -> EndWrite(This,context) ) 

#define IKPhysicalLogStream2_BeginRead(This,asn,callback,context)	\
    ( (This)->lpVtbl -> BeginRead(This,asn,callback,context) ) 

#define IKPhysicalLogStream2_EndRead(This,context,optionalVersion,metadataSize,resultingMetadata,resultingIoBuffer)	\
    ( (This)->lpVtbl -> EndRead(This,context,optionalVersion,metadataSize,resultingMetadata,resultingIoBuffer) ) 

#define IKPhysicalLogStream2_BeginReadContaining(This,asn,callback,context)	\
    ( (This)->lpVtbl -> BeginReadContaining(This,asn,callback,context) ) 

#define IKPhysicalLogStream2_EndReadContaining(This,context,containingAsn,containingVersion,metadataSize,resultingMetadata,resultingIoBuffer)	\
    ( (This)->lpVtbl -> EndReadContaining(This,context,containingAsn,containingVersion,metadataSize,resultingMetadata,resultingIoBuffer) ) 

#define IKPhysicalLogStream2_BeginQueryRecord(This,asn,callback,context)	\
    ( (This)->lpVtbl -> BeginQueryRecord(This,asn,callback,context) ) 

#define IKPhysicalLogStream2_EndQueryRecord(This,context,optionalVersion,optionalDisposition,optionalIoBufferSize,optionalDebugInfo1)	\
    ( (This)->lpVtbl -> EndQueryRecord(This,context,optionalVersion,optionalDisposition,optionalIoBufferSize,optionalDebugInfo1) ) 

#define IKPhysicalLogStream2_BeginQueryRecords(This,lowestAsn,highestAsn,resultArray,callback,context)	\
    ( (This)->lpVtbl -> BeginQueryRecords(This,lowestAsn,highestAsn,resultArray,callback,context) ) 

#define IKPhysicalLogStream2_EndQueryRecords(This,context)	\
    ( (This)->lpVtbl -> EndQueryRecords(This,context) ) 

#define IKPhysicalLogStream2_Truncate(This,truncationPoint,preferredTruncationPoint)	\
    ( (This)->lpVtbl -> Truncate(This,truncationPoint,preferredTruncationPoint) ) 

#define IKPhysicalLogStream2_BeginWaitForNotification(This,notificationType,notificationValue,callback,context)	\
    ( (This)->lpVtbl -> BeginWaitForNotification(This,notificationType,notificationValue,callback,context) ) 

#define IKPhysicalLogStream2_EndWaitForNotification(This,context)	\
    ( (This)->lpVtbl -> EndWaitForNotification(This,context) ) 

#define IKPhysicalLogStream2_BeginIoctl(This,controlCode,inBuffer,callback,context)	\
    ( (This)->lpVtbl -> BeginIoctl(This,controlCode,inBuffer,callback,context) ) 

#define IKPhysicalLogStream2_EndIoctl(This,context,result,outBuffer)	\
    ( (This)->lpVtbl -> EndIoctl(This,context,result,outBuffer) ) 

#define IKPhysicalLogStream2_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IKPhysicalLogStream2_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 


#define IKPhysicalLogStream2_BeginMultiRecordRead(This,asn,BytesToRead,callback,context)	\
    ( (This)->lpVtbl -> BeginMultiRecordRead(This,asn,BytesToRead,callback,context) ) 

#define IKPhysicalLogStream2_EndMultiRecordRead(This,context,resultingMetadata,resultingIoBuffer)	\
    ( (This)->lpVtbl -> EndMultiRecordRead(This,context,resultingMetadata,resultingIoBuffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IKPhysicalLogStream2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


