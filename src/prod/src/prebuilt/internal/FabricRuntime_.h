

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

#ifndef __fabricruntime__h__
#define __fabricruntime__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricInternalBrokeredService_FWD_DEFINED__
#define __IFabricInternalBrokeredService_FWD_DEFINED__
typedef interface IFabricInternalBrokeredService IFabricInternalBrokeredService;

#endif 	/* __IFabricInternalBrokeredService_FWD_DEFINED__ */


#ifndef __IFabricDisposable_FWD_DEFINED__
#define __IFabricDisposable_FWD_DEFINED__
typedef interface IFabricDisposable IFabricDisposable;

#endif 	/* __IFabricDisposable_FWD_DEFINED__ */


#ifndef __IFabricCodePackageHost_FWD_DEFINED__
#define __IFabricCodePackageHost_FWD_DEFINED__
typedef interface IFabricCodePackageHost IFabricCodePackageHost;

#endif 	/* __IFabricCodePackageHost_FWD_DEFINED__ */


#ifndef __IFabricTransactionalReplicatorSettingsResult_FWD_DEFINED__
#define __IFabricTransactionalReplicatorSettingsResult_FWD_DEFINED__
typedef interface IFabricTransactionalReplicatorSettingsResult IFabricTransactionalReplicatorSettingsResult;

#endif 	/* __IFabricTransactionalReplicatorSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricBackupRestoreAgent_FWD_DEFINED__
#define __IFabricBackupRestoreAgent_FWD_DEFINED__
typedef interface IFabricBackupRestoreAgent IFabricBackupRestoreAgent;

#endif 	/* __IFabricBackupRestoreAgent_FWD_DEFINED__ */


#ifndef __IFabricBackupRestoreHandler_FWD_DEFINED__
#define __IFabricBackupRestoreHandler_FWD_DEFINED__
typedef interface IFabricBackupRestoreHandler IFabricBackupRestoreHandler;

#endif 	/* __IFabricBackupRestoreHandler_FWD_DEFINED__ */


#ifndef __IFabricInternalBrokeredService_FWD_DEFINED__
#define __IFabricInternalBrokeredService_FWD_DEFINED__
typedef interface IFabricInternalBrokeredService IFabricInternalBrokeredService;

#endif 	/* __IFabricInternalBrokeredService_FWD_DEFINED__ */


#ifndef __IFabricDisposable_FWD_DEFINED__
#define __IFabricDisposable_FWD_DEFINED__
typedef interface IFabricDisposable IFabricDisposable;

#endif 	/* __IFabricDisposable_FWD_DEFINED__ */


#ifndef __IFabricCodePackageHost_FWD_DEFINED__
#define __IFabricCodePackageHost_FWD_DEFINED__
typedef interface IFabricCodePackageHost IFabricCodePackageHost;

#endif 	/* __IFabricCodePackageHost_FWD_DEFINED__ */


#ifndef __IFabricBatchOperation_FWD_DEFINED__
#define __IFabricBatchOperation_FWD_DEFINED__
typedef interface IFabricBatchOperation IFabricBatchOperation;

#endif 	/* __IFabricBatchOperation_FWD_DEFINED__ */


#ifndef __IFabricBatchOperationData_FWD_DEFINED__
#define __IFabricBatchOperationData_FWD_DEFINED__
typedef interface IFabricBatchOperationData IFabricBatchOperationData;

#endif 	/* __IFabricBatchOperationData_FWD_DEFINED__ */


#ifndef __IFabricBatchOperationStream_FWD_DEFINED__
#define __IFabricBatchOperationStream_FWD_DEFINED__
typedef interface IFabricBatchOperationStream IFabricBatchOperationStream;

#endif 	/* __IFabricBatchOperationStream_FWD_DEFINED__ */


#ifndef __IFabricInternalStateReplicator_FWD_DEFINED__
#define __IFabricInternalStateReplicator_FWD_DEFINED__
typedef interface IFabricInternalStateReplicator IFabricInternalStateReplicator;

#endif 	/* __IFabricInternalStateReplicator_FWD_DEFINED__ */


#ifndef __IFabricInternalReplicator_FWD_DEFINED__
#define __IFabricInternalReplicator_FWD_DEFINED__
typedef interface IFabricInternalReplicator IFabricInternalReplicator;

#endif 	/* __IFabricInternalReplicator_FWD_DEFINED__ */


#ifndef __IFabricStateProviderSupportsCopyUntilLatestLsn_FWD_DEFINED__
#define __IFabricStateProviderSupportsCopyUntilLatestLsn_FWD_DEFINED__
typedef interface IFabricStateProviderSupportsCopyUntilLatestLsn IFabricStateProviderSupportsCopyUntilLatestLsn;

#endif 	/* __IFabricStateProviderSupportsCopyUntilLatestLsn_FWD_DEFINED__ */


#ifndef __IFabricInternalStatefulServiceReplica_FWD_DEFINED__
#define __IFabricInternalStatefulServiceReplica_FWD_DEFINED__
typedef interface IFabricInternalStatefulServiceReplica IFabricInternalStatefulServiceReplica;

#endif 	/* __IFabricInternalStatefulServiceReplica_FWD_DEFINED__ */


#ifndef __IFabricInternalStatefulServiceReplica2_FWD_DEFINED__
#define __IFabricInternalStatefulServiceReplica2_FWD_DEFINED__
typedef interface IFabricInternalStatefulServiceReplica2 IFabricInternalStatefulServiceReplica2;

#endif 	/* __IFabricInternalStatefulServiceReplica2_FWD_DEFINED__ */


#ifndef __IFabricStatefulServiceReplicaStatusResult_FWD_DEFINED__
#define __IFabricStatefulServiceReplicaStatusResult_FWD_DEFINED__
typedef interface IFabricStatefulServiceReplicaStatusResult IFabricStatefulServiceReplicaStatusResult;

#endif 	/* __IFabricStatefulServiceReplicaStatusResult_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplicaSettingsResult_FWD_DEFINED__
#define __IFabricKeyValueStoreReplicaSettingsResult_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplicaSettingsResult IFabricKeyValueStoreReplicaSettingsResult;

#endif 	/* __IFabricKeyValueStoreReplicaSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplicaSettings_V2Result_FWD_DEFINED__
#define __IFabricKeyValueStoreReplicaSettings_V2Result_FWD_DEFINED__
typedef interface IFabricKeyValueStoreReplicaSettings_V2Result IFabricKeyValueStoreReplicaSettings_V2Result;

#endif 	/* __IFabricKeyValueStoreReplicaSettings_V2Result_FWD_DEFINED__ */


#ifndef __IFabricSharedLogSettingsResult_FWD_DEFINED__
#define __IFabricSharedLogSettingsResult_FWD_DEFINED__
typedef interface IFabricSharedLogSettingsResult IFabricSharedLogSettingsResult;

#endif 	/* __IFabricSharedLogSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricInternalManagedReplicator_FWD_DEFINED__
#define __IFabricInternalManagedReplicator_FWD_DEFINED__
typedef interface IFabricInternalManagedReplicator IFabricInternalManagedReplicator;

#endif 	/* __IFabricInternalManagedReplicator_FWD_DEFINED__ */


#ifndef __IFabricTransactionalReplicatorRuntimeConfigurations_FWD_DEFINED__
#define __IFabricTransactionalReplicatorRuntimeConfigurations_FWD_DEFINED__
typedef interface IFabricTransactionalReplicatorRuntimeConfigurations IFabricTransactionalReplicatorRuntimeConfigurations;

#endif 	/* __IFabricTransactionalReplicatorRuntimeConfigurations_FWD_DEFINED__ */


#ifndef __IFabricStateProvider2Factory_FWD_DEFINED__
#define __IFabricStateProvider2Factory_FWD_DEFINED__
typedef interface IFabricStateProvider2Factory IFabricStateProvider2Factory;

#endif 	/* __IFabricStateProvider2Factory_FWD_DEFINED__ */


#ifndef __IFabricDataLossHandler_FWD_DEFINED__
#define __IFabricDataLossHandler_FWD_DEFINED__
typedef interface IFabricDataLossHandler IFabricDataLossHandler;

#endif 	/* __IFabricDataLossHandler_FWD_DEFINED__ */


#ifndef __IFabricInternalStatefulServicePartition_FWD_DEFINED__
#define __IFabricInternalStatefulServicePartition_FWD_DEFINED__
typedef interface IFabricInternalStatefulServicePartition IFabricInternalStatefulServicePartition;

#endif 	/* __IFabricInternalStatefulServicePartition_FWD_DEFINED__ */


#ifndef __IFabricTransactionalReplicator_FWD_DEFINED__
#define __IFabricTransactionalReplicator_FWD_DEFINED__
typedef interface IFabricTransactionalReplicator IFabricTransactionalReplicator;

#endif 	/* __IFabricTransactionalReplicator_FWD_DEFINED__ */


#ifndef __IFabricTransactionalReplicatorSettingsResult_FWD_DEFINED__
#define __IFabricTransactionalReplicatorSettingsResult_FWD_DEFINED__
typedef interface IFabricTransactionalReplicatorSettingsResult IFabricTransactionalReplicatorSettingsResult;

#endif 	/* __IFabricTransactionalReplicatorSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricGetBackupSchedulePolicyResult_FWD_DEFINED__
#define __IFabricGetBackupSchedulePolicyResult_FWD_DEFINED__
typedef interface IFabricGetBackupSchedulePolicyResult IFabricGetBackupSchedulePolicyResult;

#endif 	/* __IFabricGetBackupSchedulePolicyResult_FWD_DEFINED__ */


#ifndef __IFabricGetRestorePointDetailsResult_FWD_DEFINED__
#define __IFabricGetRestorePointDetailsResult_FWD_DEFINED__
typedef interface IFabricGetRestorePointDetailsResult IFabricGetRestorePointDetailsResult;

#endif 	/* __IFabricGetRestorePointDetailsResult_FWD_DEFINED__ */


#ifndef __IFabricBackupRestoreHandler_FWD_DEFINED__
#define __IFabricBackupRestoreHandler_FWD_DEFINED__
typedef interface IFabricBackupRestoreHandler IFabricBackupRestoreHandler;

#endif 	/* __IFabricBackupRestoreHandler_FWD_DEFINED__ */


#ifndef __IFabricBackupRestoreAgent_FWD_DEFINED__
#define __IFabricBackupRestoreAgent_FWD_DEFINED__
typedef interface IFabricBackupRestoreAgent IFabricBackupRestoreAgent;

#endif 	/* __IFabricBackupRestoreAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricTypes_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricruntime__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif




















extern RPC_IF_HANDLE __MIDL_itf_fabricruntime__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricruntime__0000_0000_v0_0_s_ifspec;


#ifndef __FabricRuntime_Lib_LIBRARY_DEFINED__
#define __FabricRuntime_Lib_LIBRARY_DEFINED__

/* library FabricRuntime_Lib */
/* [version][uuid] */ 


#pragma pack(push, 8)







#pragma pack(pop)

EXTERN_C const IID LIBID_FabricRuntime_Lib;

#ifndef __IFabricInternalBrokeredService_INTERFACE_DEFINED__
#define __IFabricInternalBrokeredService_INTERFACE_DEFINED__

/* interface IFabricInternalBrokeredService */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInternalBrokeredService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ed70f43b-8574-487c-b5ff-fcfcf6a6c6ea")
    IFabricInternalBrokeredService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBrokeredService( 
            /* [retval][out] */ IUnknown **service) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInternalBrokeredServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInternalBrokeredService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInternalBrokeredService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInternalBrokeredService * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBrokeredService )( 
            IFabricInternalBrokeredService * This,
            /* [retval][out] */ IUnknown **service);
        
        END_INTERFACE
    } IFabricInternalBrokeredServiceVtbl;

    interface IFabricInternalBrokeredService
    {
        CONST_VTBL struct IFabricInternalBrokeredServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInternalBrokeredService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInternalBrokeredService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInternalBrokeredService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInternalBrokeredService_GetBrokeredService(This,service)	\
    ( (This)->lpVtbl -> GetBrokeredService(This,service) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInternalBrokeredService_INTERFACE_DEFINED__ */


#ifndef __IFabricDisposable_INTERFACE_DEFINED__
#define __IFabricDisposable_INTERFACE_DEFINED__

/* interface IFabricDisposable */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricDisposable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0FE01003-90F4-456D-BD36-6B479B473978")
    IFabricDisposable : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE Dispose( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricDisposableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricDisposable * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricDisposable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricDisposable * This);
        
        void ( STDMETHODCALLTYPE *Dispose )( 
            IFabricDisposable * This);
        
        END_INTERFACE
    } IFabricDisposableVtbl;

    interface IFabricDisposable
    {
        CONST_VTBL struct IFabricDisposableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricDisposable_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricDisposable_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricDisposable_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricDisposable_Dispose(This)	\
    ( (This)->lpVtbl -> Dispose(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricDisposable_INTERFACE_DEFINED__ */


#ifndef __IFabricCodePackageHost_INTERFACE_DEFINED__
#define __IFabricCodePackageHost_INTERFACE_DEFINED__

/* interface IFabricCodePackageHost */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCodePackageHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("31027dc1-b9eb-4992-8793-67367bfc2d1a")
    IFabricCodePackageHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginActivate( 
            /* [in] */ LPCWSTR codePackageId,
            /* [in] */ IFabricCodePackageActivationContext *activationContext,
            /* [in] */ IFabricRuntime *fabricRuntime,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **operationContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndActivate( 
            /* [in] */ IFabricAsyncOperationContext *operationContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeactivate( 
            /* [in] */ LPCWSTR codePackageId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **operationContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeactivate( 
            /* [in] */ IFabricAsyncOperationContext *operationContext) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCodePackageHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCodePackageHost * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCodePackageHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCodePackageHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginActivate )( 
            IFabricCodePackageHost * This,
            /* [in] */ LPCWSTR codePackageId,
            /* [in] */ IFabricCodePackageActivationContext *activationContext,
            /* [in] */ IFabricRuntime *fabricRuntime,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **operationContext);
        
        HRESULT ( STDMETHODCALLTYPE *EndActivate )( 
            IFabricCodePackageHost * This,
            /* [in] */ IFabricAsyncOperationContext *operationContext);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeactivate )( 
            IFabricCodePackageHost * This,
            /* [in] */ LPCWSTR codePackageId,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **operationContext);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeactivate )( 
            IFabricCodePackageHost * This,
            /* [in] */ IFabricAsyncOperationContext *operationContext);
        
        END_INTERFACE
    } IFabricCodePackageHostVtbl;

    interface IFabricCodePackageHost
    {
        CONST_VTBL struct IFabricCodePackageHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCodePackageHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCodePackageHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCodePackageHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCodePackageHost_BeginActivate(This,codePackageId,activationContext,fabricRuntime,callback,operationContext)	\
    ( (This)->lpVtbl -> BeginActivate(This,codePackageId,activationContext,fabricRuntime,callback,operationContext) ) 

#define IFabricCodePackageHost_EndActivate(This,operationContext)	\
    ( (This)->lpVtbl -> EndActivate(This,operationContext) ) 

#define IFabricCodePackageHost_BeginDeactivate(This,codePackageId,callback,operationContext)	\
    ( (This)->lpVtbl -> BeginDeactivate(This,codePackageId,callback,operationContext) ) 

#define IFabricCodePackageHost_EndDeactivate(This,operationContext)	\
    ( (This)->lpVtbl -> EndDeactivate(This,operationContext) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCodePackageHost_INTERFACE_DEFINED__ */


#ifndef __IFabricTransactionalReplicatorSettingsResult_INTERFACE_DEFINED__
#define __IFabricTransactionalReplicatorSettingsResult_INTERFACE_DEFINED__

/* interface IFabricTransactionalReplicatorSettingsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransactionalReplicatorSettingsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d78caaf0-a190-414a-9de4-564ef6f25006")
    IFabricTransactionalReplicatorSettingsResult : public IUnknown
    {
    public:
        virtual const TRANSACTIONAL_REPLICATOR_SETTINGS *STDMETHODCALLTYPE get_TransactionalReplicatorSettings( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransactionalReplicatorSettingsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransactionalReplicatorSettingsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransactionalReplicatorSettingsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransactionalReplicatorSettingsResult * This);
        
        const TRANSACTIONAL_REPLICATOR_SETTINGS *( STDMETHODCALLTYPE *get_TransactionalReplicatorSettings )( 
            IFabricTransactionalReplicatorSettingsResult * This);
        
        END_INTERFACE
    } IFabricTransactionalReplicatorSettingsResultVtbl;

    interface IFabricTransactionalReplicatorSettingsResult
    {
        CONST_VTBL struct IFabricTransactionalReplicatorSettingsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransactionalReplicatorSettingsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransactionalReplicatorSettingsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransactionalReplicatorSettingsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransactionalReplicatorSettingsResult_get_TransactionalReplicatorSettings(This)	\
    ( (This)->lpVtbl -> get_TransactionalReplicatorSettings(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransactionalReplicatorSettingsResult_INTERFACE_DEFINED__ */


#ifndef __IFabricBackupRestoreAgent_INTERFACE_DEFINED__
#define __IFabricBackupRestoreAgent_INTERFACE_DEFINED__

/* interface IFabricBackupRestoreAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBackupRestoreAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1380040a-4cee-42af-92d5-25669ff45544")
    IFabricBackupRestoreAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterBackupRestoreReplica( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreAgent0001,
            /* [in] */ IFabricBackupRestoreHandler *__MIDL__IFabricBackupRestoreAgent0002) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterBackupRestoreReplica( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreAgent0004) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetBackupSchedulePolicy( 
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetBackupSchedulePolicy( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetBackupSchedulePolicyResult **policyResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetRestorePointDetails( 
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetRestorePointDetails( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetRestorePointDetailsResult **restorePointDetails) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportBackupOperationResult( 
            /* [in] */ FABRIC_BACKUP_OPERATION_RESULT *backupOperationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportBackupOperationResult( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportRestoreOperationResult( 
            /* [in] */ FABRIC_RESTORE_OPERATION_RESULT *restoreOperationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportRestoreOperationResult( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUploadBackup( 
            /* [in] */ FABRIC_BACKUP_UPLOAD_INFO *uploadInfo,
            /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUploadBackup( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDownloadBackup( 
            /* [in] */ FABRIC_BACKUP_DOWNLOAD_INFO *downloadInfo,
            /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDownloadBackup( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBackupRestoreAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBackupRestoreAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBackupRestoreAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterBackupRestoreReplica )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreAgent0001,
            /* [in] */ IFabricBackupRestoreHandler *__MIDL__IFabricBackupRestoreAgent0002);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterBackupRestoreReplica )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreAgent0004);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetBackupSchedulePolicy )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetBackupSchedulePolicy )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetBackupSchedulePolicyResult **policyResult);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetRestorePointDetails )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetRestorePointDetails )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetRestorePointDetailsResult **restorePointDetails);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportBackupOperationResult )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_BACKUP_OPERATION_RESULT *backupOperationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportBackupOperationResult )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportRestoreOperationResult )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_RESTORE_OPERATION_RESULT *restoreOperationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportRestoreOperationResult )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUploadBackup )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_BACKUP_UPLOAD_INFO *uploadInfo,
            /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUploadBackup )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDownloadBackup )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ FABRIC_BACKUP_DOWNLOAD_INFO *downloadInfo,
            /* [in] */ FABRIC_BACKUP_STORE_INFORMATION *storeInfo,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDownloadBackup )( 
            IFabricBackupRestoreAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricBackupRestoreAgentVtbl;

    interface IFabricBackupRestoreAgent
    {
        CONST_VTBL struct IFabricBackupRestoreAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBackupRestoreAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBackupRestoreAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBackupRestoreAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBackupRestoreAgent_RegisterBackupRestoreReplica(This,__MIDL__IFabricBackupRestoreAgent0000,__MIDL__IFabricBackupRestoreAgent0001,__MIDL__IFabricBackupRestoreAgent0002)	\
    ( (This)->lpVtbl -> RegisterBackupRestoreReplica(This,__MIDL__IFabricBackupRestoreAgent0000,__MIDL__IFabricBackupRestoreAgent0001,__MIDL__IFabricBackupRestoreAgent0002) ) 

#define IFabricBackupRestoreAgent_UnregisterBackupRestoreReplica(This,__MIDL__IFabricBackupRestoreAgent0003,__MIDL__IFabricBackupRestoreAgent0004)	\
    ( (This)->lpVtbl -> UnregisterBackupRestoreReplica(This,__MIDL__IFabricBackupRestoreAgent0003,__MIDL__IFabricBackupRestoreAgent0004) ) 

#define IFabricBackupRestoreAgent_BeginGetBackupSchedulePolicy(This,info,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetBackupSchedulePolicy(This,info,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreAgent_EndGetBackupSchedulePolicy(This,context,policyResult)	\
    ( (This)->lpVtbl -> EndGetBackupSchedulePolicy(This,context,policyResult) ) 

#define IFabricBackupRestoreAgent_BeginGetRestorePointDetails(This,info,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetRestorePointDetails(This,info,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreAgent_EndGetRestorePointDetails(This,context,restorePointDetails)	\
    ( (This)->lpVtbl -> EndGetRestorePointDetails(This,context,restorePointDetails) ) 

#define IFabricBackupRestoreAgent_BeginReportBackupOperationResult(This,backupOperationResult,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportBackupOperationResult(This,backupOperationResult,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreAgent_EndReportBackupOperationResult(This,context)	\
    ( (This)->lpVtbl -> EndReportBackupOperationResult(This,context) ) 

#define IFabricBackupRestoreAgent_BeginReportRestoreOperationResult(This,restoreOperationResult,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportRestoreOperationResult(This,restoreOperationResult,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreAgent_EndReportRestoreOperationResult(This,context)	\
    ( (This)->lpVtbl -> EndReportRestoreOperationResult(This,context) ) 

#define IFabricBackupRestoreAgent_BeginUploadBackup(This,uploadInfo,storeInfo,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginUploadBackup(This,uploadInfo,storeInfo,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreAgent_EndUploadBackup(This,context)	\
    ( (This)->lpVtbl -> EndUploadBackup(This,context) ) 

#define IFabricBackupRestoreAgent_BeginDownloadBackup(This,downloadInfo,storeInfo,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDownloadBackup(This,downloadInfo,storeInfo,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreAgent_EndDownloadBackup(This,context)	\
    ( (This)->lpVtbl -> EndDownloadBackup(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBackupRestoreAgent_INTERFACE_DEFINED__ */


#ifndef __IFabricBackupRestoreHandler_INTERFACE_DEFINED__
#define __IFabricBackupRestoreHandler_INTERFACE_DEFINED__

/* interface IFabricBackupRestoreHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBackupRestoreHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("39aa5c2c-7c6c-455a-b9a1-0c08964507c0")
    IFabricBackupRestoreHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginUpdateBackupSchedulePolicy( 
            /* [in] */ FABRIC_BACKUP_POLICY *policy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpdateBackupSchedulePolicy( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginPartitionBackupOperation( 
            /* [in] */ FABRIC_BACKUP_OPERATION_ID operationId,
            /* [in] */ FABRIC_BACKUP_CONFIGURATION *backupConfiguration,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndPartitionBackupOperation( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBackupRestoreHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBackupRestoreHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBackupRestoreHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBackupRestoreHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpdateBackupSchedulePolicy )( 
            IFabricBackupRestoreHandler * This,
            /* [in] */ FABRIC_BACKUP_POLICY *policy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpdateBackupSchedulePolicy )( 
            IFabricBackupRestoreHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginPartitionBackupOperation )( 
            IFabricBackupRestoreHandler * This,
            /* [in] */ FABRIC_BACKUP_OPERATION_ID operationId,
            /* [in] */ FABRIC_BACKUP_CONFIGURATION *backupConfiguration,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndPartitionBackupOperation )( 
            IFabricBackupRestoreHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricBackupRestoreHandlerVtbl;

    interface IFabricBackupRestoreHandler
    {
        CONST_VTBL struct IFabricBackupRestoreHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBackupRestoreHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBackupRestoreHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBackupRestoreHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBackupRestoreHandler_BeginUpdateBackupSchedulePolicy(This,policy,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginUpdateBackupSchedulePolicy(This,policy,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreHandler_EndUpdateBackupSchedulePolicy(This,context)	\
    ( (This)->lpVtbl -> EndUpdateBackupSchedulePolicy(This,context) ) 

#define IFabricBackupRestoreHandler_BeginPartitionBackupOperation(This,operationId,backupConfiguration,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginPartitionBackupOperation(This,operationId,backupConfiguration,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreHandler_EndPartitionBackupOperation(This,context)	\
    ( (This)->lpVtbl -> EndPartitionBackupOperation(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBackupRestoreHandler_INTERFACE_DEFINED__ */



#ifndef __FabricRuntimeInternalModule_MODULE_DEFINED__
#define __FabricRuntimeInternalModule_MODULE_DEFINED__


/* module FabricRuntimeInternalModule */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT FabricRegisterCodePackageHost( 
    /* [in] */ __RPC__in_opt IUnknown *codePackageHost);

/* [entry] */ HRESULT FabricGetRuntimeDllVersion( 
    /* [out] */ __RPC__deref_out_opt IFabricStringResult **runtimeDllVersion);

/* [entry] */ HRESULT GetFabricKeyValueStoreReplicaDefaultSettings( 
    /* [out] */ __RPC__deref_out_opt IFabricKeyValueStoreReplicaSettingsResult **result);

/* [entry] */ HRESULT FabricLoadClusterSecurityCredentials( 
    /* [out] */ __RPC__deref_out_opt IFabricSecurityCredentialsResult **securityCredentials);

/* [entry] */ void FabricDisableIpcLeasing( void);

/* [entry] */ HRESULT FabricLoadTransactionalReplicatorSettings( 
    /* [in] */ __RPC__in_opt const IFabricCodePackageActivationContext *codePackageActivationContext,
    /* [in] */ __RPC__in LPCWSTR configurationPackageName,
    /* [in] */ __RPC__in LPCWSTR sectionName,
    /* [retval][out] */ __RPC__deref_out_opt IFabricTransactionalReplicatorSettingsResult **replicatorSettings);

/* [entry] */ HRESULT GetFabricKeyValueStoreReplicaDefaultSettings_V2( 
    /* [in] */ __RPC__in LPCWSTR workingDirectory,
    /* [in] */ __RPC__in LPCWSTR sharedLogDirectory,
    /* [in] */ __RPC__in LPCWSTR sharedLogFileName,
    /* [in] */ GUID sharedLogGuid,
    /* [out] */ __RPC__deref_out_opt IFabricKeyValueStoreReplicaSettings_V2Result **result);

/* [entry] */ HRESULT FabricCreateKeyValueStoreReplica_V2( 
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ FABRIC_PARTITION_ID partitionId,
    /* [in] */ FABRIC_REPLICA_ID replicaId,
    /* [in] */ __RPC__in const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 *kvsSettings,
    /* [in] */ __RPC__in const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
    /* [in] */ __RPC__in_opt IFabricStoreEventHandler *storeEventHandler,
    /* [in] */ __RPC__in_opt IFabricSecondaryEventHandler *secondaryEventHandler,
    /* [retval][out] */ __RPC__deref_out_opt void **keyValueStore);

/* [entry] */ HRESULT GetFabricSharedLogDefaultSettings( 
    /* [in] */ __RPC__in LPCWSTR workingDirectory,
    /* [in] */ __RPC__in LPCWSTR sharedLogDirectory,
    /* [in] */ __RPC__in LPCWSTR sharedLogFileName,
    /* [in] */ GUID sharedLogGuid,
    /* [out] */ __RPC__deref_out_opt IFabricSharedLogSettingsResult **result);

/* [entry] */ HRESULT FabricCreateBackupRestoreAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **backupRestoreAgent);

#endif /* __FabricRuntimeInternalModule_MODULE_DEFINED__ */
#endif /* __FabricRuntime_Lib_LIBRARY_DEFINED__ */

#ifndef __IFabricBatchOperation_INTERFACE_DEFINED__
#define __IFabricBatchOperation_INTERFACE_DEFINED__

/* interface IFabricBatchOperation */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBatchOperation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4FFE9FB9-A70E-4C6A-8953-8D2FF79BCD9E")
    IFabricBatchOperation : public IUnknown
    {
    public:
        virtual FABRIC_SEQUENCE_NUMBER STDMETHODCALLTYPE get_FirstSequenceNumber( void) = 0;
        
        virtual FABRIC_SEQUENCE_NUMBER STDMETHODCALLTYPE get_LastSequenceNumber( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOperation( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER sequenceNumber,
            /* [retval][out] */ IFabricOperation **operation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Acknowledge( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBatchOperationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBatchOperation * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBatchOperation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBatchOperation * This);
        
        FABRIC_SEQUENCE_NUMBER ( STDMETHODCALLTYPE *get_FirstSequenceNumber )( 
            IFabricBatchOperation * This);
        
        FABRIC_SEQUENCE_NUMBER ( STDMETHODCALLTYPE *get_LastSequenceNumber )( 
            IFabricBatchOperation * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetOperation )( 
            IFabricBatchOperation * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER sequenceNumber,
            /* [retval][out] */ IFabricOperation **operation);
        
        HRESULT ( STDMETHODCALLTYPE *Acknowledge )( 
            IFabricBatchOperation * This);
        
        END_INTERFACE
    } IFabricBatchOperationVtbl;

    interface IFabricBatchOperation
    {
        CONST_VTBL struct IFabricBatchOperationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBatchOperation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBatchOperation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBatchOperation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBatchOperation_get_FirstSequenceNumber(This)	\
    ( (This)->lpVtbl -> get_FirstSequenceNumber(This) ) 

#define IFabricBatchOperation_get_LastSequenceNumber(This)	\
    ( (This)->lpVtbl -> get_LastSequenceNumber(This) ) 

#define IFabricBatchOperation_GetOperation(This,sequenceNumber,operation)	\
    ( (This)->lpVtbl -> GetOperation(This,sequenceNumber,operation) ) 

#define IFabricBatchOperation_Acknowledge(This)	\
    ( (This)->lpVtbl -> Acknowledge(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBatchOperation_INTERFACE_DEFINED__ */


#ifndef __IFabricBatchOperationData_INTERFACE_DEFINED__
#define __IFabricBatchOperationData_INTERFACE_DEFINED__

/* interface IFabricBatchOperationData */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBatchOperationData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("329327F3-9221-4532-94B2-E3D9DA8956DB")
    IFabricBatchOperationData : public IUnknown
    {
    public:
        virtual FABRIC_SEQUENCE_NUMBER STDMETHODCALLTYPE get_FirstSequenceNumber( void) = 0;
        
        virtual FABRIC_SEQUENCE_NUMBER STDMETHODCALLTYPE get_LastSequenceNumber( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER sequenceNumber,
            /* [out] */ ULONG *count,
            /* [retval][out] */ const FABRIC_OPERATION_DATA_BUFFER **buffers) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBatchOperationDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBatchOperationData * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBatchOperationData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBatchOperationData * This);
        
        FABRIC_SEQUENCE_NUMBER ( STDMETHODCALLTYPE *get_FirstSequenceNumber )( 
            IFabricBatchOperationData * This);
        
        FABRIC_SEQUENCE_NUMBER ( STDMETHODCALLTYPE *get_LastSequenceNumber )( 
            IFabricBatchOperationData * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IFabricBatchOperationData * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER sequenceNumber,
            /* [out] */ ULONG *count,
            /* [retval][out] */ const FABRIC_OPERATION_DATA_BUFFER **buffers);
        
        END_INTERFACE
    } IFabricBatchOperationDataVtbl;

    interface IFabricBatchOperationData
    {
        CONST_VTBL struct IFabricBatchOperationDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBatchOperationData_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBatchOperationData_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBatchOperationData_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBatchOperationData_get_FirstSequenceNumber(This)	\
    ( (This)->lpVtbl -> get_FirstSequenceNumber(This) ) 

#define IFabricBatchOperationData_get_LastSequenceNumber(This)	\
    ( (This)->lpVtbl -> get_LastSequenceNumber(This) ) 

#define IFabricBatchOperationData_GetData(This,sequenceNumber,count,buffers)	\
    ( (This)->lpVtbl -> GetData(This,sequenceNumber,count,buffers) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBatchOperationData_INTERFACE_DEFINED__ */


#ifndef __IFabricBatchOperationStream_INTERFACE_DEFINED__
#define __IFabricBatchOperationStream_INTERFACE_DEFINED__

/* interface IFabricBatchOperationStream */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBatchOperationStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F929D72F-9DE2-403F-B04B-CC4324AE4C71")
    IFabricBatchOperationStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginGetBatchOperation( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetBatchOperation( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBatchOperation **batchOperation) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBatchOperationStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBatchOperationStream * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBatchOperationStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBatchOperationStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetBatchOperation )( 
            IFabricBatchOperationStream * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetBatchOperation )( 
            IFabricBatchOperationStream * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBatchOperation **batchOperation);
        
        END_INTERFACE
    } IFabricBatchOperationStreamVtbl;

    interface IFabricBatchOperationStream
    {
        CONST_VTBL struct IFabricBatchOperationStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBatchOperationStream_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBatchOperationStream_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBatchOperationStream_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBatchOperationStream_BeginGetBatchOperation(This,callback,context)	\
    ( (This)->lpVtbl -> BeginGetBatchOperation(This,callback,context) ) 

#define IFabricBatchOperationStream_EndGetBatchOperation(This,context,batchOperation)	\
    ( (This)->lpVtbl -> EndGetBatchOperation(This,context,batchOperation) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBatchOperationStream_INTERFACE_DEFINED__ */


#ifndef __IFabricInternalStateReplicator_INTERFACE_DEFINED__
#define __IFabricInternalStateReplicator_INTERFACE_DEFINED__

/* interface IFabricInternalStateReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInternalStateReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4d76c2bb-f94b-49d0-b026-c22591ab6677")
    IFabricInternalStateReplicator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReserveSequenceNumber( 
            /* [in] */ BOOLEAN alwaysReserveWhenPrimary,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReplicateBatch( 
            /* [in] */ IFabricBatchOperationData *operationData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReplicateBatch( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBatchReplicationStream( 
            /* [retval][out] */ IFabricBatchOperationStream **stream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReplicationQueueCounters( 
            /* [out] */ FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS *counters) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInternalStateReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInternalStateReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInternalStateReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInternalStateReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReserveSequenceNumber )( 
            IFabricInternalStateReplicator * This,
            /* [in] */ BOOLEAN alwaysReserveWhenPrimary,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReplicateBatch )( 
            IFabricInternalStateReplicator * This,
            /* [in] */ IFabricBatchOperationData *operationData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReplicateBatch )( 
            IFabricInternalStateReplicator * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *GetBatchReplicationStream )( 
            IFabricInternalStateReplicator * This,
            /* [retval][out] */ IFabricBatchOperationStream **stream);
        
        HRESULT ( STDMETHODCALLTYPE *GetReplicationQueueCounters )( 
            IFabricInternalStateReplicator * This,
            /* [out] */ FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS *counters);
        
        END_INTERFACE
    } IFabricInternalStateReplicatorVtbl;

    interface IFabricInternalStateReplicator
    {
        CONST_VTBL struct IFabricInternalStateReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInternalStateReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInternalStateReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInternalStateReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInternalStateReplicator_ReserveSequenceNumber(This,alwaysReserveWhenPrimary,sequenceNumber)	\
    ( (This)->lpVtbl -> ReserveSequenceNumber(This,alwaysReserveWhenPrimary,sequenceNumber) ) 

#define IFabricInternalStateReplicator_BeginReplicateBatch(This,operationData,callback,context)	\
    ( (This)->lpVtbl -> BeginReplicateBatch(This,operationData,callback,context) ) 

#define IFabricInternalStateReplicator_EndReplicateBatch(This,context)	\
    ( (This)->lpVtbl -> EndReplicateBatch(This,context) ) 

#define IFabricInternalStateReplicator_GetBatchReplicationStream(This,stream)	\
    ( (This)->lpVtbl -> GetBatchReplicationStream(This,stream) ) 

#define IFabricInternalStateReplicator_GetReplicationQueueCounters(This,counters)	\
    ( (This)->lpVtbl -> GetReplicationQueueCounters(This,counters) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInternalStateReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricInternalReplicator_INTERFACE_DEFINED__
#define __IFabricInternalReplicator_INTERFACE_DEFINED__

/* interface IFabricInternalReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInternalReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FE488461-2ED5-4D3B-8E4E-5D02D50827AB")
    IFabricInternalReplicator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetReplicatorStatus( 
            /* [retval][out] */ IFabricGetReplicatorStatusResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInternalReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInternalReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInternalReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInternalReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetReplicatorStatus )( 
            IFabricInternalReplicator * This,
            /* [retval][out] */ IFabricGetReplicatorStatusResult **result);
        
        END_INTERFACE
    } IFabricInternalReplicatorVtbl;

    interface IFabricInternalReplicator
    {
        CONST_VTBL struct IFabricInternalReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInternalReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInternalReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInternalReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInternalReplicator_GetReplicatorStatus(This,result)	\
    ( (This)->lpVtbl -> GetReplicatorStatus(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInternalReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricStateProviderSupportsCopyUntilLatestLsn_INTERFACE_DEFINED__
#define __IFabricStateProviderSupportsCopyUntilLatestLsn_INTERFACE_DEFINED__

/* interface IFabricStateProviderSupportsCopyUntilLatestLsn */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStateProviderSupportsCopyUntilLatestLsn;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D37CE92B-DB5A-430D-8D4D-B40A9331F263")
    IFabricStateProviderSupportsCopyUntilLatestLsn : public IUnknown
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStateProviderSupportsCopyUntilLatestLsnVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStateProviderSupportsCopyUntilLatestLsn * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStateProviderSupportsCopyUntilLatestLsn * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStateProviderSupportsCopyUntilLatestLsn * This);
        
        END_INTERFACE
    } IFabricStateProviderSupportsCopyUntilLatestLsnVtbl;

    interface IFabricStateProviderSupportsCopyUntilLatestLsn
    {
        CONST_VTBL struct IFabricStateProviderSupportsCopyUntilLatestLsnVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStateProviderSupportsCopyUntilLatestLsn_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStateProviderSupportsCopyUntilLatestLsn_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStateProviderSupportsCopyUntilLatestLsn_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStateProviderSupportsCopyUntilLatestLsn_INTERFACE_DEFINED__ */


#ifndef __IFabricInternalStatefulServiceReplica_INTERFACE_DEFINED__
#define __IFabricInternalStatefulServiceReplica_INTERFACE_DEFINED__

/* interface IFabricInternalStatefulServiceReplica */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInternalStatefulServiceReplica;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E5395A45-C9FC-43F7-A5C7-4C34D6B56791")
    IFabricInternalStatefulServiceReplica : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [retval][out] */ IFabricStatefulServiceReplicaStatusResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInternalStatefulServiceReplicaVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInternalStatefulServiceReplica * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInternalStatefulServiceReplica * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInternalStatefulServiceReplica * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IFabricInternalStatefulServiceReplica * This,
            /* [retval][out] */ IFabricStatefulServiceReplicaStatusResult **result);
        
        END_INTERFACE
    } IFabricInternalStatefulServiceReplicaVtbl;

    interface IFabricInternalStatefulServiceReplica
    {
        CONST_VTBL struct IFabricInternalStatefulServiceReplicaVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInternalStatefulServiceReplica_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInternalStatefulServiceReplica_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInternalStatefulServiceReplica_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInternalStatefulServiceReplica_GetStatus(This,result)	\
    ( (This)->lpVtbl -> GetStatus(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInternalStatefulServiceReplica_INTERFACE_DEFINED__ */


#ifndef __IFabricInternalStatefulServiceReplica2_INTERFACE_DEFINED__
#define __IFabricInternalStatefulServiceReplica2_INTERFACE_DEFINED__

/* interface IFabricInternalStatefulServiceReplica2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInternalStatefulServiceReplica2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1A670F6C-7C0E-4E64-850D-58DECCA2D88C")
    IFabricInternalStatefulServiceReplica2 : public IFabricInternalStatefulServiceReplica
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpdateInitializationData( 
            /* [in] */ ULONG bufferSize,
            /* [in] */ const BYTE *buffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInternalStatefulServiceReplica2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInternalStatefulServiceReplica2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInternalStatefulServiceReplica2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInternalStatefulServiceReplica2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IFabricInternalStatefulServiceReplica2 * This,
            /* [retval][out] */ IFabricStatefulServiceReplicaStatusResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateInitializationData )( 
            IFabricInternalStatefulServiceReplica2 * This,
            /* [in] */ ULONG bufferSize,
            /* [in] */ const BYTE *buffer);
        
        END_INTERFACE
    } IFabricInternalStatefulServiceReplica2Vtbl;

    interface IFabricInternalStatefulServiceReplica2
    {
        CONST_VTBL struct IFabricInternalStatefulServiceReplica2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInternalStatefulServiceReplica2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInternalStatefulServiceReplica2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInternalStatefulServiceReplica2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInternalStatefulServiceReplica2_GetStatus(This,result)	\
    ( (This)->lpVtbl -> GetStatus(This,result) ) 


#define IFabricInternalStatefulServiceReplica2_UpdateInitializationData(This,bufferSize,buffer)	\
    ( (This)->lpVtbl -> UpdateInitializationData(This,bufferSize,buffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInternalStatefulServiceReplica2_INTERFACE_DEFINED__ */


#ifndef __IFabricStatefulServiceReplicaStatusResult_INTERFACE_DEFINED__
#define __IFabricStatefulServiceReplicaStatusResult_INTERFACE_DEFINED__

/* interface IFabricStatefulServiceReplicaStatusResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStatefulServiceReplicaStatusResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("908DCBB4-BB4E-4B2E-AE0A-46DAC42184BC")
    IFabricStatefulServiceReplicaStatusResult : public IUnknown
    {
    public:
        virtual const FABRIC_REPLICA_STATUS_QUERY_RESULT *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStatefulServiceReplicaStatusResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStatefulServiceReplicaStatusResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStatefulServiceReplicaStatusResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStatefulServiceReplicaStatusResult * This);
        
        const FABRIC_REPLICA_STATUS_QUERY_RESULT *( STDMETHODCALLTYPE *get_Result )( 
            IFabricStatefulServiceReplicaStatusResult * This);
        
        END_INTERFACE
    } IFabricStatefulServiceReplicaStatusResultVtbl;

    interface IFabricStatefulServiceReplicaStatusResult
    {
        CONST_VTBL struct IFabricStatefulServiceReplicaStatusResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStatefulServiceReplicaStatusResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStatefulServiceReplicaStatusResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStatefulServiceReplicaStatusResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStatefulServiceReplicaStatusResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStatefulServiceReplicaStatusResult_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplicaSettingsResult_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplicaSettingsResult_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplicaSettingsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplicaSettingsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE4F9D4A-406E-4FFE-A35D-0EF82A9D53CE")
    IFabricKeyValueStoreReplicaSettingsResult : public IUnknown
    {
    public:
        virtual const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS *STDMETHODCALLTYPE get_Settings( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplicaSettingsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplicaSettingsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplicaSettingsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplicaSettingsResult * This);
        
        const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS *( STDMETHODCALLTYPE *get_Settings )( 
            IFabricKeyValueStoreReplicaSettingsResult * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplicaSettingsResultVtbl;

    interface IFabricKeyValueStoreReplicaSettingsResult
    {
        CONST_VTBL struct IFabricKeyValueStoreReplicaSettingsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplicaSettingsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplicaSettingsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplicaSettingsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplicaSettingsResult_get_Settings(This)	\
    ( (This)->lpVtbl -> get_Settings(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplicaSettingsResult_INTERFACE_DEFINED__ */


#ifndef __IFabricKeyValueStoreReplicaSettings_V2Result_INTERFACE_DEFINED__
#define __IFabricKeyValueStoreReplicaSettings_V2Result_INTERFACE_DEFINED__

/* interface IFabricKeyValueStoreReplicaSettings_V2Result */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricKeyValueStoreReplicaSettings_V2Result;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("63BE1B43-1519-43F3-A1F5-5C2F3010009A")
    IFabricKeyValueStoreReplicaSettings_V2Result : public IUnknown
    {
    public:
        virtual const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 *STDMETHODCALLTYPE get_Settings( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricKeyValueStoreReplicaSettings_V2ResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricKeyValueStoreReplicaSettings_V2Result * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricKeyValueStoreReplicaSettings_V2Result * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricKeyValueStoreReplicaSettings_V2Result * This);
        
        const FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 *( STDMETHODCALLTYPE *get_Settings )( 
            IFabricKeyValueStoreReplicaSettings_V2Result * This);
        
        END_INTERFACE
    } IFabricKeyValueStoreReplicaSettings_V2ResultVtbl;

    interface IFabricKeyValueStoreReplicaSettings_V2Result
    {
        CONST_VTBL struct IFabricKeyValueStoreReplicaSettings_V2ResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricKeyValueStoreReplicaSettings_V2Result_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricKeyValueStoreReplicaSettings_V2Result_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricKeyValueStoreReplicaSettings_V2Result_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricKeyValueStoreReplicaSettings_V2Result_get_Settings(This)	\
    ( (This)->lpVtbl -> get_Settings(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricKeyValueStoreReplicaSettings_V2Result_INTERFACE_DEFINED__ */


#ifndef __IFabricSharedLogSettingsResult_INTERFACE_DEFINED__
#define __IFabricSharedLogSettingsResult_INTERFACE_DEFINED__

/* interface IFabricSharedLogSettingsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricSharedLogSettingsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FDCEAB77-F1A2-4E87-8B41-4849C5EBB348")
    IFabricSharedLogSettingsResult : public IUnknown
    {
    public:
        virtual const KTLLOGGER_SHARED_LOG_SETTINGS *STDMETHODCALLTYPE get_Settings( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricSharedLogSettingsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricSharedLogSettingsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricSharedLogSettingsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricSharedLogSettingsResult * This);
        
        const KTLLOGGER_SHARED_LOG_SETTINGS *( STDMETHODCALLTYPE *get_Settings )( 
            IFabricSharedLogSettingsResult * This);
        
        END_INTERFACE
    } IFabricSharedLogSettingsResultVtbl;

    interface IFabricSharedLogSettingsResult
    {
        CONST_VTBL struct IFabricSharedLogSettingsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricSharedLogSettingsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricSharedLogSettingsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricSharedLogSettingsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricSharedLogSettingsResult_get_Settings(This)	\
    ( (This)->lpVtbl -> get_Settings(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricSharedLogSettingsResult_INTERFACE_DEFINED__ */


#ifndef __IFabricInternalManagedReplicator_INTERFACE_DEFINED__
#define __IFabricInternalManagedReplicator_INTERFACE_DEFINED__

/* interface IFabricInternalManagedReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInternalManagedReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ee723951-c3a1-44f2-92e8-f50691d7cd16")
    IFabricInternalManagedReplicator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginReplicate2( 
            /* [in] */ ULONG bufferCount,
            /* [in] */ const FABRIC_OPERATION_DATA_BUFFER_EX1 *buffers,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInternalManagedReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInternalManagedReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInternalManagedReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInternalManagedReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReplicate2 )( 
            IFabricInternalManagedReplicator * This,
            /* [in] */ ULONG bufferCount,
            /* [in] */ const FABRIC_OPERATION_DATA_BUFFER_EX1 *buffers,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        END_INTERFACE
    } IFabricInternalManagedReplicatorVtbl;

    interface IFabricInternalManagedReplicator
    {
        CONST_VTBL struct IFabricInternalManagedReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInternalManagedReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInternalManagedReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInternalManagedReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInternalManagedReplicator_BeginReplicate2(This,bufferCount,buffers,callback,sequenceNumber,context)	\
    ( (This)->lpVtbl -> BeginReplicate2(This,bufferCount,buffers,callback,sequenceNumber,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInternalManagedReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricTransactionalReplicatorRuntimeConfigurations_INTERFACE_DEFINED__
#define __IFabricTransactionalReplicatorRuntimeConfigurations_INTERFACE_DEFINED__

/* interface IFabricTransactionalReplicatorRuntimeConfigurations */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransactionalReplicatorRuntimeConfigurations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72d749db-c9f2-4da8-a2a7-6893dbd65727")
    IFabricTransactionalReplicatorRuntimeConfigurations : public IUnknown
    {
    public:
        virtual LPCWSTR STDMETHODCALLTYPE get_WorkDirectory( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransactionalReplicatorRuntimeConfigurationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransactionalReplicatorRuntimeConfigurations * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransactionalReplicatorRuntimeConfigurations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransactionalReplicatorRuntimeConfigurations * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_WorkDirectory )( 
            IFabricTransactionalReplicatorRuntimeConfigurations * This);
        
        END_INTERFACE
    } IFabricTransactionalReplicatorRuntimeConfigurationsVtbl;

    interface IFabricTransactionalReplicatorRuntimeConfigurations
    {
        CONST_VTBL struct IFabricTransactionalReplicatorRuntimeConfigurationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransactionalReplicatorRuntimeConfigurations_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransactionalReplicatorRuntimeConfigurations_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransactionalReplicatorRuntimeConfigurations_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransactionalReplicatorRuntimeConfigurations_get_WorkDirectory(This)	\
    ( (This)->lpVtbl -> get_WorkDirectory(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransactionalReplicatorRuntimeConfigurations_INTERFACE_DEFINED__ */


#ifndef __IFabricStateProvider2Factory_INTERFACE_DEFINED__
#define __IFabricStateProvider2Factory_INTERFACE_DEFINED__

/* interface IFabricStateProvider2Factory */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStateProvider2Factory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ebc51ff5-5ea6-4f5a-b74f-a00ee8720616")
    IFabricStateProvider2Factory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Create( 
            /* [in] */ const FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS *factoryArguments,
            /* [retval][out] */ void **stateProvider) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStateProvider2FactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStateProvider2Factory * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStateProvider2Factory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStateProvider2Factory * This);
        
        HRESULT ( STDMETHODCALLTYPE *Create )( 
            IFabricStateProvider2Factory * This,
            /* [in] */ const FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS *factoryArguments,
            /* [retval][out] */ void **stateProvider);
        
        END_INTERFACE
    } IFabricStateProvider2FactoryVtbl;

    interface IFabricStateProvider2Factory
    {
        CONST_VTBL struct IFabricStateProvider2FactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStateProvider2Factory_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStateProvider2Factory_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStateProvider2Factory_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStateProvider2Factory_Create(This,factoryArguments,stateProvider)	\
    ( (This)->lpVtbl -> Create(This,factoryArguments,stateProvider) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStateProvider2Factory_INTERFACE_DEFINED__ */


#ifndef __IFabricDataLossHandler_INTERFACE_DEFINED__
#define __IFabricDataLossHandler_INTERFACE_DEFINED__

/* interface IFabricDataLossHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricDataLossHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0bba0a6a-8f00-41b5-9bbf-3ee30357028d")
    IFabricDataLossHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricDataLossHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricDataLossHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricDataLossHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricDataLossHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOnDataLoss )( 
            IFabricDataLossHandler * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOnDataLoss )( 
            IFabricDataLossHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ BOOLEAN *isStateChanged);
        
        END_INTERFACE
    } IFabricDataLossHandlerVtbl;

    interface IFabricDataLossHandler
    {
        CONST_VTBL struct IFabricDataLossHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricDataLossHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricDataLossHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricDataLossHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricDataLossHandler_BeginOnDataLoss(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOnDataLoss(This,callback,context) ) 

#define IFabricDataLossHandler_EndOnDataLoss(This,context,isStateChanged)	\
    ( (This)->lpVtbl -> EndOnDataLoss(This,context,isStateChanged) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricDataLossHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricInternalStatefulServicePartition_INTERFACE_DEFINED__
#define __IFabricInternalStatefulServicePartition_INTERFACE_DEFINED__

/* interface IFabricInternalStatefulServicePartition */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInternalStatefulServicePartition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9f1f0101-1ab2-4a18-8b5b-17b61fd99699")
    IFabricInternalStatefulServicePartition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTransactionalReplicator( 
            /* [in] */ IFabricStateProvider2Factory *stateProviderFactory,
            /* [in] */ IFabricDataLossHandler *dataLossHandler,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [in] */ const TRANSACTIONAL_REPLICATOR_SETTINGS *transactionalReplicatorSettings,
            /* [in] */ const KTLLOGGER_SHARED_LOG_SETTINGS *ktlloggerSharedSettings,
            /* [out] */ IFabricPrimaryReplicator **primaryReplicator,
            /* [retval][out] */ void **transactionalReplicator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTransactionalReplicatorInternal( 
            /* [in] */ IFabricTransactionalReplicatorRuntimeConfigurations *runtimeConfigurations,
            /* [in] */ IFabricStateProvider2Factory *stateProviderFactory,
            /* [in] */ IFabricDataLossHandler *dataLossHandler,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [in] */ const TRANSACTIONAL_REPLICATOR_SETTINGS *transactionalReplicatorSettings,
            /* [in] */ const KTLLOGGER_SHARED_LOG_SETTINGS *ktlloggerSharedSettings,
            /* [out] */ IFabricPrimaryReplicator **primaryReplicator,
            /* [retval][out] */ void **transactionalReplicator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKtlSystem( 
            /* [retval][out] */ void **ktlSystem) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInternalStatefulServicePartitionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInternalStatefulServicePartition * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInternalStatefulServicePartition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInternalStatefulServicePartition * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransactionalReplicator )( 
            IFabricInternalStatefulServicePartition * This,
            /* [in] */ IFabricStateProvider2Factory *stateProviderFactory,
            /* [in] */ IFabricDataLossHandler *dataLossHandler,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [in] */ const TRANSACTIONAL_REPLICATOR_SETTINGS *transactionalReplicatorSettings,
            /* [in] */ const KTLLOGGER_SHARED_LOG_SETTINGS *ktlloggerSharedSettings,
            /* [out] */ IFabricPrimaryReplicator **primaryReplicator,
            /* [retval][out] */ void **transactionalReplicator);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTransactionalReplicatorInternal )( 
            IFabricInternalStatefulServicePartition * This,
            /* [in] */ IFabricTransactionalReplicatorRuntimeConfigurations *runtimeConfigurations,
            /* [in] */ IFabricStateProvider2Factory *stateProviderFactory,
            /* [in] */ IFabricDataLossHandler *dataLossHandler,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
            /* [in] */ const TRANSACTIONAL_REPLICATOR_SETTINGS *transactionalReplicatorSettings,
            /* [in] */ const KTLLOGGER_SHARED_LOG_SETTINGS *ktlloggerSharedSettings,
            /* [out] */ IFabricPrimaryReplicator **primaryReplicator,
            /* [retval][out] */ void **transactionalReplicator);
        
        HRESULT ( STDMETHODCALLTYPE *GetKtlSystem )( 
            IFabricInternalStatefulServicePartition * This,
            /* [retval][out] */ void **ktlSystem);
        
        END_INTERFACE
    } IFabricInternalStatefulServicePartitionVtbl;

    interface IFabricInternalStatefulServicePartition
    {
        CONST_VTBL struct IFabricInternalStatefulServicePartitionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInternalStatefulServicePartition_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInternalStatefulServicePartition_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInternalStatefulServicePartition_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInternalStatefulServicePartition_CreateTransactionalReplicator(This,stateProviderFactory,dataLossHandler,replicatorSettings,transactionalReplicatorSettings,ktlloggerSharedSettings,primaryReplicator,transactionalReplicator)	\
    ( (This)->lpVtbl -> CreateTransactionalReplicator(This,stateProviderFactory,dataLossHandler,replicatorSettings,transactionalReplicatorSettings,ktlloggerSharedSettings,primaryReplicator,transactionalReplicator) ) 

#define IFabricInternalStatefulServicePartition_CreateTransactionalReplicatorInternal(This,runtimeConfigurations,stateProviderFactory,dataLossHandler,replicatorSettings,transactionalReplicatorSettings,ktlloggerSharedSettings,primaryReplicator,transactionalReplicator)	\
    ( (This)->lpVtbl -> CreateTransactionalReplicatorInternal(This,runtimeConfigurations,stateProviderFactory,dataLossHandler,replicatorSettings,transactionalReplicatorSettings,ktlloggerSharedSettings,primaryReplicator,transactionalReplicator) ) 

#define IFabricInternalStatefulServicePartition_GetKtlSystem(This,ktlSystem)	\
    ( (This)->lpVtbl -> GetKtlSystem(This,ktlSystem) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInternalStatefulServicePartition_INTERFACE_DEFINED__ */


#ifndef __IFabricTransactionalReplicator_INTERFACE_DEFINED__
#define __IFabricTransactionalReplicator_INTERFACE_DEFINED__

/* interface IFabricTransactionalReplicator */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransactionalReplicator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e4b0f177-4f61-41bc-996f-9560beb33d0c")
    IFabricTransactionalReplicator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpdateReplicatorSettings( 
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransactionalReplicatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransactionalReplicator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransactionalReplicator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransactionalReplicator * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateReplicatorSettings )( 
            IFabricTransactionalReplicator * This,
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        END_INTERFACE
    } IFabricTransactionalReplicatorVtbl;

    interface IFabricTransactionalReplicator
    {
        CONST_VTBL struct IFabricTransactionalReplicatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransactionalReplicator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransactionalReplicator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransactionalReplicator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransactionalReplicator_UpdateReplicatorSettings(This,replicatorSettings)	\
    ( (This)->lpVtbl -> UpdateReplicatorSettings(This,replicatorSettings) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransactionalReplicator_INTERFACE_DEFINED__ */


#ifndef __IFabricGetBackupSchedulePolicyResult_INTERFACE_DEFINED__
#define __IFabricGetBackupSchedulePolicyResult_INTERFACE_DEFINED__

/* interface IFabricGetBackupSchedulePolicyResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricGetBackupSchedulePolicyResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5b3e9db0-c280-4a16-ab73-2c0005c1543f")
    IFabricGetBackupSchedulePolicyResult : public IUnknown
    {
    public:
        virtual const FABRIC_BACKUP_POLICY *STDMETHODCALLTYPE get_BackupSchedulePolicy( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricGetBackupSchedulePolicyResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricGetBackupSchedulePolicyResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricGetBackupSchedulePolicyResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricGetBackupSchedulePolicyResult * This);
        
        const FABRIC_BACKUP_POLICY *( STDMETHODCALLTYPE *get_BackupSchedulePolicy )( 
            IFabricGetBackupSchedulePolicyResult * This);
        
        END_INTERFACE
    } IFabricGetBackupSchedulePolicyResultVtbl;

    interface IFabricGetBackupSchedulePolicyResult
    {
        CONST_VTBL struct IFabricGetBackupSchedulePolicyResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricGetBackupSchedulePolicyResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricGetBackupSchedulePolicyResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricGetBackupSchedulePolicyResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricGetBackupSchedulePolicyResult_get_BackupSchedulePolicy(This)	\
    ( (This)->lpVtbl -> get_BackupSchedulePolicy(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricGetBackupSchedulePolicyResult_INTERFACE_DEFINED__ */


#ifndef __IFabricGetRestorePointDetailsResult_INTERFACE_DEFINED__
#define __IFabricGetRestorePointDetailsResult_INTERFACE_DEFINED__

/* interface IFabricGetRestorePointDetailsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricGetRestorePointDetailsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("64965ed4-b3d0-4574-adb8-35f084f0aa06")
    IFabricGetRestorePointDetailsResult : public IUnknown
    {
    public:
        virtual const FABRIC_RESTORE_POINT_DETAILS *STDMETHODCALLTYPE get_RestorePointDetails( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricGetRestorePointDetailsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricGetRestorePointDetailsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricGetRestorePointDetailsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricGetRestorePointDetailsResult * This);
        
        const FABRIC_RESTORE_POINT_DETAILS *( STDMETHODCALLTYPE *get_RestorePointDetails )( 
            IFabricGetRestorePointDetailsResult * This);
        
        END_INTERFACE
    } IFabricGetRestorePointDetailsResultVtbl;

    interface IFabricGetRestorePointDetailsResult
    {
        CONST_VTBL struct IFabricGetRestorePointDetailsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricGetRestorePointDetailsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricGetRestorePointDetailsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricGetRestorePointDetailsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricGetRestorePointDetailsResult_get_RestorePointDetails(This)	\
    ( (This)->lpVtbl -> get_RestorePointDetails(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricGetRestorePointDetailsResult_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


