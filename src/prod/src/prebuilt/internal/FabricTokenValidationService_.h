

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

#ifndef __fabrictokenvalidationservice__h__
#define __fabrictokenvalidationservice__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricTokenValidationService_FWD_DEFINED__
#define __IFabricTokenValidationService_FWD_DEFINED__
typedef interface IFabricTokenValidationService IFabricTokenValidationService;

#endif 	/* __IFabricTokenValidationService_FWD_DEFINED__ */


#ifndef __IFabricTokenValidationServiceAgent_FWD_DEFINED__
#define __IFabricTokenValidationServiceAgent_FWD_DEFINED__
typedef interface IFabricTokenValidationServiceAgent IFabricTokenValidationServiceAgent;

#endif 	/* __IFabricTokenValidationServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricTokenClaimResult_FWD_DEFINED__
#define __IFabricTokenClaimResult_FWD_DEFINED__
typedef interface IFabricTokenClaimResult IFabricTokenClaimResult;

#endif 	/* __IFabricTokenClaimResult_FWD_DEFINED__ */


#ifndef __IFabricTokenServiceMetadataResult_FWD_DEFINED__
#define __IFabricTokenServiceMetadataResult_FWD_DEFINED__
typedef interface IFabricTokenServiceMetadataResult IFabricTokenServiceMetadataResult;

#endif 	/* __IFabricTokenServiceMetadataResult_FWD_DEFINED__ */


#ifndef __FabricTokenValidationServiceAgent_FWD_DEFINED__
#define __FabricTokenValidationServiceAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricTokenValidationServiceAgent FabricTokenValidationServiceAgent;
#else
typedef struct FabricTokenValidationServiceAgent FabricTokenValidationServiceAgent;
#endif /* __cplusplus */

#endif 	/* __FabricTokenValidationServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricTokenValidationService_FWD_DEFINED__
#define __IFabricTokenValidationService_FWD_DEFINED__
typedef interface IFabricTokenValidationService IFabricTokenValidationService;

#endif 	/* __IFabricTokenValidationService_FWD_DEFINED__ */


#ifndef __IFabricTokenValidationServiceAgent_FWD_DEFINED__
#define __IFabricTokenValidationServiceAgent_FWD_DEFINED__
typedef interface IFabricTokenValidationServiceAgent IFabricTokenValidationServiceAgent;

#endif 	/* __IFabricTokenValidationServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricTokenClaimResult_FWD_DEFINED__
#define __IFabricTokenClaimResult_FWD_DEFINED__
typedef interface IFabricTokenClaimResult IFabricTokenClaimResult;

#endif 	/* __IFabricTokenClaimResult_FWD_DEFINED__ */


#ifndef __IFabricTokenServiceMetadataResult_FWD_DEFINED__
#define __IFabricTokenServiceMetadataResult_FWD_DEFINED__
typedef interface IFabricTokenServiceMetadataResult IFabricTokenServiceMetadataResult;

#endif 	/* __IFabricTokenServiceMetadataResult_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricCommon_.h"
#include "FabricRuntime.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabrictokenvalidationservice__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif






extern RPC_IF_HANDLE __MIDL_itf_fabrictokenvalidationservice__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabrictokenvalidationservice__0000_0000_v0_0_s_ifspec;


#ifndef __FabricTokenValidationServiceLib_LIBRARY_DEFINED__
#define __FabricTokenValidationServiceLib_LIBRARY_DEFINED__

/* library FabricTokenValidationServiceLib */
/* [version][uuid] */ 


#pragma pack(push, 8)





#pragma pack(pop)

EXTERN_C const IID LIBID_FabricTokenValidationServiceLib;

#ifndef __IFabricTokenValidationService_INTERFACE_DEFINED__
#define __IFabricTokenValidationService_INTERFACE_DEFINED__

/* interface IFabricTokenValidationService */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTokenValidationService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a70f8024-32f8-48ab-84cf-2119a25513a9")
    IFabricTokenValidationService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginValidateToken( 
            /* [in] */ LPCWSTR authToken,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndValidateToken( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ void **claimResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTokenServiceMetadata( 
            /* [retval][out] */ IFabricTokenServiceMetadataResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTokenValidationServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTokenValidationService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTokenValidationService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTokenValidationService * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginValidateToken )( 
            IFabricTokenValidationService * This,
            /* [in] */ LPCWSTR authToken,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndValidateToken )( 
            IFabricTokenValidationService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ void **claimResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetTokenServiceMetadata )( 
            IFabricTokenValidationService * This,
            /* [retval][out] */ IFabricTokenServiceMetadataResult **result);
        
        END_INTERFACE
    } IFabricTokenValidationServiceVtbl;

    interface IFabricTokenValidationService
    {
        CONST_VTBL struct IFabricTokenValidationServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTokenValidationService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTokenValidationService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTokenValidationService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTokenValidationService_BeginValidateToken(This,authToken,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginValidateToken(This,authToken,timeoutMilliseconds,callback,context) ) 

#define IFabricTokenValidationService_EndValidateToken(This,context,claimResult)	\
    ( (This)->lpVtbl -> EndValidateToken(This,context,claimResult) ) 

#define IFabricTokenValidationService_GetTokenServiceMetadata(This,result)	\
    ( (This)->lpVtbl -> GetTokenServiceMetadata(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTokenValidationService_INTERFACE_DEFINED__ */


#ifndef __IFabricTokenValidationServiceAgent_INTERFACE_DEFINED__
#define __IFabricTokenValidationServiceAgent_INTERFACE_DEFINED__

/* interface IFabricTokenValidationServiceAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTokenValidationServiceAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("45898312-084e-4792-b234-52efb8d79cd8")
    IFabricTokenValidationServiceAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterTokenValidationService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricTokenValidationServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricTokenValidationServiceAgent0001,
            /* [in] */ IFabricTokenValidationService *__MIDL__IFabricTokenValidationServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterTokenValidationService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricTokenValidationServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricTokenValidationServiceAgent0004) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTokenValidationServiceAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTokenValidationServiceAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTokenValidationServiceAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTokenValidationServiceAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterTokenValidationService )( 
            IFabricTokenValidationServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricTokenValidationServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricTokenValidationServiceAgent0001,
            /* [in] */ IFabricTokenValidationService *__MIDL__IFabricTokenValidationServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterTokenValidationService )( 
            IFabricTokenValidationServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricTokenValidationServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricTokenValidationServiceAgent0004);
        
        END_INTERFACE
    } IFabricTokenValidationServiceAgentVtbl;

    interface IFabricTokenValidationServiceAgent
    {
        CONST_VTBL struct IFabricTokenValidationServiceAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTokenValidationServiceAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTokenValidationServiceAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTokenValidationServiceAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTokenValidationServiceAgent_RegisterTokenValidationService(This,__MIDL__IFabricTokenValidationServiceAgent0000,__MIDL__IFabricTokenValidationServiceAgent0001,__MIDL__IFabricTokenValidationServiceAgent0002,serviceAddress)	\
    ( (This)->lpVtbl -> RegisterTokenValidationService(This,__MIDL__IFabricTokenValidationServiceAgent0000,__MIDL__IFabricTokenValidationServiceAgent0001,__MIDL__IFabricTokenValidationServiceAgent0002,serviceAddress) ) 

#define IFabricTokenValidationServiceAgent_UnregisterTokenValidationService(This,__MIDL__IFabricTokenValidationServiceAgent0003,__MIDL__IFabricTokenValidationServiceAgent0004)	\
    ( (This)->lpVtbl -> UnregisterTokenValidationService(This,__MIDL__IFabricTokenValidationServiceAgent0003,__MIDL__IFabricTokenValidationServiceAgent0004) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTokenValidationServiceAgent_INTERFACE_DEFINED__ */


#ifndef __IFabricTokenClaimResult_INTERFACE_DEFINED__
#define __IFabricTokenClaimResult_INTERFACE_DEFINED__

/* interface IFabricTokenClaimResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTokenClaimResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("931ff709-4604-4e6a-bc10-a38056cbf3b4")
    IFabricTokenClaimResult : public IUnknown
    {
    public:
        virtual const FABRIC_TOKEN_CLAIM_RESULT_LIST *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTokenClaimResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTokenClaimResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTokenClaimResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTokenClaimResult * This);
        
        const FABRIC_TOKEN_CLAIM_RESULT_LIST *( STDMETHODCALLTYPE *get_Result )( 
            IFabricTokenClaimResult * This);
        
        END_INTERFACE
    } IFabricTokenClaimResultVtbl;

    interface IFabricTokenClaimResult
    {
        CONST_VTBL struct IFabricTokenClaimResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTokenClaimResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTokenClaimResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTokenClaimResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTokenClaimResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTokenClaimResult_INTERFACE_DEFINED__ */


#ifndef __IFabricTokenServiceMetadataResult_INTERFACE_DEFINED__
#define __IFabricTokenServiceMetadataResult_INTERFACE_DEFINED__

/* interface IFabricTokenServiceMetadataResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTokenServiceMetadataResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("316ea660-56ec-4459-a4ad-8170787c5c48")
    IFabricTokenServiceMetadataResult : public IUnknown
    {
    public:
        virtual const FABRIC_TOKEN_SERVICE_METADATA *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTokenServiceMetadataResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTokenServiceMetadataResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTokenServiceMetadataResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTokenServiceMetadataResult * This);
        
        const FABRIC_TOKEN_SERVICE_METADATA *( STDMETHODCALLTYPE *get_Result )( 
            IFabricTokenServiceMetadataResult * This);
        
        END_INTERFACE
    } IFabricTokenServiceMetadataResultVtbl;

    interface IFabricTokenServiceMetadataResult
    {
        CONST_VTBL struct IFabricTokenServiceMetadataResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTokenServiceMetadataResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTokenServiceMetadataResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTokenServiceMetadataResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTokenServiceMetadataResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTokenServiceMetadataResult_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricTokenValidationServiceAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("44cbab57-79c5-4961-8206-d561270d1cf1")
FabricTokenValidationServiceAgent;
#endif


#ifndef __FabricTokenValidationService_MODULE_DEFINED__
#define __FabricTokenValidationService_MODULE_DEFINED__


/* module FabricTokenValidationService */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricTokenValidationServiceAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricTokenValidationServiceAgent);

/* [entry] */ HRESULT GetDefaultAzureActiveDirectoryConfigurations( 
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringMapResult **result);

#endif /* __FabricTokenValidationService_MODULE_DEFINED__ */
#endif /* __FabricTokenValidationServiceLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


