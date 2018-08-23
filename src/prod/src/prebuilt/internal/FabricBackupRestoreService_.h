

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

#ifndef __fabricbackuprestoreservice__h__
#define __fabricbackuprestoreservice__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricBackupRestoreService_FWD_DEFINED__
#define __IFabricBackupRestoreService_FWD_DEFINED__
typedef interface IFabricBackupRestoreService IFabricBackupRestoreService;

#endif 	/* __IFabricBackupRestoreService_FWD_DEFINED__ */


#ifndef __IFabricBackupRestoreServiceAgent_FWD_DEFINED__
#define __IFabricBackupRestoreServiceAgent_FWD_DEFINED__
typedef interface IFabricBackupRestoreServiceAgent IFabricBackupRestoreServiceAgent;

#endif 	/* __IFabricBackupRestoreServiceAgent_FWD_DEFINED__ */


#ifndef __FabricBackupRestoreServiceAgent_FWD_DEFINED__
#define __FabricBackupRestoreServiceAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricBackupRestoreServiceAgent FabricBackupRestoreServiceAgent;
#else
typedef struct FabricBackupRestoreServiceAgent FabricBackupRestoreServiceAgent;
#endif /* __cplusplus */

#endif 	/* __FabricBackupRestoreServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricBackupRestoreService_FWD_DEFINED__
#define __IFabricBackupRestoreService_FWD_DEFINED__
typedef interface IFabricBackupRestoreService IFabricBackupRestoreService;

#endif 	/* __IFabricBackupRestoreService_FWD_DEFINED__ */


#ifndef __IFabricBackupRestoreServiceAgent_FWD_DEFINED__
#define __IFabricBackupRestoreServiceAgent_FWD_DEFINED__
typedef interface IFabricBackupRestoreServiceAgent IFabricBackupRestoreServiceAgent;

#endif 	/* __IFabricBackupRestoreServiceAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"
#include "FabricRuntime_.h"
#include "FabricClient_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricbackuprestoreservice__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif




extern RPC_IF_HANDLE __MIDL_itf_fabricbackuprestoreservice__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricbackuprestoreservice__0000_0000_v0_0_s_ifspec;


#ifndef __FabricBackupRestoreServiceLib_LIBRARY_DEFINED__
#define __FabricBackupRestoreServiceLib_LIBRARY_DEFINED__

/* library FabricBackupRestoreServiceLib */
/* [version][uuid] */ 


#pragma pack(push, 8)



#pragma pack(pop)

EXTERN_C const IID LIBID_FabricBackupRestoreServiceLib;

#ifndef __IFabricBackupRestoreService_INTERFACE_DEFINED__
#define __IFabricBackupRestoreService_INTERFACE_DEFINED__

/* interface IFabricBackupRestoreService */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBackupRestoreService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("785d56a7-de89-490f-a2e2-70ca20b51159")
    IFabricBackupRestoreService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginGetBackupSchedulePolicy( 
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetBackupSchedulePolicy( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetBackupSchedulePolicyResult **policy) = 0;
        
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
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBackupRestoreServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBackupRestoreService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBackupRestoreService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBackupRestoreService * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetBackupSchedulePolicy )( 
            IFabricBackupRestoreService * This,
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetBackupSchedulePolicy )( 
            IFabricBackupRestoreService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetBackupSchedulePolicyResult **policy);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetRestorePointDetails )( 
            IFabricBackupRestoreService * This,
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetRestorePointDetails )( 
            IFabricBackupRestoreService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricGetRestorePointDetailsResult **restorePointDetails);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportBackupOperationResult )( 
            IFabricBackupRestoreService * This,
            /* [in] */ FABRIC_BACKUP_OPERATION_RESULT *backupOperationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportBackupOperationResult )( 
            IFabricBackupRestoreService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportRestoreOperationResult )( 
            IFabricBackupRestoreService * This,
            /* [in] */ FABRIC_RESTORE_OPERATION_RESULT *restoreOperationResult,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportRestoreOperationResult )( 
            IFabricBackupRestoreService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricBackupRestoreServiceVtbl;

    interface IFabricBackupRestoreService
    {
        CONST_VTBL struct IFabricBackupRestoreServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBackupRestoreService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBackupRestoreService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBackupRestoreService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBackupRestoreService_BeginGetBackupSchedulePolicy(This,info,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetBackupSchedulePolicy(This,info,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreService_EndGetBackupSchedulePolicy(This,context,policy)	\
    ( (This)->lpVtbl -> EndGetBackupSchedulePolicy(This,context,policy) ) 

#define IFabricBackupRestoreService_BeginGetRestorePointDetails(This,info,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetRestorePointDetails(This,info,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreService_EndGetRestorePointDetails(This,context,restorePointDetails)	\
    ( (This)->lpVtbl -> EndGetRestorePointDetails(This,context,restorePointDetails) ) 

#define IFabricBackupRestoreService_BeginReportBackupOperationResult(This,backupOperationResult,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportBackupOperationResult(This,backupOperationResult,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreService_EndReportBackupOperationResult(This,context)	\
    ( (This)->lpVtbl -> EndReportBackupOperationResult(This,context) ) 

#define IFabricBackupRestoreService_BeginReportRestoreOperationResult(This,restoreOperationResult,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportRestoreOperationResult(This,restoreOperationResult,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreService_EndReportRestoreOperationResult(This,context)	\
    ( (This)->lpVtbl -> EndReportRestoreOperationResult(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBackupRestoreService_INTERFACE_DEFINED__ */


#ifndef __IFabricBackupRestoreServiceAgent_INTERFACE_DEFINED__
#define __IFabricBackupRestoreServiceAgent_INTERFACE_DEFINED__

/* interface IFabricBackupRestoreServiceAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBackupRestoreServiceAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f3521ca3-d166-4954-861f-371a04d7b414")
    IFabricBackupRestoreServiceAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterBackupRestoreService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreServiceAgent0001,
            /* [in] */ IFabricBackupRestoreService *__MIDL__IFabricBackupRestoreServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterBackupRestoreService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreServiceAgent0004) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUpdateBackupSchedulePolicy( 
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ FABRIC_BACKUP_POLICY *policy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpdateBackupSchedulePolicy( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginPartitionBackupOperation( 
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ FABRIC_BACKUP_OPERATION_ID operationId,
            /* [in] */ FABRIC_BACKUP_CONFIGURATION *backupConfiguration,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndPartitionBackupOperation( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBackupRestoreServiceAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBackupRestoreServiceAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBackupRestoreServiceAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBackupRestoreServiceAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterBackupRestoreService )( 
            IFabricBackupRestoreServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreServiceAgent0001,
            /* [in] */ IFabricBackupRestoreService *__MIDL__IFabricBackupRestoreServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterBackupRestoreService )( 
            IFabricBackupRestoreServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricBackupRestoreServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricBackupRestoreServiceAgent0004);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpdateBackupSchedulePolicy )( 
            IFabricBackupRestoreServiceAgent * This,
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ FABRIC_BACKUP_POLICY *policy,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpdateBackupSchedulePolicy )( 
            IFabricBackupRestoreServiceAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginPartitionBackupOperation )( 
            IFabricBackupRestoreServiceAgent * This,
            /* [in] */ FABRIC_BACKUP_PARTITION_INFO *info,
            /* [in] */ FABRIC_BACKUP_OPERATION_ID operationId,
            /* [in] */ FABRIC_BACKUP_CONFIGURATION *backupConfiguration,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndPartitionBackupOperation )( 
            IFabricBackupRestoreServiceAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricBackupRestoreServiceAgentVtbl;

    interface IFabricBackupRestoreServiceAgent
    {
        CONST_VTBL struct IFabricBackupRestoreServiceAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBackupRestoreServiceAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBackupRestoreServiceAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBackupRestoreServiceAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBackupRestoreServiceAgent_RegisterBackupRestoreService(This,__MIDL__IFabricBackupRestoreServiceAgent0000,__MIDL__IFabricBackupRestoreServiceAgent0001,__MIDL__IFabricBackupRestoreServiceAgent0002,serviceAddress)	\
    ( (This)->lpVtbl -> RegisterBackupRestoreService(This,__MIDL__IFabricBackupRestoreServiceAgent0000,__MIDL__IFabricBackupRestoreServiceAgent0001,__MIDL__IFabricBackupRestoreServiceAgent0002,serviceAddress) ) 

#define IFabricBackupRestoreServiceAgent_UnregisterBackupRestoreService(This,__MIDL__IFabricBackupRestoreServiceAgent0003,__MIDL__IFabricBackupRestoreServiceAgent0004)	\
    ( (This)->lpVtbl -> UnregisterBackupRestoreService(This,__MIDL__IFabricBackupRestoreServiceAgent0003,__MIDL__IFabricBackupRestoreServiceAgent0004) ) 

#define IFabricBackupRestoreServiceAgent_BeginUpdateBackupSchedulePolicy(This,info,policy,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginUpdateBackupSchedulePolicy(This,info,policy,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreServiceAgent_EndUpdateBackupSchedulePolicy(This,context)	\
    ( (This)->lpVtbl -> EndUpdateBackupSchedulePolicy(This,context) ) 

#define IFabricBackupRestoreServiceAgent_BeginPartitionBackupOperation(This,info,operationId,backupConfiguration,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginPartitionBackupOperation(This,info,operationId,backupConfiguration,timeoutMilliseconds,callback,context) ) 

#define IFabricBackupRestoreServiceAgent_EndPartitionBackupOperation(This,context)	\
    ( (This)->lpVtbl -> EndPartitionBackupOperation(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBackupRestoreServiceAgent_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricBackupRestoreServiceAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("1ed41302-5ada-431c-a70f-728bab21c2ac")
FabricBackupRestoreServiceAgent;
#endif


#ifndef __FabricBackupRestoreService_MODULE_DEFINED__
#define __FabricBackupRestoreService_MODULE_DEFINED__


/* module FabricBackupRestoreService */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricBackupRestoreServiceAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricBackupRestoreServiceAgent);

#endif /* __FabricBackupRestoreService_MODULE_DEFINED__ */
#endif /* __FabricBackupRestoreServiceLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


