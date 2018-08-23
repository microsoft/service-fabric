

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

#ifndef __fabricimagestore__h__
#define __fabricimagestore__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricNativeImageStoreClient_FWD_DEFINED__
#define __IFabricNativeImageStoreClient_FWD_DEFINED__
typedef interface IFabricNativeImageStoreClient IFabricNativeImageStoreClient;

#endif 	/* __IFabricNativeImageStoreClient_FWD_DEFINED__ */


#ifndef __FabricNativeImageStoreClient_FWD_DEFINED__
#define __FabricNativeImageStoreClient_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricNativeImageStoreClient FabricNativeImageStoreClient;
#else
typedef struct FabricNativeImageStoreClient FabricNativeImageStoreClient;
#endif /* __cplusplus */

#endif 	/* __FabricNativeImageStoreClient_FWD_DEFINED__ */


#ifndef __IFabricNativeImageStoreProgressEventHandler_FWD_DEFINED__
#define __IFabricNativeImageStoreProgressEventHandler_FWD_DEFINED__
typedef interface IFabricNativeImageStoreProgressEventHandler IFabricNativeImageStoreProgressEventHandler;

#endif 	/* __IFabricNativeImageStoreProgressEventHandler_FWD_DEFINED__ */


#ifndef __IFabricNativeImageStoreClient_FWD_DEFINED__
#define __IFabricNativeImageStoreClient_FWD_DEFINED__
typedef interface IFabricNativeImageStoreClient IFabricNativeImageStoreClient;

#endif 	/* __IFabricNativeImageStoreClient_FWD_DEFINED__ */


#ifndef __IFabricNativeImageStoreClient2_FWD_DEFINED__
#define __IFabricNativeImageStoreClient2_FWD_DEFINED__
typedef interface IFabricNativeImageStoreClient2 IFabricNativeImageStoreClient2;

#endif 	/* __IFabricNativeImageStoreClient2_FWD_DEFINED__ */


#ifndef __IFabricNativeImageStoreClient3_FWD_DEFINED__
#define __IFabricNativeImageStoreClient3_FWD_DEFINED__
typedef interface IFabricNativeImageStoreClient3 IFabricNativeImageStoreClient3;

#endif 	/* __IFabricNativeImageStoreClient3_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricimagestore__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif




extern RPC_IF_HANDLE __MIDL_itf_fabricimagestore__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricimagestore__0000_0000_v0_0_s_ifspec;


#ifndef __FabricImageStoreLib_LIBRARY_DEFINED__
#define __FabricImageStoreLib_LIBRARY_DEFINED__

/* library FabricImageStoreLib */
/* [version][uuid] */ 


#pragma pack(push, 8)


#pragma pack(pop)

EXTERN_C const IID LIBID_FabricImageStoreLib;

#ifndef __IFabricNativeImageStoreClient_INTERFACE_DEFINED__
#define __IFabricNativeImageStoreClient_INTERFACE_DEFINED__

/* interface IFabricNativeImageStoreClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricNativeImageStoreClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f6b3ceea-3577-49fa-8e67-2d0ad1024c79")
    IFabricNativeImageStoreClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSecurityCredentials( 
            /* [in] */ const FABRIC_SECURITY_CREDENTIALS *securityCredentials) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UploadContent( 
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyContent( 
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_STRING_LIST *skipFiles,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
            /* [in] */ BOOLEAN checkMarkFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DownloadContent( 
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR localDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ListContent( 
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ IFabricStringListResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ListContentWithDetails( 
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ BOOLEAN isRecursive,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DoesContentExist( 
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ BOOLEAN *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteContent( 
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricNativeImageStoreClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricNativeImageStoreClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricNativeImageStoreClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSecurityCredentials )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ const FABRIC_SECURITY_CREDENTIALS *securityCredentials);
        
        HRESULT ( STDMETHODCALLTYPE *UploadContent )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *CopyContent )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_STRING_LIST *skipFiles,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
            /* [in] */ BOOLEAN checkMarkFile);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadContent )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR localDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *ListContent )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *ListContentWithDetails )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ BOOLEAN isRecursive,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT **result);
        
        HRESULT ( STDMETHODCALLTYPE *DoesContentExist )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteContent )( 
            IFabricNativeImageStoreClient * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds);
        
        END_INTERFACE
    } IFabricNativeImageStoreClientVtbl;

    interface IFabricNativeImageStoreClient
    {
        CONST_VTBL struct IFabricNativeImageStoreClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricNativeImageStoreClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricNativeImageStoreClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricNativeImageStoreClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricNativeImageStoreClient_SetSecurityCredentials(This,securityCredentials)	\
    ( (This)->lpVtbl -> SetSecurityCredentials(This,securityCredentials) ) 

#define IFabricNativeImageStoreClient_UploadContent(This,remoteDestination,localSource,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> UploadContent(This,remoteDestination,localSource,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient_CopyContent(This,remoteSource,remoteDestination,timeoutMilliseconds,skipFiles,copyFlag,checkMarkFile)	\
    ( (This)->lpVtbl -> CopyContent(This,remoteSource,remoteDestination,timeoutMilliseconds,skipFiles,copyFlag,checkMarkFile) ) 

#define IFabricNativeImageStoreClient_DownloadContent(This,remoteSource,localDestination,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> DownloadContent(This,remoteSource,localDestination,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient_ListContent(This,remoteLocation,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListContent(This,remoteLocation,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient_ListContentWithDetails(This,remoteLocation,isRecursive,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListContentWithDetails(This,remoteLocation,isRecursive,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient_DoesContentExist(This,remoteLocation,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> DoesContentExist(This,remoteLocation,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient_DeleteContent(This,remoteLocation,timeoutMilliseconds)	\
    ( (This)->lpVtbl -> DeleteContent(This,remoteLocation,timeoutMilliseconds) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricNativeImageStoreClient_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricNativeImageStoreClient;

#ifdef __cplusplus

class DECLSPEC_UUID("49f77ae3-ac64-4f9e-936e-4058c570a387")
FabricNativeImageStoreClient;
#endif


#ifndef __FabricImageStore_MODULE_DEFINED__
#define __FabricImageStore_MODULE_DEFINED__


/* module FabricImageStore */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT FabricCreateNativeImageStoreClient( 
    /* [in] */ BOOLEAN isInternal,
    /* [in] */ __RPC__in LPCWSTR workingDirectory,
    /* [in] */ USHORT connectionStringsSize,
    /* [size_is][in] */ __RPC__in_ecount_full(connectionStringsSize) const LPCWSTR *connectionStrings,
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **nativeImageStoreClient);

/* [entry] */ HRESULT FabricCreateLocalNativeImageStoreClient( 
    /* [in] */ BOOLEAN isInternal,
    /* [in] */ __RPC__in LPCWSTR workingDirectory,
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **nativeImageStoreClient);

/* [entry] */ HRESULT ArchiveApplicationPackage( 
    /* [in] */ __RPC__in LPCWSTR appPackageRootDirectory,
    /* [in] */ __RPC__in_opt const IFabricNativeImageStoreProgressEventHandler *progressHandler);

/* [entry] */ HRESULT TryExtractApplicationPackage( 
    /* [in] */ __RPC__in LPCWSTR appPackageRootDirectory,
    /* [in] */ __RPC__in_opt const IFabricNativeImageStoreProgressEventHandler *progressHandler,
    /* [retval][out] */ __RPC__out BOOLEAN *archiveExists);

/* [entry] */ HRESULT GenerateSfpkg( 
    /* [in] */ __RPC__in LPCWSTR appPackageRootDirectory,
    /* [in] */ __RPC__in LPCWSTR destinationDirectory,
    /* [in] */ BOOLEAN applyCompression,
    /* [in] */ __RPC__in LPCWSTR sfPkgName,
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **sfPkgFilePath);

/* [entry] */ HRESULT ExpandSfpkg( 
    /* [in] */ __RPC__in LPCWSTR sfPkgFilePath,
    /* [in] */ __RPC__in LPCWSTR appPackageRootDirectory);

#endif /* __FabricImageStore_MODULE_DEFINED__ */
#endif /* __FabricImageStoreLib_LIBRARY_DEFINED__ */

#ifndef __IFabricNativeImageStoreProgressEventHandler_INTERFACE_DEFINED__
#define __IFabricNativeImageStoreProgressEventHandler_INTERFACE_DEFINED__

/* interface IFabricNativeImageStoreProgressEventHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricNativeImageStoreProgressEventHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("62119833-ad63-47d8-bb8b-167483f7b05a")
    IFabricNativeImageStoreProgressEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetUpdateInterval( 
            /* [out] */ DWORD *milliseconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUpdateProgress( 
            /* [in] */ ULONGLONG completedItems,
            /* [in] */ ULONGLONG totalItems,
            /* [in] */ FABRIC_PROGRESS_UNIT_TYPE ItemType) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricNativeImageStoreProgressEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricNativeImageStoreProgressEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricNativeImageStoreProgressEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricNativeImageStoreProgressEventHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetUpdateInterval )( 
            IFabricNativeImageStoreProgressEventHandler * This,
            /* [out] */ DWORD *milliseconds);
        
        HRESULT ( STDMETHODCALLTYPE *OnUpdateProgress )( 
            IFabricNativeImageStoreProgressEventHandler * This,
            /* [in] */ ULONGLONG completedItems,
            /* [in] */ ULONGLONG totalItems,
            /* [in] */ FABRIC_PROGRESS_UNIT_TYPE ItemType);
        
        END_INTERFACE
    } IFabricNativeImageStoreProgressEventHandlerVtbl;

    interface IFabricNativeImageStoreProgressEventHandler
    {
        CONST_VTBL struct IFabricNativeImageStoreProgressEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricNativeImageStoreProgressEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricNativeImageStoreProgressEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricNativeImageStoreProgressEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricNativeImageStoreProgressEventHandler_GetUpdateInterval(This,milliseconds)	\
    ( (This)->lpVtbl -> GetUpdateInterval(This,milliseconds) ) 

#define IFabricNativeImageStoreProgressEventHandler_OnUpdateProgress(This,completedItems,totalItems,ItemType)	\
    ( (This)->lpVtbl -> OnUpdateProgress(This,completedItems,totalItems,ItemType) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricNativeImageStoreProgressEventHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricNativeImageStoreClient2_INTERFACE_DEFINED__
#define __IFabricNativeImageStoreClient2_INTERFACE_DEFINED__

/* interface IFabricNativeImageStoreClient2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricNativeImageStoreClient2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80fa7785-4ad1-45b1-9e21-0b8c0307c22f")
    IFabricNativeImageStoreClient2 : public IFabricNativeImageStoreClient
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DownloadContent2( 
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR localDestination,
            /* [in] */ const IFabricNativeImageStoreProgressEventHandler *progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UploadContent2( 
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ const IFabricNativeImageStoreProgressEventHandler *progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricNativeImageStoreClient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricNativeImageStoreClient2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricNativeImageStoreClient2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSecurityCredentials )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ const FABRIC_SECURITY_CREDENTIALS *securityCredentials);
        
        HRESULT ( STDMETHODCALLTYPE *UploadContent )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *CopyContent )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_STRING_LIST *skipFiles,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
            /* [in] */ BOOLEAN checkMarkFile);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadContent )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR localDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *ListContent )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *ListContentWithDetails )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ BOOLEAN isRecursive,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT **result);
        
        HRESULT ( STDMETHODCALLTYPE *DoesContentExist )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteContent )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadContent2 )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR localDestination,
            /* [in] */ const IFabricNativeImageStoreProgressEventHandler *progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *UploadContent2 )( 
            IFabricNativeImageStoreClient2 * This,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ const IFabricNativeImageStoreProgressEventHandler *progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        END_INTERFACE
    } IFabricNativeImageStoreClient2Vtbl;

    interface IFabricNativeImageStoreClient2
    {
        CONST_VTBL struct IFabricNativeImageStoreClient2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricNativeImageStoreClient2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricNativeImageStoreClient2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricNativeImageStoreClient2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricNativeImageStoreClient2_SetSecurityCredentials(This,securityCredentials)	\
    ( (This)->lpVtbl -> SetSecurityCredentials(This,securityCredentials) ) 

#define IFabricNativeImageStoreClient2_UploadContent(This,remoteDestination,localSource,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> UploadContent(This,remoteDestination,localSource,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient2_CopyContent(This,remoteSource,remoteDestination,timeoutMilliseconds,skipFiles,copyFlag,checkMarkFile)	\
    ( (This)->lpVtbl -> CopyContent(This,remoteSource,remoteDestination,timeoutMilliseconds,skipFiles,copyFlag,checkMarkFile) ) 

#define IFabricNativeImageStoreClient2_DownloadContent(This,remoteSource,localDestination,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> DownloadContent(This,remoteSource,localDestination,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient2_ListContent(This,remoteLocation,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListContent(This,remoteLocation,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient2_ListContentWithDetails(This,remoteLocation,isRecursive,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListContentWithDetails(This,remoteLocation,isRecursive,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient2_DoesContentExist(This,remoteLocation,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> DoesContentExist(This,remoteLocation,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient2_DeleteContent(This,remoteLocation,timeoutMilliseconds)	\
    ( (This)->lpVtbl -> DeleteContent(This,remoteLocation,timeoutMilliseconds) ) 


#define IFabricNativeImageStoreClient2_DownloadContent2(This,remoteSource,localDestination,progressHandler,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> DownloadContent2(This,remoteSource,localDestination,progressHandler,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient2_UploadContent2(This,remoteDestination,localSource,progressHandler,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> UploadContent2(This,remoteDestination,localSource,progressHandler,timeoutMilliseconds,copyFlag) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricNativeImageStoreClient2_INTERFACE_DEFINED__ */


#ifndef __IFabricNativeImageStoreClient3_INTERFACE_DEFINED__
#define __IFabricNativeImageStoreClient3_INTERFACE_DEFINED__

/* interface IFabricNativeImageStoreClient3 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricNativeImageStoreClient3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d5e63df3-ffb1-4837-9959-2a5b1da94f33")
    IFabricNativeImageStoreClient3 : public IFabricNativeImageStoreClient2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ListPagedContent( 
            /* [in] */ const FABRIC_IMAGE_STORE_LIST_DESCRIPTION *listDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ListPagedContentWithDetails( 
            /* [in] */ const FABRIC_IMAGE_STORE_LIST_DESCRIPTION *listDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricNativeImageStoreClient3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricNativeImageStoreClient3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricNativeImageStoreClient3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSecurityCredentials )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ const FABRIC_SECURITY_CREDENTIALS *securityCredentials);
        
        HRESULT ( STDMETHODCALLTYPE *UploadContent )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *CopyContent )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_STRING_LIST *skipFiles,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
            /* [in] */ BOOLEAN checkMarkFile);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadContent )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR localDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *ListContent )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *ListContentWithDetails )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ BOOLEAN isRecursive,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT **result);
        
        HRESULT ( STDMETHODCALLTYPE *DoesContentExist )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteContent )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadContent2 )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR localDestination,
            /* [in] */ const IFabricNativeImageStoreProgressEventHandler *progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *UploadContent2 )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ const IFabricNativeImageStoreProgressEventHandler *progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        
        HRESULT ( STDMETHODCALLTYPE *ListPagedContent )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ const FABRIC_IMAGE_STORE_LIST_DESCRIPTION *listDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT **result);
        
        HRESULT ( STDMETHODCALLTYPE *ListPagedContentWithDetails )( 
            IFabricNativeImageStoreClient3 * This,
            /* [in] */ const FABRIC_IMAGE_STORE_LIST_DESCRIPTION *listDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [retval][out] */ FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT **result);
        
        END_INTERFACE
    } IFabricNativeImageStoreClient3Vtbl;

    interface IFabricNativeImageStoreClient3
    {
        CONST_VTBL struct IFabricNativeImageStoreClient3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricNativeImageStoreClient3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricNativeImageStoreClient3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricNativeImageStoreClient3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricNativeImageStoreClient3_SetSecurityCredentials(This,securityCredentials)	\
    ( (This)->lpVtbl -> SetSecurityCredentials(This,securityCredentials) ) 

#define IFabricNativeImageStoreClient3_UploadContent(This,remoteDestination,localSource,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> UploadContent(This,remoteDestination,localSource,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient3_CopyContent(This,remoteSource,remoteDestination,timeoutMilliseconds,skipFiles,copyFlag,checkMarkFile)	\
    ( (This)->lpVtbl -> CopyContent(This,remoteSource,remoteDestination,timeoutMilliseconds,skipFiles,copyFlag,checkMarkFile) ) 

#define IFabricNativeImageStoreClient3_DownloadContent(This,remoteSource,localDestination,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> DownloadContent(This,remoteSource,localDestination,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient3_ListContent(This,remoteLocation,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListContent(This,remoteLocation,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient3_ListContentWithDetails(This,remoteLocation,isRecursive,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListContentWithDetails(This,remoteLocation,isRecursive,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient3_DoesContentExist(This,remoteLocation,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> DoesContentExist(This,remoteLocation,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient3_DeleteContent(This,remoteLocation,timeoutMilliseconds)	\
    ( (This)->lpVtbl -> DeleteContent(This,remoteLocation,timeoutMilliseconds) ) 


#define IFabricNativeImageStoreClient3_DownloadContent2(This,remoteSource,localDestination,progressHandler,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> DownloadContent2(This,remoteSource,localDestination,progressHandler,timeoutMilliseconds,copyFlag) ) 

#define IFabricNativeImageStoreClient3_UploadContent2(This,remoteDestination,localSource,progressHandler,timeoutMilliseconds,copyFlag)	\
    ( (This)->lpVtbl -> UploadContent2(This,remoteDestination,localSource,progressHandler,timeoutMilliseconds,copyFlag) ) 


#define IFabricNativeImageStoreClient3_ListPagedContent(This,listDescription,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListPagedContent(This,listDescription,timeoutMilliseconds,result) ) 

#define IFabricNativeImageStoreClient3_ListPagedContentWithDetails(This,listDescription,timeoutMilliseconds,result)	\
    ( (This)->lpVtbl -> ListPagedContentWithDetails(This,listDescription,timeoutMilliseconds,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricNativeImageStoreClient3_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


