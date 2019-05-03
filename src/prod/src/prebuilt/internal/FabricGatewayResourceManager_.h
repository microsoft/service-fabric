

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

#ifndef __FabricGatewayResourceManager__h__
#define __FabricGatewayResourceManager__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricGatewayResourceManager_FWD_DEFINED__
#define __IFabricGatewayResourceManager_FWD_DEFINED__
typedef interface IFabricGatewayResourceManager IFabricGatewayResourceManager;

#endif 	/* __IFabricGatewayResourceManager_FWD_DEFINED__ */


#ifndef __IFabricGatewayResourceManagerAgent_FWD_DEFINED__
#define __IFabricGatewayResourceManagerAgent_FWD_DEFINED__
typedef interface IFabricGatewayResourceManagerAgent IFabricGatewayResourceManagerAgent;

#endif 	/* __IFabricGatewayResourceManagerAgent_FWD_DEFINED__ */


#ifndef __FabricGatewayResourceManagerAgent_FWD_DEFINED__
#define __FabricGatewayResourceManagerAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricGatewayResourceManagerAgent FabricGatewayResourceManagerAgent;
#else
typedef struct FabricGatewayResourceManagerAgent FabricGatewayResourceManagerAgent;
#endif /* __cplusplus */

#endif 	/* __FabricGatewayResourceManagerAgent_FWD_DEFINED__ */


#ifndef __IFabricGatewayResourceManager_FWD_DEFINED__
#define __IFabricGatewayResourceManager_FWD_DEFINED__
typedef interface IFabricGatewayResourceManager IFabricGatewayResourceManager;

#endif 	/* __IFabricGatewayResourceManager_FWD_DEFINED__ */


#ifndef __IFabricGatewayResourceManagerAgent_FWD_DEFINED__
#define __IFabricGatewayResourceManagerAgent_FWD_DEFINED__
typedef interface IFabricGatewayResourceManagerAgent IFabricGatewayResourceManagerAgent;

#endif 	/* __IFabricGatewayResourceManagerAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_FabricGatewayResourceManager__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif




extern RPC_IF_HANDLE __MIDL_itf_FabricGatewayResourceManager__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_FabricGatewayResourceManager__0000_0000_v0_0_s_ifspec;


#ifndef __FabricGateResourceManagerLib_LIBRARY_DEFINED__
#define __FabricGateResourceManagerLib_LIBRARY_DEFINED__

/* library FabricGateResourceManagerLib */
/* [version][uuid] */ 


#pragma pack(push, 8)



#pragma pack(pop)

EXTERN_C const IID LIBID_FabricGateResourceManagerLib;

#ifndef __IFabricGatewayResourceManager_INTERFACE_DEFINED__
#define __IFabricGatewayResourceManager_INTERFACE_DEFINED__

/* interface IFabricGatewayResourceManager */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricGatewayResourceManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bf9b93a9-5b74-4a28-b205-edf387adf3db")
    IFabricGatewayResourceManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginCreateOrUpdateGatewayResource( 
            /* [in] */ LPCWSTR gatewayResourceDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCreateOrUpdateGatewayResource( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetGatewayResourceList( 
            /* [in] */ LPCWSTR gatewayResourceName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetGatewayResourceList( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringListResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeleteGatewayResource( 
            /* [in] */ LPCWSTR gatewayResourceName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeleteGatewayResource( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricGatewayResourceManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricGatewayResourceManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricGatewayResourceManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricGatewayResourceManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCreateOrUpdateGatewayResource )( 
            IFabricGatewayResourceManager * This,
            /* [in] */ LPCWSTR gatewayResourceDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCreateOrUpdateGatewayResource )( 
            IFabricGatewayResourceManager * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetGatewayResourceList )( 
            IFabricGatewayResourceManager * This,
            /* [in] */ LPCWSTR gatewayResourceName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetGatewayResourceList )( 
            IFabricGatewayResourceManager * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteGatewayResource )( 
            IFabricGatewayResourceManager * This,
            /* [in] */ LPCWSTR gatewayResourceName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteGatewayResource )( 
            IFabricGatewayResourceManager * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricGatewayResourceManagerVtbl;

    interface IFabricGatewayResourceManager
    {
        CONST_VTBL struct IFabricGatewayResourceManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricGatewayResourceManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricGatewayResourceManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricGatewayResourceManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricGatewayResourceManager_BeginCreateOrUpdateGatewayResource(This,gatewayResourceDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginCreateOrUpdateGatewayResource(This,gatewayResourceDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricGatewayResourceManager_EndCreateOrUpdateGatewayResource(This,context,result)	\
    ( (This)->lpVtbl -> EndCreateOrUpdateGatewayResource(This,context,result) ) 

#define IFabricGatewayResourceManager_BeginGetGatewayResourceList(This,gatewayResourceName,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetGatewayResourceList(This,gatewayResourceName,timeoutMilliseconds,callback,context) ) 

#define IFabricGatewayResourceManager_EndGetGatewayResourceList(This,context,result)	\
    ( (This)->lpVtbl -> EndGetGatewayResourceList(This,context,result) ) 

#define IFabricGatewayResourceManager_BeginDeleteGatewayResource(This,gatewayResourceName,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteGatewayResource(This,gatewayResourceName,timeoutMilliseconds,callback,context) ) 

#define IFabricGatewayResourceManager_EndDeleteGatewayResource(This,context)	\
    ( (This)->lpVtbl -> EndDeleteGatewayResource(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricGatewayResourceManager_INTERFACE_DEFINED__ */


#ifndef __IFabricGatewayResourceManagerAgent_INTERFACE_DEFINED__
#define __IFabricGatewayResourceManagerAgent_INTERFACE_DEFINED__

/* interface IFabricGatewayResourceManagerAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricGatewayResourceManagerAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("029614d9-928a-489d-8eaf-c09f44028f46")
    IFabricGatewayResourceManagerAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterGatewayResourceManager( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricGatewayResourceManagerAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricGatewayResourceManagerAgent0001,
            /* [in] */ IFabricGatewayResourceManager *__MIDL__IFabricGatewayResourceManagerAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterGatewayResourceManager( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricGatewayResourceManagerAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricGatewayResourceManagerAgent0004) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricGatewayResourceManagerAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricGatewayResourceManagerAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricGatewayResourceManagerAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricGatewayResourceManagerAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterGatewayResourceManager )( 
            IFabricGatewayResourceManagerAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricGatewayResourceManagerAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricGatewayResourceManagerAgent0001,
            /* [in] */ IFabricGatewayResourceManager *__MIDL__IFabricGatewayResourceManagerAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterGatewayResourceManager )( 
            IFabricGatewayResourceManagerAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricGatewayResourceManagerAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricGatewayResourceManagerAgent0004);
        
        END_INTERFACE
    } IFabricGatewayResourceManagerAgentVtbl;

    interface IFabricGatewayResourceManagerAgent
    {
        CONST_VTBL struct IFabricGatewayResourceManagerAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricGatewayResourceManagerAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricGatewayResourceManagerAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricGatewayResourceManagerAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricGatewayResourceManagerAgent_RegisterGatewayResourceManager(This,__MIDL__IFabricGatewayResourceManagerAgent0000,__MIDL__IFabricGatewayResourceManagerAgent0001,__MIDL__IFabricGatewayResourceManagerAgent0002,serviceAddress)	\
    ( (This)->lpVtbl -> RegisterGatewayResourceManager(This,__MIDL__IFabricGatewayResourceManagerAgent0000,__MIDL__IFabricGatewayResourceManagerAgent0001,__MIDL__IFabricGatewayResourceManagerAgent0002,serviceAddress) ) 

#define IFabricGatewayResourceManagerAgent_UnregisterGatewayResourceManager(This,__MIDL__IFabricGatewayResourceManagerAgent0003,__MIDL__IFabricGatewayResourceManagerAgent0004)	\
    ( (This)->lpVtbl -> UnregisterGatewayResourceManager(This,__MIDL__IFabricGatewayResourceManagerAgent0003,__MIDL__IFabricGatewayResourceManagerAgent0004) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricGatewayResourceManagerAgent_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricGatewayResourceManagerAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("2407ced2-3530-4944-915e-d56e9392cd5b")
FabricGatewayResourceManagerAgent;
#endif


#ifndef __FabricGatewayResourceManager_MODULE_DEFINED__
#define __FabricGatewayResourceManager_MODULE_DEFINED__


/* module FabricGatewayResourceManager */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricGatewayResourceManagerAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **FabricGatewayResourceManagerAgent);

#endif /* __FabricGatewayResourceManager_MODULE_DEFINED__ */
#endif /* __FabricGateResourceManagerLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


