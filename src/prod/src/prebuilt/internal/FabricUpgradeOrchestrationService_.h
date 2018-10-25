

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

#ifndef __FabricUpgradeOrchestrationService__h__
#define __FabricUpgradeOrchestrationService__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricUpgradeOrchestrationService_FWD_DEFINED__
#define __IFabricUpgradeOrchestrationService_FWD_DEFINED__
typedef interface IFabricUpgradeOrchestrationService IFabricUpgradeOrchestrationService;

#endif 	/* __IFabricUpgradeOrchestrationService_FWD_DEFINED__ */


#ifndef __IFabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__
#define __IFabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__
typedef interface IFabricUpgradeOrchestrationServiceAgent IFabricUpgradeOrchestrationServiceAgent;

#endif 	/* __IFabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__ */


#ifndef __FabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__
#define __FabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricUpgradeOrchestrationServiceAgent FabricUpgradeOrchestrationServiceAgent;
#else
typedef struct FabricUpgradeOrchestrationServiceAgent FabricUpgradeOrchestrationServiceAgent;
#endif /* __cplusplus */

#endif 	/* __FabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricUpgradeOrchestrationService_FWD_DEFINED__
#define __IFabricUpgradeOrchestrationService_FWD_DEFINED__
typedef interface IFabricUpgradeOrchestrationService IFabricUpgradeOrchestrationService;

#endif 	/* __IFabricUpgradeOrchestrationService_FWD_DEFINED__ */


#ifndef __IFabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__
#define __IFabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__
typedef interface IFabricUpgradeOrchestrationServiceAgent IFabricUpgradeOrchestrationServiceAgent;

#endif 	/* __IFabricUpgradeOrchestrationServiceAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"
#include "FabricRuntime.h"
#include "FabricClient_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_FabricUpgradeOrchestrationService__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif




extern RPC_IF_HANDLE __MIDL_itf_FabricUpgradeOrchestrationService__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_FabricUpgradeOrchestrationService__0000_0000_v0_0_s_ifspec;


#ifndef __FabricUpgradeOrchestrationServiceLib_LIBRARY_DEFINED__
#define __FabricUpgradeOrchestrationServiceLib_LIBRARY_DEFINED__

/* library FabricUpgradeOrchestrationServiceLib */
/* [version][uuid] */ 


#pragma pack(push, 8)



#pragma pack(pop)

EXTERN_C const IID LIBID_FabricUpgradeOrchestrationServiceLib;

#ifndef __IFabricUpgradeOrchestrationService_INTERFACE_DEFINED__
#define __IFabricUpgradeOrchestrationService_INTERFACE_DEFINED__

/* interface IFabricUpgradeOrchestrationService */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricUpgradeOrchestrationService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("41695A06-EC43-4F96-A785-A38872FC39BC")
    IFabricUpgradeOrchestrationService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginUpgradeConfiguration( 
            /* [in] */ FABRIC_START_UPGRADE_DESCRIPTION *startUpgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpgradeConfiguration( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetClusterConfigurationUpgradeStatus( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetClusterConfigurationUpgradeStatus( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricOrchestrationUpgradeStatusResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetClusterConfiguration( 
            /* [in] */ LPCWSTR apiVersion,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetClusterConfiguration( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetUpgradesPendingApproval( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetUpgradesPendingApproval( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStartApprovedUpgrades( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartApprovedUpgrades( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCallSystemService( 
            /* [in] */ LPCWSTR action,
            /* [in] */ LPCWSTR inputBlob,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCallSystemService( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **outputBlob) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricUpgradeOrchestrationServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricUpgradeOrchestrationService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricUpgradeOrchestrationService * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpgradeConfiguration )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ FABRIC_START_UPGRADE_DESCRIPTION *startUpgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpgradeConfiguration )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetClusterConfigurationUpgradeStatus )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetClusterConfigurationUpgradeStatus )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricOrchestrationUpgradeStatusResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetClusterConfiguration )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ LPCWSTR apiVersion,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetClusterConfiguration )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetUpgradesPendingApproval )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetUpgradesPendingApproval )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartApprovedUpgrades )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartApprovedUpgrades )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCallSystemService )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ LPCWSTR action,
            /* [in] */ LPCWSTR inputBlob,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCallSystemService )( 
            IFabricUpgradeOrchestrationService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **outputBlob);
        
        END_INTERFACE
    } IFabricUpgradeOrchestrationServiceVtbl;

    interface IFabricUpgradeOrchestrationService
    {
        CONST_VTBL struct IFabricUpgradeOrchestrationServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricUpgradeOrchestrationService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricUpgradeOrchestrationService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricUpgradeOrchestrationService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricUpgradeOrchestrationService_BeginUpgradeConfiguration(This,startUpgradeDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginUpgradeConfiguration(This,startUpgradeDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricUpgradeOrchestrationService_EndUpgradeConfiguration(This,context)	\
    ( (This)->lpVtbl -> EndUpgradeConfiguration(This,context) ) 

#define IFabricUpgradeOrchestrationService_BeginGetClusterConfigurationUpgradeStatus(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetClusterConfigurationUpgradeStatus(This,timeoutMilliseconds,callback,context) ) 

#define IFabricUpgradeOrchestrationService_EndGetClusterConfigurationUpgradeStatus(This,context,result)	\
    ( (This)->lpVtbl -> EndGetClusterConfigurationUpgradeStatus(This,context,result) ) 

#define IFabricUpgradeOrchestrationService_BeginGetClusterConfiguration(This,apiVersion,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetClusterConfiguration(This,apiVersion,timeoutMilliseconds,callback,context) ) 

#define IFabricUpgradeOrchestrationService_EndGetClusterConfiguration(This,context,result)	\
    ( (This)->lpVtbl -> EndGetClusterConfiguration(This,context,result) ) 

#define IFabricUpgradeOrchestrationService_BeginGetUpgradesPendingApproval(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetUpgradesPendingApproval(This,timeoutMilliseconds,callback,context) ) 

#define IFabricUpgradeOrchestrationService_EndGetUpgradesPendingApproval(This,context)	\
    ( (This)->lpVtbl -> EndGetUpgradesPendingApproval(This,context) ) 

#define IFabricUpgradeOrchestrationService_BeginStartApprovedUpgrades(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartApprovedUpgrades(This,timeoutMilliseconds,callback,context) ) 

#define IFabricUpgradeOrchestrationService_EndStartApprovedUpgrades(This,context)	\
    ( (This)->lpVtbl -> EndStartApprovedUpgrades(This,context) ) 

#define IFabricUpgradeOrchestrationService_BeginCallSystemService(This,action,inputBlob,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginCallSystemService(This,action,inputBlob,timeoutMilliseconds,callback,context) ) 

#define IFabricUpgradeOrchestrationService_EndCallSystemService(This,context,outputBlob)	\
    ( (This)->lpVtbl -> EndCallSystemService(This,context,outputBlob) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricUpgradeOrchestrationService_INTERFACE_DEFINED__ */


#ifndef __IFabricUpgradeOrchestrationServiceAgent_INTERFACE_DEFINED__
#define __IFabricUpgradeOrchestrationServiceAgent_INTERFACE_DEFINED__

/* interface IFabricUpgradeOrchestrationServiceAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricUpgradeOrchestrationServiceAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F035ECFB-00F7-4316-9645-ED46D7E1D725")
    IFabricUpgradeOrchestrationServiceAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterUpgradeOrchestrationService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0001,
            /* [in] */ IFabricUpgradeOrchestrationService *__MIDL__IFabricUpgradeOrchestrationServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterUpgradeOrchestrationService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0004) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricUpgradeOrchestrationServiceAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricUpgradeOrchestrationServiceAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricUpgradeOrchestrationServiceAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricUpgradeOrchestrationServiceAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterUpgradeOrchestrationService )( 
            IFabricUpgradeOrchestrationServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0001,
            /* [in] */ IFabricUpgradeOrchestrationService *__MIDL__IFabricUpgradeOrchestrationServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterUpgradeOrchestrationService )( 
            IFabricUpgradeOrchestrationServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricUpgradeOrchestrationServiceAgent0004);
        
        END_INTERFACE
    } IFabricUpgradeOrchestrationServiceAgentVtbl;

    interface IFabricUpgradeOrchestrationServiceAgent
    {
        CONST_VTBL struct IFabricUpgradeOrchestrationServiceAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricUpgradeOrchestrationServiceAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricUpgradeOrchestrationServiceAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricUpgradeOrchestrationServiceAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricUpgradeOrchestrationServiceAgent_RegisterUpgradeOrchestrationService(This,__MIDL__IFabricUpgradeOrchestrationServiceAgent0000,__MIDL__IFabricUpgradeOrchestrationServiceAgent0001,__MIDL__IFabricUpgradeOrchestrationServiceAgent0002,serviceAddress)	\
    ( (This)->lpVtbl -> RegisterUpgradeOrchestrationService(This,__MIDL__IFabricUpgradeOrchestrationServiceAgent0000,__MIDL__IFabricUpgradeOrchestrationServiceAgent0001,__MIDL__IFabricUpgradeOrchestrationServiceAgent0002,serviceAddress) ) 

#define IFabricUpgradeOrchestrationServiceAgent_UnregisterUpgradeOrchestrationService(This,__MIDL__IFabricUpgradeOrchestrationServiceAgent0003,__MIDL__IFabricUpgradeOrchestrationServiceAgent0004)	\
    ( (This)->lpVtbl -> UnregisterUpgradeOrchestrationService(This,__MIDL__IFabricUpgradeOrchestrationServiceAgent0003,__MIDL__IFabricUpgradeOrchestrationServiceAgent0004) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricUpgradeOrchestrationServiceAgent_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricUpgradeOrchestrationServiceAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("7F05BCD1-C206-485D-A4A4-175FBB587A62")
FabricUpgradeOrchestrationServiceAgent;
#endif


#ifndef __FabricUpgradeOrchestrationService_MODULE_DEFINED__
#define __FabricUpgradeOrchestrationService_MODULE_DEFINED__


/* module FabricUpgradeOrchestrationService */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricUpgradeOrchestrationServiceAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricUpgradeOrchestrationServiceAgent);

#endif /* __FabricUpgradeOrchestrationService_MODULE_DEFINED__ */
#endif /* __FabricUpgradeOrchestrationServiceLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


