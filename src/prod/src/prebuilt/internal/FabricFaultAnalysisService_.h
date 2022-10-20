

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

#ifndef __fabricfaultanalysisservice__h__
#define __fabricfaultanalysisservice__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricFaultAnalysisService_FWD_DEFINED__
#define __IFabricFaultAnalysisService_FWD_DEFINED__
typedef interface IFabricFaultAnalysisService IFabricFaultAnalysisService;

#endif 	/* __IFabricFaultAnalysisService_FWD_DEFINED__ */


#ifndef __IFabricFaultAnalysisServiceAgent_FWD_DEFINED__
#define __IFabricFaultAnalysisServiceAgent_FWD_DEFINED__
typedef interface IFabricFaultAnalysisServiceAgent IFabricFaultAnalysisServiceAgent;

#endif 	/* __IFabricFaultAnalysisServiceAgent_FWD_DEFINED__ */


#ifndef __FabricFaultAnalysisServiceAgent_FWD_DEFINED__
#define __FabricFaultAnalysisServiceAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricFaultAnalysisServiceAgent FabricFaultAnalysisServiceAgent;
#else
typedef struct FabricFaultAnalysisServiceAgent FabricFaultAnalysisServiceAgent;
#endif /* __cplusplus */

#endif 	/* __FabricFaultAnalysisServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricFaultAnalysisService_FWD_DEFINED__
#define __IFabricFaultAnalysisService_FWD_DEFINED__
typedef interface IFabricFaultAnalysisService IFabricFaultAnalysisService;

#endif 	/* __IFabricFaultAnalysisService_FWD_DEFINED__ */


#ifndef __IFabricFaultAnalysisServiceAgent_FWD_DEFINED__
#define __IFabricFaultAnalysisServiceAgent_FWD_DEFINED__
typedef interface IFabricFaultAnalysisServiceAgent IFabricFaultAnalysisServiceAgent;

#endif 	/* __IFabricFaultAnalysisServiceAgent_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"
#include "FabricRuntime.h"
#include "FabricClient_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricfaultanalysisservice__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif




extern RPC_IF_HANDLE __MIDL_itf_fabricfaultanalysisservice__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricfaultanalysisservice__0000_0000_v0_0_s_ifspec;


#ifndef __FabricFaultAnalysisServiceLib_LIBRARY_DEFINED__
#define __FabricFaultAnalysisServiceLib_LIBRARY_DEFINED__

/* library FabricFaultAnalysisServiceLib */
/* [version][uuid] */ 


#pragma pack(push, 8)



#pragma pack(pop)

EXTERN_C const IID LIBID_FabricFaultAnalysisServiceLib;

#ifndef __IFabricFaultAnalysisService_INTERFACE_DEFINED__
#define __IFabricFaultAnalysisService_INTERFACE_DEFINED__

/* interface IFabricFaultAnalysisService */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricFaultAnalysisService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("bf9b93a9-5b74-4a28-b205-edf387adf3db")
    IFabricFaultAnalysisService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginStartPartitionDataLoss( 
            /* [in] */ FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION *invokeDataLossDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartPartitionDataLoss( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetPartitionDataLossProgress( 
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetPartitionDataLossProgress( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionDataLossProgressResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStartPartitionQuorumLoss( 
            /* [in] */ FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION *invokeQuorumLossDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartPartitionQuorumLoss( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetPartitionQuorumLossProgress( 
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetPartitionQuorumLossProgress( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionQuorumLossProgressResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStartPartitionRestart( 
            /* [in] */ FABRIC_START_PARTITION_RESTART_DESCRIPTION *restartPartitionDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartPartitionRestart( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetPartitionRestartProgress( 
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetPartitionRestartProgress( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionRestartProgressResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetTestCommandStatusList( 
            /* [in] */ FABRIC_TEST_COMMAND_LIST_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetTestCommandStatusList( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricTestCommandStatusResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCancelTestCommand( 
            /* [in] */ FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCancelTestCommand( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStartChaos( 
            /* [in] */ FABRIC_START_CHAOS_DESCRIPTION *startChaosDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartChaos( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStopChaos( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStopChaos( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetChaosReport( 
            /* [in] */ FABRIC_GET_CHAOS_REPORT_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetChaosReport( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricChaosReportResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetStoppedNodeList( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetStoppedNodeList( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStartNodeTransition( 
            /* [in] */ const FABRIC_NODE_TRANSITION_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartNodeTransition( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginGetNodeTransitionProgress( 
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndGetNodeTransitionProgress( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricNodeTransitionProgressResult **result) = 0;
        
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

    typedef struct IFabricFaultAnalysisServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricFaultAnalysisService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricFaultAnalysisService * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartPartitionDataLoss )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_START_PARTITION_DATA_LOSS_DESCRIPTION *invokeDataLossDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartPartitionDataLoss )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetPartitionDataLossProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetPartitionDataLossProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionDataLossProgressResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartPartitionQuorumLoss )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_START_PARTITION_QUORUM_LOSS_DESCRIPTION *invokeQuorumLossDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartPartitionQuorumLoss )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetPartitionQuorumLossProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetPartitionQuorumLossProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionQuorumLossProgressResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartPartitionRestart )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_START_PARTITION_RESTART_DESCRIPTION *restartPartitionDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartPartitionRestart )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetPartitionRestartProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetPartitionRestartProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricPartitionRestartProgressResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetTestCommandStatusList )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_TEST_COMMAND_LIST_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetTestCommandStatusList )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricTestCommandStatusResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCancelTestCommand )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCancelTestCommand )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartChaos )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_START_CHAOS_DESCRIPTION *startChaosDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartChaos )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStopChaos )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStopChaos )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetChaosReport )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_GET_CHAOS_REPORT_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetChaosReport )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricChaosReportResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetStoppedNodeList )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetStoppedNodeList )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartNodeTransition )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ const FABRIC_NODE_TRANSITION_DESCRIPTION *description,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartNodeTransition )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginGetNodeTransitionProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ FABRIC_TEST_COMMAND_OPERATION_ID operationId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndGetNodeTransitionProgress )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricNodeTransitionProgressResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCallSystemService )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ LPCWSTR action,
            /* [in] */ LPCWSTR inputBlob,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCallSystemService )( 
            IFabricFaultAnalysisService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **outputBlob);
        
        END_INTERFACE
    } IFabricFaultAnalysisServiceVtbl;

    interface IFabricFaultAnalysisService
    {
        CONST_VTBL struct IFabricFaultAnalysisServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricFaultAnalysisService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricFaultAnalysisService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricFaultAnalysisService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricFaultAnalysisService_BeginStartPartitionDataLoss(This,invokeDataLossDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartPartitionDataLoss(This,invokeDataLossDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndStartPartitionDataLoss(This,context)	\
    ( (This)->lpVtbl -> EndStartPartitionDataLoss(This,context) ) 

#define IFabricFaultAnalysisService_BeginGetPartitionDataLossProgress(This,operationId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetPartitionDataLossProgress(This,operationId,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndGetPartitionDataLossProgress(This,context,result)	\
    ( (This)->lpVtbl -> EndGetPartitionDataLossProgress(This,context,result) ) 

#define IFabricFaultAnalysisService_BeginStartPartitionQuorumLoss(This,invokeQuorumLossDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartPartitionQuorumLoss(This,invokeQuorumLossDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndStartPartitionQuorumLoss(This,context)	\
    ( (This)->lpVtbl -> EndStartPartitionQuorumLoss(This,context) ) 

#define IFabricFaultAnalysisService_BeginGetPartitionQuorumLossProgress(This,operationId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetPartitionQuorumLossProgress(This,operationId,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndGetPartitionQuorumLossProgress(This,context,result)	\
    ( (This)->lpVtbl -> EndGetPartitionQuorumLossProgress(This,context,result) ) 

#define IFabricFaultAnalysisService_BeginStartPartitionRestart(This,restartPartitionDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartPartitionRestart(This,restartPartitionDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndStartPartitionRestart(This,context)	\
    ( (This)->lpVtbl -> EndStartPartitionRestart(This,context) ) 

#define IFabricFaultAnalysisService_BeginGetPartitionRestartProgress(This,operationId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetPartitionRestartProgress(This,operationId,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndGetPartitionRestartProgress(This,context,result)	\
    ( (This)->lpVtbl -> EndGetPartitionRestartProgress(This,context,result) ) 

#define IFabricFaultAnalysisService_BeginGetTestCommandStatusList(This,description,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetTestCommandStatusList(This,description,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndGetTestCommandStatusList(This,context,result)	\
    ( (This)->lpVtbl -> EndGetTestCommandStatusList(This,context,result) ) 

#define IFabricFaultAnalysisService_BeginCancelTestCommand(This,description,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginCancelTestCommand(This,description,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndCancelTestCommand(This,context)	\
    ( (This)->lpVtbl -> EndCancelTestCommand(This,context) ) 

#define IFabricFaultAnalysisService_BeginStartChaos(This,startChaosDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartChaos(This,startChaosDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndStartChaos(This,context)	\
    ( (This)->lpVtbl -> EndStartChaos(This,context) ) 

#define IFabricFaultAnalysisService_BeginStopChaos(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStopChaos(This,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndStopChaos(This,context)	\
    ( (This)->lpVtbl -> EndStopChaos(This,context) ) 

#define IFabricFaultAnalysisService_BeginGetChaosReport(This,description,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetChaosReport(This,description,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndGetChaosReport(This,context,result)	\
    ( (This)->lpVtbl -> EndGetChaosReport(This,context,result) ) 

#define IFabricFaultAnalysisService_BeginGetStoppedNodeList(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetStoppedNodeList(This,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndGetStoppedNodeList(This,context,result)	\
    ( (This)->lpVtbl -> EndGetStoppedNodeList(This,context,result) ) 

#define IFabricFaultAnalysisService_BeginStartNodeTransition(This,description,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartNodeTransition(This,description,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndStartNodeTransition(This,context)	\
    ( (This)->lpVtbl -> EndStartNodeTransition(This,context) ) 

#define IFabricFaultAnalysisService_BeginGetNodeTransitionProgress(This,operationId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginGetNodeTransitionProgress(This,operationId,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndGetNodeTransitionProgress(This,context,result)	\
    ( (This)->lpVtbl -> EndGetNodeTransitionProgress(This,context,result) ) 

#define IFabricFaultAnalysisService_BeginCallSystemService(This,action,inputBlob,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginCallSystemService(This,action,inputBlob,timeoutMilliseconds,callback,context) ) 

#define IFabricFaultAnalysisService_EndCallSystemService(This,context,outputBlob)	\
    ( (This)->lpVtbl -> EndCallSystemService(This,context,outputBlob) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricFaultAnalysisService_INTERFACE_DEFINED__ */


#ifndef __IFabricFaultAnalysisServiceAgent_INTERFACE_DEFINED__
#define __IFabricFaultAnalysisServiceAgent_INTERFACE_DEFINED__

/* interface IFabricFaultAnalysisServiceAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricFaultAnalysisServiceAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a202ba9d-1478-42a8-ad03-4a1f15c7d255")
    IFabricFaultAnalysisServiceAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterFaultAnalysisService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricFaultAnalysisServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricFaultAnalysisServiceAgent0001,
            /* [in] */ IFabricFaultAnalysisService *__MIDL__IFabricFaultAnalysisServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterFaultAnalysisService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricFaultAnalysisServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricFaultAnalysisServiceAgent0004) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricFaultAnalysisServiceAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricFaultAnalysisServiceAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricFaultAnalysisServiceAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricFaultAnalysisServiceAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterFaultAnalysisService )( 
            IFabricFaultAnalysisServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricFaultAnalysisServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricFaultAnalysisServiceAgent0001,
            /* [in] */ IFabricFaultAnalysisService *__MIDL__IFabricFaultAnalysisServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterFaultAnalysisService )( 
            IFabricFaultAnalysisServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricFaultAnalysisServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricFaultAnalysisServiceAgent0004);
        
        END_INTERFACE
    } IFabricFaultAnalysisServiceAgentVtbl;

    interface IFabricFaultAnalysisServiceAgent
    {
        CONST_VTBL struct IFabricFaultAnalysisServiceAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricFaultAnalysisServiceAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricFaultAnalysisServiceAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricFaultAnalysisServiceAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricFaultAnalysisServiceAgent_RegisterFaultAnalysisService(This,__MIDL__IFabricFaultAnalysisServiceAgent0000,__MIDL__IFabricFaultAnalysisServiceAgent0001,__MIDL__IFabricFaultAnalysisServiceAgent0002,serviceAddress)	\
    ( (This)->lpVtbl -> RegisterFaultAnalysisService(This,__MIDL__IFabricFaultAnalysisServiceAgent0000,__MIDL__IFabricFaultAnalysisServiceAgent0001,__MIDL__IFabricFaultAnalysisServiceAgent0002,serviceAddress) ) 

#define IFabricFaultAnalysisServiceAgent_UnregisterFaultAnalysisService(This,__MIDL__IFabricFaultAnalysisServiceAgent0003,__MIDL__IFabricFaultAnalysisServiceAgent0004)	\
    ( (This)->lpVtbl -> UnregisterFaultAnalysisService(This,__MIDL__IFabricFaultAnalysisServiceAgent0003,__MIDL__IFabricFaultAnalysisServiceAgent0004) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricFaultAnalysisServiceAgent_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricFaultAnalysisServiceAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("bbea0807-9fc1-4ef4-81e2-99516511564d")
FabricFaultAnalysisServiceAgent;
#endif


#ifndef __FabricFaultAnalysisService_MODULE_DEFINED__
#define __FabricFaultAnalysisService_MODULE_DEFINED__


/* module FabricFaultAnalysisService */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricFaultAnalysisServiceAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricFaultAnalysisServiceAgent);

#endif /* __FabricFaultAnalysisService_MODULE_DEFINED__ */
#endif /* __FabricFaultAnalysisServiceLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


