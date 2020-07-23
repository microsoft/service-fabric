

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

#ifndef __fabricclient__h__
#define __fabricclient__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IInternalFabricServiceManagementClient_FWD_DEFINED__
#define __IInternalFabricServiceManagementClient_FWD_DEFINED__
typedef interface IInternalFabricServiceManagementClient IInternalFabricServiceManagementClient;

#endif 	/* __IInternalFabricServiceManagementClient_FWD_DEFINED__ */


#ifndef __IInternalFabricServiceManagementClient2_FWD_DEFINED__
#define __IInternalFabricServiceManagementClient2_FWD_DEFINED__
typedef interface IInternalFabricServiceManagementClient2 IInternalFabricServiceManagementClient2;

#endif 	/* __IInternalFabricServiceManagementClient2_FWD_DEFINED__ */


#ifndef __IInternalFabricApplicationManagementClient_FWD_DEFINED__
#define __IInternalFabricApplicationManagementClient_FWD_DEFINED__
typedef interface IInternalFabricApplicationManagementClient IInternalFabricApplicationManagementClient;

#endif 	/* __IInternalFabricApplicationManagementClient_FWD_DEFINED__ */


#ifndef __IInternalFabricApplicationManagementClient2_FWD_DEFINED__
#define __IInternalFabricApplicationManagementClient2_FWD_DEFINED__
typedef interface IInternalFabricApplicationManagementClient2 IInternalFabricApplicationManagementClient2;

#endif 	/* __IInternalFabricApplicationManagementClient2_FWD_DEFINED__ */


#ifndef __IInternalFabricClusterManagementClient_FWD_DEFINED__
#define __IInternalFabricClusterManagementClient_FWD_DEFINED__
typedef interface IInternalFabricClusterManagementClient IInternalFabricClusterManagementClient;

#endif 	/* __IInternalFabricClusterManagementClient_FWD_DEFINED__ */


#ifndef __IInternalFabricResolvedServicePartition_FWD_DEFINED__
#define __IInternalFabricResolvedServicePartition_FWD_DEFINED__
typedef interface IInternalFabricResolvedServicePartition IInternalFabricResolvedServicePartition;

#endif 	/* __IInternalFabricResolvedServicePartition_FWD_DEFINED__ */


#ifndef __IInternalFabricInfrastructureServiceClient_FWD_DEFINED__
#define __IInternalFabricInfrastructureServiceClient_FWD_DEFINED__
typedef interface IInternalFabricInfrastructureServiceClient IInternalFabricInfrastructureServiceClient;

#endif 	/* __IInternalFabricInfrastructureServiceClient_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryClient_FWD_DEFINED__
#define __IInternalFabricQueryClient_FWD_DEFINED__
typedef interface IInternalFabricQueryClient IInternalFabricQueryClient;

#endif 	/* __IInternalFabricQueryClient_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryClient2_FWD_DEFINED__
#define __IInternalFabricQueryClient2_FWD_DEFINED__
typedef interface IInternalFabricQueryClient2 IInternalFabricQueryClient2;

#endif 	/* __IInternalFabricQueryClient2_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryResult_FWD_DEFINED__
#define __IInternalFabricQueryResult_FWD_DEFINED__
typedef interface IInternalFabricQueryResult IInternalFabricQueryResult;

#endif 	/* __IInternalFabricQueryResult_FWD_DEFINED__ */


#ifndef __IFabricAccessControlClient_FWD_DEFINED__
#define __IFabricAccessControlClient_FWD_DEFINED__
typedef interface IFabricAccessControlClient IFabricAccessControlClient;

#endif 	/* __IFabricAccessControlClient_FWD_DEFINED__ */


#ifndef __IFabricGetAclResult_FWD_DEFINED__
#define __IFabricGetAclResult_FWD_DEFINED__
typedef interface IFabricGetAclResult IFabricGetAclResult;

#endif 	/* __IFabricGetAclResult_FWD_DEFINED__ */


#ifndef __IFabricImageStoreClient_FWD_DEFINED__
#define __IFabricImageStoreClient_FWD_DEFINED__
typedef interface IFabricImageStoreClient IFabricImageStoreClient;

#endif 	/* __IFabricImageStoreClient_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryResult2_FWD_DEFINED__
#define __IInternalFabricQueryResult2_FWD_DEFINED__
typedef interface IInternalFabricQueryResult2 IInternalFabricQueryResult2;

#endif 	/* __IInternalFabricQueryResult2_FWD_DEFINED__ */


#ifndef __IInternalFabricServiceManagementClient_FWD_DEFINED__
#define __IInternalFabricServiceManagementClient_FWD_DEFINED__
typedef interface IInternalFabricServiceManagementClient IInternalFabricServiceManagementClient;

#endif 	/* __IInternalFabricServiceManagementClient_FWD_DEFINED__ */


#ifndef __IInternalFabricServiceManagementClient2_FWD_DEFINED__
#define __IInternalFabricServiceManagementClient2_FWD_DEFINED__
typedef interface IInternalFabricServiceManagementClient2 IInternalFabricServiceManagementClient2;

#endif 	/* __IInternalFabricServiceManagementClient2_FWD_DEFINED__ */


#ifndef __IInternalFabricApplicationManagementClient_FWD_DEFINED__
#define __IInternalFabricApplicationManagementClient_FWD_DEFINED__
typedef interface IInternalFabricApplicationManagementClient IInternalFabricApplicationManagementClient;

#endif 	/* __IInternalFabricApplicationManagementClient_FWD_DEFINED__ */


#ifndef __IInternalFabricApplicationManagementClient2_FWD_DEFINED__
#define __IInternalFabricApplicationManagementClient2_FWD_DEFINED__
typedef interface IInternalFabricApplicationManagementClient2 IInternalFabricApplicationManagementClient2;

#endif 	/* __IInternalFabricApplicationManagementClient2_FWD_DEFINED__ */


#ifndef __IInternalFabricClusterManagementClient_FWD_DEFINED__
#define __IInternalFabricClusterManagementClient_FWD_DEFINED__
typedef interface IInternalFabricClusterManagementClient IInternalFabricClusterManagementClient;

#endif 	/* __IInternalFabricClusterManagementClient_FWD_DEFINED__ */


#ifndef __IInternalFabricResolvedServicePartition_FWD_DEFINED__
#define __IInternalFabricResolvedServicePartition_FWD_DEFINED__
typedef interface IInternalFabricResolvedServicePartition IInternalFabricResolvedServicePartition;

#endif 	/* __IInternalFabricResolvedServicePartition_FWD_DEFINED__ */


#ifndef __IInternalFabricInfrastructureServiceClient_FWD_DEFINED__
#define __IInternalFabricInfrastructureServiceClient_FWD_DEFINED__
typedef interface IInternalFabricInfrastructureServiceClient IInternalFabricInfrastructureServiceClient;

#endif 	/* __IInternalFabricInfrastructureServiceClient_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryClient_FWD_DEFINED__
#define __IInternalFabricQueryClient_FWD_DEFINED__
typedef interface IInternalFabricQueryClient IInternalFabricQueryClient;

#endif 	/* __IInternalFabricQueryClient_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryClient2_FWD_DEFINED__
#define __IInternalFabricQueryClient2_FWD_DEFINED__
typedef interface IInternalFabricQueryClient2 IInternalFabricQueryClient2;

#endif 	/* __IInternalFabricQueryClient2_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryResult_FWD_DEFINED__
#define __IInternalFabricQueryResult_FWD_DEFINED__
typedef interface IInternalFabricQueryResult IInternalFabricQueryResult;

#endif 	/* __IInternalFabricQueryResult_FWD_DEFINED__ */


#ifndef __IInternalFabricQueryResult2_FWD_DEFINED__
#define __IInternalFabricQueryResult2_FWD_DEFINED__
typedef interface IInternalFabricQueryResult2 IInternalFabricQueryResult2;

#endif 	/* __IInternalFabricQueryResult2_FWD_DEFINED__ */


#ifndef __IFabricAccessControlClient_FWD_DEFINED__
#define __IFabricAccessControlClient_FWD_DEFINED__
typedef interface IFabricAccessControlClient IFabricAccessControlClient;

#endif 	/* __IFabricAccessControlClient_FWD_DEFINED__ */


#ifndef __IFabricGetAclResult_FWD_DEFINED__
#define __IFabricGetAclResult_FWD_DEFINED__
typedef interface IFabricGetAclResult IFabricGetAclResult;

#endif 	/* __IFabricGetAclResult_FWD_DEFINED__ */


#ifndef __IFabricImageStoreClient_FWD_DEFINED__
#define __IFabricImageStoreClient_FWD_DEFINED__
typedef interface IFabricImageStoreClient IFabricImageStoreClient;

#endif 	/* __IFabricImageStoreClient_FWD_DEFINED__ */


#ifndef __IFabricTestManagementClientInternal_FWD_DEFINED__
#define __IFabricTestManagementClientInternal_FWD_DEFINED__
typedef interface IFabricTestManagementClientInternal IFabricTestManagementClientInternal;

#endif 	/* __IFabricTestManagementClientInternal_FWD_DEFINED__ */


#ifndef __IFabricTestManagementClientInternal2_FWD_DEFINED__
#define __IFabricTestManagementClientInternal2_FWD_DEFINED__
typedef interface IFabricTestManagementClientInternal2 IFabricTestManagementClientInternal2;

#endif 	/* __IFabricTestManagementClientInternal2_FWD_DEFINED__ */


#ifndef __IFabricFaultManagementClientInternal_FWD_DEFINED__
#define __IFabricFaultManagementClientInternal_FWD_DEFINED__
typedef interface IFabricFaultManagementClientInternal IFabricFaultManagementClientInternal;

#endif 	/* __IFabricFaultManagementClientInternal_FWD_DEFINED__ */


#ifndef __IFabricGetComposeDeploymentStatusListResult_FWD_DEFINED__
#define __IFabricGetComposeDeploymentStatusListResult_FWD_DEFINED__
typedef interface IFabricGetComposeDeploymentStatusListResult IFabricGetComposeDeploymentStatusListResult;

#endif 	/* __IFabricGetComposeDeploymentStatusListResult_FWD_DEFINED__ */


#ifndef __IFabricComposeDeploymentUpgradeProgressResult_FWD_DEFINED__
#define __IFabricComposeDeploymentUpgradeProgressResult_FWD_DEFINED__
typedef interface IFabricComposeDeploymentUpgradeProgressResult IFabricComposeDeploymentUpgradeProgressResult;

#endif 	/* __IFabricComposeDeploymentUpgradeProgressResult_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricClient.h"
#include "FabricTypes_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricclient__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif
















extern RPC_IF_HANDLE __MIDL_itf_fabricclient__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricclient__0000_0000_v0_0_s_ifspec;


#ifndef __FabricClient_Lib_LIBRARY_DEFINED__
#define __FabricClient_Lib_LIBRARY_DEFINED__

/* library FabricClient_Lib */
/* [version][uuid] */ 


#pragma pack(push, 8)















#pragma pack(pop)

EXTERN_C const IID LIBID_FabricClient_Lib;

#ifndef __IInternalFabricServiceManagementClient_INTERFACE_DEFINED__
#define __IInternalFabricServiceManagementClient_INTERFACE_DEFINED__

/* interface IInternalFabricServiceManagementClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricServiceManagementClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1185854b-9d32-4c78-b1f2-1fae7ae4c002")
    IInternalFabricServiceManagementClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginMovePrimary( 
            /* [in] */ const FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION *movePrimaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndMovePrimary( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginMoveSecondary( 
            /* [in] */ const FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION *moveSecondaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndMoveSecondary( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricServiceManagementClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricServiceManagementClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricServiceManagementClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricServiceManagementClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginMovePrimary )( 
            IInternalFabricServiceManagementClient * This,
            /* [in] */ const FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION *movePrimaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndMovePrimary )( 
            IInternalFabricServiceManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginMoveSecondary )( 
            IInternalFabricServiceManagementClient * This,
            /* [in] */ const FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION *moveSecondaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndMoveSecondary )( 
            IInternalFabricServiceManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IInternalFabricServiceManagementClientVtbl;

    interface IInternalFabricServiceManagementClient
    {
        CONST_VTBL struct IInternalFabricServiceManagementClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricServiceManagementClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricServiceManagementClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricServiceManagementClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricServiceManagementClient_BeginMovePrimary(This,movePrimaryDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginMovePrimary(This,movePrimaryDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricServiceManagementClient_EndMovePrimary(This,context)	\
    ( (This)->lpVtbl -> EndMovePrimary(This,context) ) 

#define IInternalFabricServiceManagementClient_BeginMoveSecondary(This,moveSecondaryDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginMoveSecondary(This,moveSecondaryDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricServiceManagementClient_EndMoveSecondary(This,context)	\
    ( (This)->lpVtbl -> EndMoveSecondary(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricServiceManagementClient_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricServiceManagementClient2_INTERFACE_DEFINED__
#define __IInternalFabricServiceManagementClient2_INTERFACE_DEFINED__

/* interface IInternalFabricServiceManagementClient2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricServiceManagementClient2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ee2bc5c7-235c-4dd1-b0a9-2ed4b45ebec9")
    IInternalFabricServiceManagementClient2 : public IInternalFabricServiceManagementClient
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginGetCachedServiceDescription( 
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetCachedServiceDescription( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceDescriptionResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricServiceManagementClient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricServiceManagementClient2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricServiceManagementClient2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricServiceManagementClient2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginMovePrimary )( 
            IInternalFabricServiceManagementClient2 * This,
            /* [in] */ const FABRIC_MOVE_PRIMARY_REPLICA_DESCRIPTION *movePrimaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndMovePrimary )( 
            IInternalFabricServiceManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginMoveSecondary )( 
            IInternalFabricServiceManagementClient2 * This,
            /* [in] */ const FABRIC_MOVE_SECONDARY_REPLICA_DESCRIPTION *moveSecondaryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndMoveSecondary )( 
            IInternalFabricServiceManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetCachedServiceDescription )( 
            IInternalFabricServiceManagementClient2 * This,
            /* [in] */ FABRIC_URI name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetCachedServiceDescription )( 
            IInternalFabricServiceManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceDescriptionResult **result);
        
        END_INTERFACE
    } IInternalFabricServiceManagementClient2Vtbl;

    interface IInternalFabricServiceManagementClient2
    {
        CONST_VTBL struct IInternalFabricServiceManagementClient2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricServiceManagementClient2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricServiceManagementClient2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricServiceManagementClient2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricServiceManagementClient2_BeginMovePrimary(This,movePrimaryDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginMovePrimary(This,movePrimaryDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricServiceManagementClient2_EndMovePrimary(This,context)	\
    ( (This)->lpVtbl -> EndMovePrimary(This,context) ) 

#define IInternalFabricServiceManagementClient2_BeginMoveSecondary(This,moveSecondaryDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginMoveSecondary(This,moveSecondaryDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricServiceManagementClient2_EndMoveSecondary(This,context)	\
    ( (This)->lpVtbl -> EndMoveSecondary(This,context) ) 


#define IInternalFabricServiceManagementClient2_BeginGetCachedServiceDescription(This,name,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetCachedServiceDescription(This,name,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricServiceManagementClient2_EndGetCachedServiceDescription(This,context,result)	\
    ( (This)->lpVtbl -> EndGetCachedServiceDescription(This,context,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricServiceManagementClient2_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricApplicationManagementClient_INTERFACE_DEFINED__
#define __IInternalFabricApplicationManagementClient_INTERFACE_DEFINED__

/* interface IInternalFabricApplicationManagementClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricApplicationManagementClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d2db94b8-7725-42a4-b9f0-7d677a3fac18")
    IInternalFabricApplicationManagementClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRestartDeployedCodePackageInternal( 
            /* [in] */ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION *restartCodePackageDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRestartDeployedCodePackageInternal( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCreateComposeDeployment( 
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION *applicationDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCreateComposeDeployment( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetComposeDeploymentStatusList( 
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetComposeDeploymentStatusList( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetComposeDeploymentStatusListResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeleteComposeDeployment( 
            /* [in] */ const FABRIC_DELETE_COMPOSE_DEPLOYMENT_DESCRIPTION *deleteDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeleteComposeDeployment( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUpgradeComposeDeployment( 
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION *upgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpgradeComposeDeployment( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetComposeDeploymentUpgradeProgress( 
            /* [in] */ LPCWSTR deploymentName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetComposeDeploymentUpgradeProgress( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricComposeDeploymentUpgradeProgressResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricApplicationManagementClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricApplicationManagementClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricApplicationManagementClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRestartDeployedCodePackageInternal )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION *restartCodePackageDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRestartDeployedCodePackageInternal )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCreateComposeDeployment )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION *applicationDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCreateComposeDeployment )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetComposeDeploymentStatusList )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetComposeDeploymentStatusList )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetComposeDeploymentStatusListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteComposeDeployment )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ const FABRIC_DELETE_COMPOSE_DEPLOYMENT_DESCRIPTION *deleteDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteComposeDeployment )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpgradeComposeDeployment )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION *upgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpgradeComposeDeployment )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetComposeDeploymentUpgradeProgress )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ LPCWSTR deploymentName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetComposeDeploymentUpgradeProgress )( 
            IInternalFabricApplicationManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricComposeDeploymentUpgradeProgressResult **result);
        
        END_INTERFACE
    } IInternalFabricApplicationManagementClientVtbl;

    interface IInternalFabricApplicationManagementClient
    {
        CONST_VTBL struct IInternalFabricApplicationManagementClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricApplicationManagementClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricApplicationManagementClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricApplicationManagementClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricApplicationManagementClient_BeginRestartDeployedCodePackageInternal(This,restartCodePackageDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRestartDeployedCodePackageInternal(This,restartCodePackageDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient_EndRestartDeployedCodePackageInternal(This,context)	\
    ( (This)->lpVtbl -> EndRestartDeployedCodePackageInternal(This,context) ) 

#define IInternalFabricApplicationManagementClient_BeginCreateComposeDeployment(This,applicationDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginCreateComposeDeployment(This,applicationDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient_EndCreateComposeDeployment(This,context)	\
    ( (This)->lpVtbl -> EndCreateComposeDeployment(This,context) ) 

#define IInternalFabricApplicationManagementClient_BeginGetComposeDeploymentStatusList(This,queryDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetComposeDeploymentStatusList(This,queryDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient_EndGetComposeDeploymentStatusList(This,context,result)	\
    ( (This)->lpVtbl -> EndGetComposeDeploymentStatusList(This,context,result) ) 

#define IInternalFabricApplicationManagementClient_BeginDeleteComposeDeployment(This,deleteDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteComposeDeployment(This,deleteDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient_EndDeleteComposeDeployment(This,context)	\
    ( (This)->lpVtbl -> EndDeleteComposeDeployment(This,context) ) 

#define IInternalFabricApplicationManagementClient_BeginUpgradeComposeDeployment(This,upgradeDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginUpgradeComposeDeployment(This,upgradeDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient_EndUpgradeComposeDeployment(This,context)	\
    ( (This)->lpVtbl -> EndUpgradeComposeDeployment(This,context) ) 

#define IInternalFabricApplicationManagementClient_BeginGetComposeDeploymentUpgradeProgress(This,deploymentName,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetComposeDeploymentUpgradeProgress(This,deploymentName,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient_EndGetComposeDeploymentUpgradeProgress(This,context,result)	\
    ( (This)->lpVtbl -> EndGetComposeDeploymentUpgradeProgress(This,context,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricApplicationManagementClient_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricApplicationManagementClient2_INTERFACE_DEFINED__
#define __IInternalFabricApplicationManagementClient2_INTERFACE_DEFINED__

/* interface IInternalFabricApplicationManagementClient2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricApplicationManagementClient2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("94a1bed3-445a-41ae-b1b4-6a9ba4be71e5")
    IInternalFabricApplicationManagementClient2 : public IInternalFabricApplicationManagementClient
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRollbackComposeDeployment( 
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_ROLLBACK_DESCRIPTION *rollbackDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRollbackComposeDeployment( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricApplicationManagementClient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricApplicationManagementClient2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricApplicationManagementClient2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRestartDeployedCodePackageInternal )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ const FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION *restartCodePackageDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRestartDeployedCodePackageInternal )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCreateComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION *applicationDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCreateComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetComposeDeploymentStatusList )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetComposeDeploymentStatusList )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetComposeDeploymentStatusListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ const FABRIC_DELETE_COMPOSE_DEPLOYMENT_DESCRIPTION *deleteDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpgradeComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION *upgradeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpgradeComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetComposeDeploymentUpgradeProgress )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ LPCWSTR deploymentName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetComposeDeploymentUpgradeProgress )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricComposeDeploymentUpgradeProgressResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRollbackComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ const FABRIC_COMPOSE_DEPLOYMENT_ROLLBACK_DESCRIPTION *rollbackDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRollbackComposeDeployment )( 
            IInternalFabricApplicationManagementClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IInternalFabricApplicationManagementClient2Vtbl;

    interface IInternalFabricApplicationManagementClient2
    {
        CONST_VTBL struct IInternalFabricApplicationManagementClient2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricApplicationManagementClient2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricApplicationManagementClient2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricApplicationManagementClient2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricApplicationManagementClient2_BeginRestartDeployedCodePackageInternal(This,restartCodePackageDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRestartDeployedCodePackageInternal(This,restartCodePackageDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient2_EndRestartDeployedCodePackageInternal(This,context)	\
    ( (This)->lpVtbl -> EndRestartDeployedCodePackageInternal(This,context) ) 

#define IInternalFabricApplicationManagementClient2_BeginCreateComposeDeployment(This,applicationDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginCreateComposeDeployment(This,applicationDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient2_EndCreateComposeDeployment(This,context)	\
    ( (This)->lpVtbl -> EndCreateComposeDeployment(This,context) ) 

#define IInternalFabricApplicationManagementClient2_BeginGetComposeDeploymentStatusList(This,queryDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetComposeDeploymentStatusList(This,queryDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient2_EndGetComposeDeploymentStatusList(This,context,result)	\
    ( (This)->lpVtbl -> EndGetComposeDeploymentStatusList(This,context,result) ) 

#define IInternalFabricApplicationManagementClient2_BeginDeleteComposeDeployment(This,deleteDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteComposeDeployment(This,deleteDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient2_EndDeleteComposeDeployment(This,context)	\
    ( (This)->lpVtbl -> EndDeleteComposeDeployment(This,context) ) 

#define IInternalFabricApplicationManagementClient2_BeginUpgradeComposeDeployment(This,upgradeDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginUpgradeComposeDeployment(This,upgradeDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient2_EndUpgradeComposeDeployment(This,context)	\
    ( (This)->lpVtbl -> EndUpgradeComposeDeployment(This,context) ) 

#define IInternalFabricApplicationManagementClient2_BeginGetComposeDeploymentUpgradeProgress(This,deploymentName,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetComposeDeploymentUpgradeProgress(This,deploymentName,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient2_EndGetComposeDeploymentUpgradeProgress(This,context,result)	\
    ( (This)->lpVtbl -> EndGetComposeDeploymentUpgradeProgress(This,context,result) ) 


#define IInternalFabricApplicationManagementClient2_BeginRollbackComposeDeployment(This,rollbackDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRollbackComposeDeployment(This,rollbackDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricApplicationManagementClient2_EndRollbackComposeDeployment(This,context)	\
    ( (This)->lpVtbl -> EndRollbackComposeDeployment(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricApplicationManagementClient2_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricClusterManagementClient_INTERFACE_DEFINED__
#define __IInternalFabricClusterManagementClient_INTERFACE_DEFINED__

/* interface IInternalFabricClusterManagementClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricClusterManagementClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28ab258c-c5d3-4306-bf2d-fc00dd40dddd")
    IInternalFabricClusterManagementClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginStartInfrastructureTask( 
            /* [in] */ const FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION *__MIDL__IInternalFabricClusterManagementClient0000,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartInfrastructureTask( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isComplete) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginFinishInfrastructureTask( 
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndFinishInfrastructureTask( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isComplete) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAddUnreliableTransportBehavior( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR name,
            /* [in] */ const FABRIC_UNRELIABLETRANSPORT_BEHAVIOR *unreliableTransportBehavior,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAddUnreliableTransportBehavior( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRemoveUnreliableTransportBehavior( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRemoveUnreliableTransportBehavior( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetTransportBehaviorList( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetTransportBehaviorList( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringListResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRestartNodeInternal( 
            /* [in] */ const FABRIC_RESTART_NODE_DESCRIPTION *restartNodeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRestartNodeInternal( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStartNodeInternal( 
            /* [in] */ const FABRIC_START_NODE_DESCRIPTION *startNodeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartNodeInternal( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricClusterManagementClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricClusterManagementClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricClusterManagementClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartInfrastructureTask )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ const FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION *__MIDL__IInternalFabricClusterManagementClient0000,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartInfrastructureTask )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isComplete);
        
        HRESULT ( STDMETHODCALLTYPE *BeginFinishInfrastructureTask )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndFinishInfrastructureTask )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isComplete);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAddUnreliableTransportBehavior )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR name,
            /* [in] */ const FABRIC_UNRELIABLETRANSPORT_BEHAVIOR *unreliableTransportBehavior,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndAddUnreliableTransportBehavior )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRemoveUnreliableTransportBehavior )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR name,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRemoveUnreliableTransportBehavior )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetTransportBehaviorList )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetTransportBehaviorList )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringListResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRestartNodeInternal )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ const FABRIC_RESTART_NODE_DESCRIPTION *restartNodeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRestartNodeInternal )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartNodeInternal )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ const FABRIC_START_NODE_DESCRIPTION *startNodeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartNodeInternal )( 
            IInternalFabricClusterManagementClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IInternalFabricClusterManagementClientVtbl;

    interface IInternalFabricClusterManagementClient
    {
        CONST_VTBL struct IInternalFabricClusterManagementClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricClusterManagementClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricClusterManagementClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricClusterManagementClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricClusterManagementClient_BeginStartInfrastructureTask(This,__MIDL__IInternalFabricClusterManagementClient0000,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartInfrastructureTask(This,__MIDL__IInternalFabricClusterManagementClient0000,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricClusterManagementClient_EndStartInfrastructureTask(This,context,isComplete)	\
    ( (This)->lpVtbl -> EndStartInfrastructureTask(This,context,isComplete) ) 

#define IInternalFabricClusterManagementClient_BeginFinishInfrastructureTask(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginFinishInfrastructureTask(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricClusterManagementClient_EndFinishInfrastructureTask(This,context,isComplete)	\
    ( (This)->lpVtbl -> EndFinishInfrastructureTask(This,context,isComplete) ) 

#define IInternalFabricClusterManagementClient_BeginAddUnreliableTransportBehavior(This,nodeName,name,unreliableTransportBehavior,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginAddUnreliableTransportBehavior(This,nodeName,name,unreliableTransportBehavior,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricClusterManagementClient_EndAddUnreliableTransportBehavior(This,context)	\
    ( (This)->lpVtbl -> EndAddUnreliableTransportBehavior(This,context) ) 

#define IInternalFabricClusterManagementClient_BeginRemoveUnreliableTransportBehavior(This,nodeName,name,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRemoveUnreliableTransportBehavior(This,nodeName,name,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricClusterManagementClient_EndRemoveUnreliableTransportBehavior(This,context)	\
    ( (This)->lpVtbl -> EndRemoveUnreliableTransportBehavior(This,context) ) 

#define IInternalFabricClusterManagementClient_BeginGetTransportBehaviorList(This,nodeName,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetTransportBehaviorList(This,nodeName,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricClusterManagementClient_EndGetTransportBehaviorList(This,context,result)	\
    ( (This)->lpVtbl -> EndGetTransportBehaviorList(This,context,result) ) 

#define IInternalFabricClusterManagementClient_BeginRestartNodeInternal(This,restartNodeDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRestartNodeInternal(This,restartNodeDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricClusterManagementClient_EndRestartNodeInternal(This,context)	\
    ( (This)->lpVtbl -> EndRestartNodeInternal(This,context) ) 

#define IInternalFabricClusterManagementClient_BeginStartNodeInternal(This,startNodeDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartNodeInternal(This,startNodeDescription,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricClusterManagementClient_EndStartNodeInternal(This,context)	\
    ( (This)->lpVtbl -> EndStartNodeInternal(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricClusterManagementClient_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricResolvedServicePartition_INTERFACE_DEFINED__
#define __IInternalFabricResolvedServicePartition_INTERFACE_DEFINED__

/* interface IInternalFabricResolvedServicePartition */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricResolvedServicePartition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d08373e3-b0c7-40c9-83cd-f92de738d72d")
    IInternalFabricResolvedServicePartition : public IUnknown
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FMVersion( 
            /* [out] */ LONGLONG *value) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Generation( 
            /* [out] */ LONGLONG *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricResolvedServicePartitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricResolvedServicePartition * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricResolvedServicePartition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricResolvedServicePartition * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FMVersion )( 
            IInternalFabricResolvedServicePartition * This,
            /* [out] */ LONGLONG *value);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Generation )( 
            IInternalFabricResolvedServicePartition * This,
            /* [out] */ LONGLONG *value);
        
        END_INTERFACE
    } IInternalFabricResolvedServicePartitionVtbl;

    interface IInternalFabricResolvedServicePartition
    {
        CONST_VTBL struct IInternalFabricResolvedServicePartitionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricResolvedServicePartition_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricResolvedServicePartition_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricResolvedServicePartition_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricResolvedServicePartition_get_FMVersion(This,value)	\
    ( (This)->lpVtbl -> get_FMVersion(This,value) ) 

#define IInternalFabricResolvedServicePartition_get_Generation(This,value)	\
    ( (This)->lpVtbl -> get_Generation(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricResolvedServicePartition_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricInfrastructureServiceClient_INTERFACE_DEFINED__
#define __IInternalFabricInfrastructureServiceClient_INTERFACE_DEFINED__

/* interface IInternalFabricInfrastructureServiceClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricInfrastructureServiceClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f0ab3055-03cd-450a-ac45-d4f215fde386")
    IInternalFabricInfrastructureServiceClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRunCommand( 
            /* [in] */ const LPCWSTR command,
            /* [in] */ FABRIC_PARTITION_ID targetPartitionId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRunCommand( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportStartTaskSuccess( 
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportStartTaskSuccess( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportFinishTaskSuccess( 
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportFinishTaskSuccess( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportTaskFailure( 
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportTaskFailure( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricInfrastructureServiceClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricInfrastructureServiceClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricInfrastructureServiceClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRunCommand )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ const LPCWSTR command,
            /* [in] */ FABRIC_PARTITION_ID targetPartitionId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRunCommand )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportStartTaskSuccess )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportStartTaskSuccess )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportFinishTaskSuccess )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportFinishTaskSuccess )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportTaskFailure )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ const LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportTaskFailure )( 
            IInternalFabricInfrastructureServiceClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IInternalFabricInfrastructureServiceClientVtbl;

    interface IInternalFabricInfrastructureServiceClient
    {
        CONST_VTBL struct IInternalFabricInfrastructureServiceClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricInfrastructureServiceClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricInfrastructureServiceClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricInfrastructureServiceClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricInfrastructureServiceClient_BeginRunCommand(This,command,targetPartitionId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRunCommand(This,command,targetPartitionId,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricInfrastructureServiceClient_EndRunCommand(This,context,result)	\
    ( (This)->lpVtbl -> EndRunCommand(This,context,result) ) 

#define IInternalFabricInfrastructureServiceClient_BeginReportStartTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportStartTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricInfrastructureServiceClient_EndReportStartTaskSuccess(This,context)	\
    ( (This)->lpVtbl -> EndReportStartTaskSuccess(This,context) ) 

#define IInternalFabricInfrastructureServiceClient_BeginReportFinishTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportFinishTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricInfrastructureServiceClient_EndReportFinishTaskSuccess(This,context)	\
    ( (This)->lpVtbl -> EndReportFinishTaskSuccess(This,context) ) 

#define IInternalFabricInfrastructureServiceClient_BeginReportTaskFailure(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportTaskFailure(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricInfrastructureServiceClient_EndReportTaskFailure(This,context)	\
    ( (This)->lpVtbl -> EndReportTaskFailure(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricInfrastructureServiceClient_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricQueryClient_INTERFACE_DEFINED__
#define __IInternalFabricQueryClient_INTERFACE_DEFINED__

/* interface IInternalFabricQueryClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricQueryClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c70c3e89-d0cc-44bf-ab2c-8d7cb7a9078a")
    IInternalFabricQueryClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginInternalQuery( 
            /* [in] */ LPCWSTR queryName,
            /* [in] */ const FABRIC_STRING_PAIR_LIST *queryArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndInternalQuery( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IInternalFabricQueryResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricQueryClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricQueryClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricQueryClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricQueryClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginInternalQuery )( 
            IInternalFabricQueryClient * This,
            /* [in] */ LPCWSTR queryName,
            /* [in] */ const FABRIC_STRING_PAIR_LIST *queryArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndInternalQuery )( 
            IInternalFabricQueryClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IInternalFabricQueryResult **result);
        
        END_INTERFACE
    } IInternalFabricQueryClientVtbl;

    interface IInternalFabricQueryClient
    {
        CONST_VTBL struct IInternalFabricQueryClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricQueryClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricQueryClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricQueryClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricQueryClient_BeginInternalQuery(This,queryName,queryArgs,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginInternalQuery(This,queryName,queryArgs,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricQueryClient_EndInternalQuery(This,context,result)	\
    ( (This)->lpVtbl -> EndInternalQuery(This,context,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricQueryClient_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricQueryClient2_INTERFACE_DEFINED__
#define __IInternalFabricQueryClient2_INTERFACE_DEFINED__

/* interface IInternalFabricQueryClient2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricQueryClient2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ad2b44fa-1379-416a-8b7f-45744ef8ee8f")
    IInternalFabricQueryClient2 : public IInternalFabricQueryClient
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EndInternalQuery2( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IInternalFabricQueryResult2 **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricQueryClient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricQueryClient2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricQueryClient2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricQueryClient2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginInternalQuery )( 
            IInternalFabricQueryClient2 * This,
            /* [in] */ LPCWSTR queryName,
            /* [in] */ const FABRIC_STRING_PAIR_LIST *queryArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndInternalQuery )( 
            IInternalFabricQueryClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IInternalFabricQueryResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *EndInternalQuery2 )( 
            IInternalFabricQueryClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IInternalFabricQueryResult2 **result);
        
        END_INTERFACE
    } IInternalFabricQueryClient2Vtbl;

    interface IInternalFabricQueryClient2
    {
        CONST_VTBL struct IInternalFabricQueryClient2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricQueryClient2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricQueryClient2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricQueryClient2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricQueryClient2_BeginInternalQuery(This,queryName,queryArgs,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginInternalQuery(This,queryName,queryArgs,timeoutMilliseconds,callback,context) ) 

#define IInternalFabricQueryClient2_EndInternalQuery(This,context,result)	\
    ( (This)->lpVtbl -> EndInternalQuery(This,context,result) ) 


#define IInternalFabricQueryClient2_EndInternalQuery2(This,context,result)	\
    ( (This)->lpVtbl -> EndInternalQuery2(This,context,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricQueryClient2_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricQueryResult_INTERFACE_DEFINED__
#define __IInternalFabricQueryResult_INTERFACE_DEFINED__

/* interface IInternalFabricQueryResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricQueryResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0589b863-d108-41f0-b178-a5478c76a9ae")
    IInternalFabricQueryResult : public IUnknown
    {
    public:
        virtual const FABRIC_QUERY_RESULT *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricQueryResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricQueryResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricQueryResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricQueryResult * This);
        
        const FABRIC_QUERY_RESULT *( STDMETHODCALLTYPE *get_Result )( 
            IInternalFabricQueryResult * This);
        
        END_INTERFACE
    } IInternalFabricQueryResultVtbl;

    interface IInternalFabricQueryResult
    {
        CONST_VTBL struct IInternalFabricQueryResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricQueryResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricQueryResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricQueryResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricQueryResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricQueryResult_INTERFACE_DEFINED__ */


#ifndef __IFabricAccessControlClient_INTERFACE_DEFINED__
#define __IFabricAccessControlClient_INTERFACE_DEFINED__

/* interface IFabricAccessControlClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricAccessControlClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bb5dd5a2-0ab5-4faa-8510-13129f536bcf")
    IFabricAccessControlClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginSetAcl( 
            /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
            /* [in] */ const FABRIC_SECURITY_ACL *acl,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndSetAcl( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetAcl( 
            /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetAcl( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetAclResult **acl) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricAccessControlClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricAccessControlClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricAccessControlClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricAccessControlClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginSetAcl )( 
            IFabricAccessControlClient * This,
            /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
            /* [in] */ const FABRIC_SECURITY_ACL *acl,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndSetAcl )( 
            IFabricAccessControlClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetAcl )( 
            IFabricAccessControlClient * This,
            /* [in] */ const FABRIC_SECURITY_RESOURCE_IDENTIFIER *resource,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetAcl )( 
            IFabricAccessControlClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetAclResult **acl);
        
        END_INTERFACE
    } IFabricAccessControlClientVtbl;

    interface IFabricAccessControlClient
    {
        CONST_VTBL struct IFabricAccessControlClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricAccessControlClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricAccessControlClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricAccessControlClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricAccessControlClient_BeginSetAcl(This,resource,acl,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginSetAcl(This,resource,acl,timeoutMilliseconds,callback,context) ) 

#define IFabricAccessControlClient_EndSetAcl(This,context)	\
    ( (This)->lpVtbl -> EndSetAcl(This,context) ) 

#define IFabricAccessControlClient_BeginGetAcl(This,resource,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetAcl(This,resource,timeoutMilliseconds,callback,context) ) 

#define IFabricAccessControlClient_EndGetAcl(This,context,acl)	\
    ( (This)->lpVtbl -> EndGetAcl(This,context,acl) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricAccessControlClient_INTERFACE_DEFINED__ */


#ifndef __IFabricGetAclResult_INTERFACE_DEFINED__
#define __IFabricGetAclResult_INTERFACE_DEFINED__

/* interface IFabricGetAclResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricGetAclResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8c3fc89b-b8b1-4b18-bc04-34291de8d57f")
    IFabricGetAclResult : public IUnknown
    {
    public:
        virtual const FABRIC_SECURITY_ACL *STDMETHODCALLTYPE get_Acl( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricGetAclResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricGetAclResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricGetAclResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricGetAclResult * This);
        
        const FABRIC_SECURITY_ACL *( STDMETHODCALLTYPE *get_Acl )( 
            IFabricGetAclResult * This);
        
        END_INTERFACE
    } IFabricGetAclResultVtbl;

    interface IFabricGetAclResult
    {
        CONST_VTBL struct IFabricGetAclResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricGetAclResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricGetAclResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricGetAclResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricGetAclResult_get_Acl(This)	\
    ( (This)->lpVtbl -> get_Acl(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricGetAclResult_INTERFACE_DEFINED__ */


#ifndef __IFabricImageStoreClient_INTERFACE_DEFINED__
#define __IFabricImageStoreClient_INTERFACE_DEFINED__

/* interface IFabricImageStoreClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricImageStoreClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("df2f3510-f40c-42cd-9f04-7943e8a028b5")
    IFabricImageStoreClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Upload( 
            /* [in] */ LPCWSTR sourceFullPath,
            /* [in] */ LPCWSTR destinationRelativePath,
            /* [in] */ BOOLEAN shouldOverwrite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ LPCWSTR relativePath) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricImageStoreClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricImageStoreClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricImageStoreClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricImageStoreClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *Upload )( 
            IFabricImageStoreClient * This,
            /* [in] */ LPCWSTR sourceFullPath,
            /* [in] */ LPCWSTR destinationRelativePath,
            /* [in] */ BOOLEAN shouldOverwrite);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IFabricImageStoreClient * This,
            /* [in] */ LPCWSTR relativePath);
        
        END_INTERFACE
    } IFabricImageStoreClientVtbl;

    interface IFabricImageStoreClient
    {
        CONST_VTBL struct IFabricImageStoreClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricImageStoreClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricImageStoreClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricImageStoreClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricImageStoreClient_Upload(This,sourceFullPath,destinationRelativePath,shouldOverwrite)	\
    ( (This)->lpVtbl -> Upload(This,sourceFullPath,destinationRelativePath,shouldOverwrite) ) 

#define IFabricImageStoreClient_Delete(This,relativePath)	\
    ( (This)->lpVtbl -> Delete(This,relativePath) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricImageStoreClient_INTERFACE_DEFINED__ */


#ifndef __IInternalFabricQueryResult2_INTERFACE_DEFINED__
#define __IInternalFabricQueryResult2_INTERFACE_DEFINED__

/* interface IInternalFabricQueryResult2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IInternalFabricQueryResult2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5fa743e7-1ea6-4359-a918-fa11716b9c17")
    IInternalFabricQueryResult2 : public IInternalFabricQueryResult
    {
    public:
        virtual const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IInternalFabricQueryResult2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternalFabricQueryResult2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternalFabricQueryResult2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternalFabricQueryResult2 * This);
        
        const FABRIC_QUERY_RESULT *( STDMETHODCALLTYPE *get_Result )( 
            IInternalFabricQueryResult2 * This);
        
        const FABRIC_PAGING_STATUS *( STDMETHODCALLTYPE *get_PagingStatus )( 
            IInternalFabricQueryResult2 * This);
        
        END_INTERFACE
    } IInternalFabricQueryResult2Vtbl;

    interface IInternalFabricQueryResult2
    {
        CONST_VTBL struct IInternalFabricQueryResult2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternalFabricQueryResult2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IInternalFabricQueryResult2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IInternalFabricQueryResult2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IInternalFabricQueryResult2_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 


#define IInternalFabricQueryResult2_get_PagingStatus(This)	\
    ( (This)->lpVtbl -> get_PagingStatus(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IInternalFabricQueryResult2_INTERFACE_DEFINED__ */



#ifndef __FabricClientInternalModule_MODULE_DEFINED__
#define __FabricClientInternalModule_MODULE_DEFINED__


/* module FabricClientInternalModule */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT GetFabricClientDefaultSettings( 
    /* [retval][out] */ __RPC__deref_out_opt IFabricClientSettingsResult **result);

/* [entry] */ HRESULT FabricGetClientDllVersion( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **clientDllVersion);

#endif /* __FabricClientInternalModule_MODULE_DEFINED__ */
#endif /* __FabricClient_Lib_LIBRARY_DEFINED__ */

#ifndef __IFabricTestManagementClientInternal_INTERFACE_DEFINED__
#define __IFabricTestManagementClientInternal_INTERFACE_DEFINED__

/* interface IFabricTestManagementClientInternal */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTestManagementClientInternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("acc01447-6fb2-4d27-be74-cec878a21cea")
    IFabricTestManagementClientInternal : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginAddUnreliableLeaseBehavior( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR behaviorString,
            /* [in] */ LPCWSTR alias,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAddUnreliableLeaseBehavior( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRemoveUnreliableLeaseBehavior( 
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR alias,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRemoveUnreliableLeaseBehavior( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTestManagementClientInternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTestManagementClientInternal * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTestManagementClientInternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTestManagementClientInternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAddUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR behaviorString,
            /* [in] */ LPCWSTR alias,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndAddUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRemoveUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR alias,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRemoveUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricTestManagementClientInternalVtbl;

    interface IFabricTestManagementClientInternal
    {
        CONST_VTBL struct IFabricTestManagementClientInternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTestManagementClientInternal_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTestManagementClientInternal_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTestManagementClientInternal_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTestManagementClientInternal_BeginAddUnreliableLeaseBehavior(This,nodeName,behaviorString,alias,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginAddUnreliableLeaseBehavior(This,nodeName,behaviorString,alias,timeoutMilliseconds,callback,context) ) 

#define IFabricTestManagementClientInternal_EndAddUnreliableLeaseBehavior(This,context)	\
    ( (This)->lpVtbl -> EndAddUnreliableLeaseBehavior(This,context) ) 

#define IFabricTestManagementClientInternal_BeginRemoveUnreliableLeaseBehavior(This,nodeName,alias,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRemoveUnreliableLeaseBehavior(This,nodeName,alias,timeoutMilliseconds,callback,context) ) 

#define IFabricTestManagementClientInternal_EndRemoveUnreliableLeaseBehavior(This,context)	\
    ( (This)->lpVtbl -> EndRemoveUnreliableLeaseBehavior(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTestManagementClientInternal_INTERFACE_DEFINED__ */


#ifndef __IFabricTestManagementClientInternal2_INTERFACE_DEFINED__
#define __IFabricTestManagementClientInternal2_INTERFACE_DEFINED__

/* interface IFabricTestManagementClientInternal2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTestManagementClientInternal2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c744a0d9-9e79-45c8-bf46-41275c00ec32")
    IFabricTestManagementClientInternal2 : public IFabricTestManagementClientInternal
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginGetNodeListInternal( 
            /* [in] */ const FABRIC_NODE_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ BOOLEAN excludeStoppedNodeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetNodeList2Internal( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetNodeListResult2 **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTestManagementClientInternal2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTestManagementClientInternal2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTestManagementClientInternal2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTestManagementClientInternal2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAddUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal2 * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR behaviorString,
            /* [in] */ LPCWSTR alias,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndAddUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRemoveUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal2 * This,
            /* [in] */ LPCWSTR nodeName,
            /* [in] */ LPCWSTR alias,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRemoveUnreliableLeaseBehavior )( 
            IFabricTestManagementClientInternal2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetNodeListInternal )( 
            IFabricTestManagementClientInternal2 * This,
            /* [in] */ const FABRIC_NODE_QUERY_DESCRIPTION *queryDescription,
            /* [in] */ BOOLEAN excludeStoppedNodeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetNodeList2Internal )( 
            IFabricTestManagementClientInternal2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetNodeListResult2 **result);
        
        END_INTERFACE
    } IFabricTestManagementClientInternal2Vtbl;

    interface IFabricTestManagementClientInternal2
    {
        CONST_VTBL struct IFabricTestManagementClientInternal2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTestManagementClientInternal2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTestManagementClientInternal2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTestManagementClientInternal2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTestManagementClientInternal2_BeginAddUnreliableLeaseBehavior(This,nodeName,behaviorString,alias,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginAddUnreliableLeaseBehavior(This,nodeName,behaviorString,alias,timeoutMilliseconds,callback,context) ) 

#define IFabricTestManagementClientInternal2_EndAddUnreliableLeaseBehavior(This,context)	\
    ( (This)->lpVtbl -> EndAddUnreliableLeaseBehavior(This,context) ) 

#define IFabricTestManagementClientInternal2_BeginRemoveUnreliableLeaseBehavior(This,nodeName,alias,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRemoveUnreliableLeaseBehavior(This,nodeName,alias,timeoutMilliseconds,callback,context) ) 

#define IFabricTestManagementClientInternal2_EndRemoveUnreliableLeaseBehavior(This,context)	\
    ( (This)->lpVtbl -> EndRemoveUnreliableLeaseBehavior(This,context) ) 


#define IFabricTestManagementClientInternal2_BeginGetNodeListInternal(This,queryDescription,excludeStoppedNodeInfo,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetNodeListInternal(This,queryDescription,excludeStoppedNodeInfo,timeoutMilliseconds,callback,context) ) 

#define IFabricTestManagementClientInternal2_EndGetNodeList2Internal(This,context,result)	\
    ( (This)->lpVtbl -> EndGetNodeList2Internal(This,context,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTestManagementClientInternal2_INTERFACE_DEFINED__ */


#ifndef __IFabricFaultManagementClientInternal_INTERFACE_DEFINED__
#define __IFabricFaultManagementClientInternal_INTERFACE_DEFINED__

/* interface IFabricFaultManagementClientInternal */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricFaultManagementClientInternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("cd20ba5d-ca33-4a7d-b81f-9efd78189b3a")
    IFabricFaultManagementClientInternal : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginStopNodeInternal( 
            /* [in] */ const FABRIC_STOP_NODE_DESCRIPTION_INTERNAL *stopNodeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStopNodeInternal( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricFaultManagementClientInternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricFaultManagementClientInternal * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricFaultManagementClientInternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricFaultManagementClientInternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStopNodeInternal )( 
            IFabricFaultManagementClientInternal * This,
            /* [in] */ const FABRIC_STOP_NODE_DESCRIPTION_INTERNAL *stopNodeDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStopNodeInternal )( 
            IFabricFaultManagementClientInternal * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricFaultManagementClientInternalVtbl;

    interface IFabricFaultManagementClientInternal
    {
        CONST_VTBL struct IFabricFaultManagementClientInternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricFaultManagementClientInternal_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricFaultManagementClientInternal_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricFaultManagementClientInternal_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricFaultManagementClientInternal_BeginStopNodeInternal(This,stopNodeDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStopNodeInternal(This,stopNodeDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultManagementClientInternal_EndStopNodeInternal(This,context)	\
    ( (This)->lpVtbl -> EndStopNodeInternal(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricFaultManagementClientInternal_INTERFACE_DEFINED__ */


#ifndef __IFabricGetComposeDeploymentStatusListResult_INTERFACE_DEFINED__
#define __IFabricGetComposeDeploymentStatusListResult_INTERFACE_DEFINED__

/* interface IFabricGetComposeDeploymentStatusListResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricGetComposeDeploymentStatusListResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("539e2e3a-6ecd-4f81-b84b-5df65d78b12a")
    IFabricGetComposeDeploymentStatusListResult : public IUnknown
    {
    public:
        virtual const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ComposeDeploymentStatusQueryList( void) = 0;
        
        virtual const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricGetComposeDeploymentStatusListResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricGetComposeDeploymentStatusListResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricGetComposeDeploymentStatusListResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricGetComposeDeploymentStatusListResult * This);
        
        const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST *( STDMETHODCALLTYPE *get_ComposeDeploymentStatusQueryList )( 
            IFabricGetComposeDeploymentStatusListResult * This);
        
        const FABRIC_PAGING_STATUS *( STDMETHODCALLTYPE *get_PagingStatus )( 
            IFabricGetComposeDeploymentStatusListResult * This);
        
        END_INTERFACE
    } IFabricGetComposeDeploymentStatusListResultVtbl;

    interface IFabricGetComposeDeploymentStatusListResult
    {
        CONST_VTBL struct IFabricGetComposeDeploymentStatusListResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricGetComposeDeploymentStatusListResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricGetComposeDeploymentStatusListResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricGetComposeDeploymentStatusListResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricGetComposeDeploymentStatusListResult_get_ComposeDeploymentStatusQueryList(This)	\
    ( (This)->lpVtbl -> get_ComposeDeploymentStatusQueryList(This) ) 

#define IFabricGetComposeDeploymentStatusListResult_get_PagingStatus(This)	\
    ( (This)->lpVtbl -> get_PagingStatus(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricGetComposeDeploymentStatusListResult_INTERFACE_DEFINED__ */


#ifndef __IFabricComposeDeploymentUpgradeProgressResult_INTERFACE_DEFINED__
#define __IFabricComposeDeploymentUpgradeProgressResult_INTERFACE_DEFINED__

/* interface IFabricComposeDeploymentUpgradeProgressResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricComposeDeploymentUpgradeProgressResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c7880462-7328-406a-a6ce-a989ca551a74")
    IFabricComposeDeploymentUpgradeProgressResult : public IUnknown
    {
    public:
        virtual const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS *STDMETHODCALLTYPE get_UpgradeProgress( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricComposeDeploymentUpgradeProgressResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricComposeDeploymentUpgradeProgressResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricComposeDeploymentUpgradeProgressResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricComposeDeploymentUpgradeProgressResult * This);
        
        const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS *( STDMETHODCALLTYPE *get_UpgradeProgress )( 
            IFabricComposeDeploymentUpgradeProgressResult * This);
        
        END_INTERFACE
    } IFabricComposeDeploymentUpgradeProgressResultVtbl;

    interface IFabricComposeDeploymentUpgradeProgressResult
    {
        CONST_VTBL struct IFabricComposeDeploymentUpgradeProgressResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricComposeDeploymentUpgradeProgressResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricComposeDeploymentUpgradeProgressResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricComposeDeploymentUpgradeProgressResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricComposeDeploymentUpgradeProgressResult_get_UpgradeProgress(This)	\
    ( (This)->lpVtbl -> get_UpgradeProgress(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricComposeDeploymentUpgradeProgressResult_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


