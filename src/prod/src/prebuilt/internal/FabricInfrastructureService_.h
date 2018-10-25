

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

#ifndef __fabricinfrastructureservice__h__
#define __fabricinfrastructureservice__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricInfrastructureService_FWD_DEFINED__
#define __IFabricInfrastructureService_FWD_DEFINED__
typedef interface IFabricInfrastructureService IFabricInfrastructureService;

#endif 	/* __IFabricInfrastructureService_FWD_DEFINED__ */


#ifndef __IFabricInfrastructureServiceAgent_FWD_DEFINED__
#define __IFabricInfrastructureServiceAgent_FWD_DEFINED__
typedef interface IFabricInfrastructureServiceAgent IFabricInfrastructureServiceAgent;

#endif 	/* __IFabricInfrastructureServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricInfrastructureTaskQueryResult_FWD_DEFINED__
#define __IFabricInfrastructureTaskQueryResult_FWD_DEFINED__
typedef interface IFabricInfrastructureTaskQueryResult IFabricInfrastructureTaskQueryResult;

#endif 	/* __IFabricInfrastructureTaskQueryResult_FWD_DEFINED__ */


#ifndef __FabricInfrastructureServiceAgent_FWD_DEFINED__
#define __FabricInfrastructureServiceAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricInfrastructureServiceAgent FabricInfrastructureServiceAgent;
#else
typedef struct FabricInfrastructureServiceAgent FabricInfrastructureServiceAgent;
#endif /* __cplusplus */

#endif 	/* __FabricInfrastructureServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricInfrastructureService_FWD_DEFINED__
#define __IFabricInfrastructureService_FWD_DEFINED__
typedef interface IFabricInfrastructureService IFabricInfrastructureService;

#endif 	/* __IFabricInfrastructureService_FWD_DEFINED__ */


#ifndef __IFabricInfrastructureServiceAgent_FWD_DEFINED__
#define __IFabricInfrastructureServiceAgent_FWD_DEFINED__
typedef interface IFabricInfrastructureServiceAgent IFabricInfrastructureServiceAgent;

#endif 	/* __IFabricInfrastructureServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricInfrastructureTaskQueryResult_FWD_DEFINED__
#define __IFabricInfrastructureTaskQueryResult_FWD_DEFINED__
typedef interface IFabricInfrastructureTaskQueryResult IFabricInfrastructureTaskQueryResult;

#endif 	/* __IFabricInfrastructureTaskQueryResult_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"
#include "FabricRuntime.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricinfrastructureservice__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif





extern RPC_IF_HANDLE __MIDL_itf_fabricinfrastructureservice__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricinfrastructureservice__0000_0000_v0_0_s_ifspec;


#ifndef __FabricInfrastructureServiceLib_LIBRARY_DEFINED__
#define __FabricInfrastructureServiceLib_LIBRARY_DEFINED__

/* library FabricInfrastructureServiceLib */
/* [version][uuid] */ 


#pragma pack(push, 8)




#pragma pack(pop)

EXTERN_C const IID LIBID_FabricInfrastructureServiceLib;

#ifndef __IFabricInfrastructureService_INTERFACE_DEFINED__
#define __IFabricInfrastructureService_INTERFACE_DEFINED__

/* interface IFabricInfrastructureService */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInfrastructureService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d948b384-fd16-48f3-a4ad-d6e68c6bf2bb")
    IFabricInfrastructureService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRunCommand( 
            /* [in] */ BOOLEAN isAdminCommand,
            /* [in] */ LPCWSTR command,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRunCommand( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportStartTaskSuccess( 
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportStartTaskSuccess( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportFinishTaskSuccess( 
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportFinishTaskSuccess( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReportTaskFailure( 
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReportTaskFailure( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInfrastructureServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInfrastructureService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInfrastructureService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInfrastructureService * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRunCommand )( 
            IFabricInfrastructureService * This,
            /* [in] */ BOOLEAN isAdminCommand,
            /* [in] */ LPCWSTR command,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRunCommand )( 
            IFabricInfrastructureService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportStartTaskSuccess )( 
            IFabricInfrastructureService * This,
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportStartTaskSuccess )( 
            IFabricInfrastructureService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportFinishTaskSuccess )( 
            IFabricInfrastructureService * This,
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportFinishTaskSuccess )( 
            IFabricInfrastructureService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReportTaskFailure )( 
            IFabricInfrastructureService * This,
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReportTaskFailure )( 
            IFabricInfrastructureService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricInfrastructureServiceVtbl;

    interface IFabricInfrastructureService
    {
        CONST_VTBL struct IFabricInfrastructureServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInfrastructureService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInfrastructureService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInfrastructureService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInfrastructureService_BeginRunCommand(This,isAdminCommand,command,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRunCommand(This,isAdminCommand,command,timeoutMilliseconds,callback,context) ) 

#define IFabricInfrastructureService_EndRunCommand(This,context,result)	\
    ( (This)->lpVtbl -> EndRunCommand(This,context,result) ) 

#define IFabricInfrastructureService_BeginReportStartTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportStartTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IFabricInfrastructureService_EndReportStartTaskSuccess(This,context)	\
    ( (This)->lpVtbl -> EndReportStartTaskSuccess(This,context) ) 

#define IFabricInfrastructureService_BeginReportFinishTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportFinishTaskSuccess(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IFabricInfrastructureService_EndReportFinishTaskSuccess(This,context)	\
    ( (This)->lpVtbl -> EndReportFinishTaskSuccess(This,context) ) 

#define IFabricInfrastructureService_BeginReportTaskFailure(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginReportTaskFailure(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IFabricInfrastructureService_EndReportTaskFailure(This,context)	\
    ( (This)->lpVtbl -> EndReportTaskFailure(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInfrastructureService_INTERFACE_DEFINED__ */


#ifndef __IFabricInfrastructureServiceAgent_INTERFACE_DEFINED__
#define __IFabricInfrastructureServiceAgent_INTERFACE_DEFINED__

/* interface IFabricInfrastructureServiceAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInfrastructureServiceAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2416a4e2-9313-42ce-93c9-b499764840ce")
    IFabricInfrastructureServiceAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterInfrastructureServiceFactory( 
            /* [in] */ IFabricStatefulServiceFactory *factory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterInfrastructureService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricInfrastructureServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricInfrastructureServiceAgent0001,
            /* [in] */ IFabricInfrastructureService *__MIDL__IFabricInfrastructureServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterInfrastructureService( 
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricInfrastructureServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricInfrastructureServiceAgent0004) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginStartInfrastructureTask( 
            /* [in] */ FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION *taskDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndStartInfrastructureTask( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginFinishInfrastructureTask( 
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndFinishInfrastructureTask( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginQueryInfrastructureTask( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndQueryInfrastructureTask( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricInfrastructureTaskQueryResult **queryResult) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInfrastructureServiceAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInfrastructureServiceAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInfrastructureServiceAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterInfrastructureServiceFactory )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ IFabricStatefulServiceFactory *factory);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterInfrastructureService )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricInfrastructureServiceAgent0000,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricInfrastructureServiceAgent0001,
            /* [in] */ IFabricInfrastructureService *__MIDL__IFabricInfrastructureServiceAgent0002,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterInfrastructureService )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ FABRIC_PARTITION_ID __MIDL__IFabricInfrastructureServiceAgent0003,
            /* [in] */ FABRIC_REPLICA_ID __MIDL__IFabricInfrastructureServiceAgent0004);
        
        HRESULT ( STDMETHODCALLTYPE *BeginStartInfrastructureTask )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION *taskDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndStartInfrastructureTask )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginFinishInfrastructureTask )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ LPCWSTR taskId,
            /* [in] */ ULONGLONG instanceId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndFinishInfrastructureTask )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginQueryInfrastructureTask )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndQueryInfrastructureTask )( 
            IFabricInfrastructureServiceAgent * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricInfrastructureTaskQueryResult **queryResult);
        
        END_INTERFACE
    } IFabricInfrastructureServiceAgentVtbl;

    interface IFabricInfrastructureServiceAgent
    {
        CONST_VTBL struct IFabricInfrastructureServiceAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInfrastructureServiceAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInfrastructureServiceAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInfrastructureServiceAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInfrastructureServiceAgent_RegisterInfrastructureServiceFactory(This,factory)	\
    ( (This)->lpVtbl -> RegisterInfrastructureServiceFactory(This,factory) ) 

#define IFabricInfrastructureServiceAgent_RegisterInfrastructureService(This,__MIDL__IFabricInfrastructureServiceAgent0000,__MIDL__IFabricInfrastructureServiceAgent0001,__MIDL__IFabricInfrastructureServiceAgent0002,serviceAddress)	\
    ( (This)->lpVtbl -> RegisterInfrastructureService(This,__MIDL__IFabricInfrastructureServiceAgent0000,__MIDL__IFabricInfrastructureServiceAgent0001,__MIDL__IFabricInfrastructureServiceAgent0002,serviceAddress) ) 

#define IFabricInfrastructureServiceAgent_UnregisterInfrastructureService(This,__MIDL__IFabricInfrastructureServiceAgent0003,__MIDL__IFabricInfrastructureServiceAgent0004)	\
    ( (This)->lpVtbl -> UnregisterInfrastructureService(This,__MIDL__IFabricInfrastructureServiceAgent0003,__MIDL__IFabricInfrastructureServiceAgent0004) ) 

#define IFabricInfrastructureServiceAgent_BeginStartInfrastructureTask(This,taskDescription,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginStartInfrastructureTask(This,taskDescription,timeoutMilliseconds,callback,context) ) 

#define IFabricInfrastructureServiceAgent_EndStartInfrastructureTask(This,context)	\
    ( (This)->lpVtbl -> EndStartInfrastructureTask(This,context) ) 

#define IFabricInfrastructureServiceAgent_BeginFinishInfrastructureTask(This,taskId,instanceId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginFinishInfrastructureTask(This,taskId,instanceId,timeoutMilliseconds,callback,context) ) 

#define IFabricInfrastructureServiceAgent_EndFinishInfrastructureTask(This,context)	\
    ( (This)->lpVtbl -> EndFinishInfrastructureTask(This,context) ) 

#define IFabricInfrastructureServiceAgent_BeginQueryInfrastructureTask(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginQueryInfrastructureTask(This,timeoutMilliseconds,callback,context) ) 

#define IFabricInfrastructureServiceAgent_EndQueryInfrastructureTask(This,context,queryResult)	\
    ( (This)->lpVtbl -> EndQueryInfrastructureTask(This,context,queryResult) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInfrastructureServiceAgent_INTERFACE_DEFINED__ */


#ifndef __IFabricInfrastructureTaskQueryResult_INTERFACE_DEFINED__
#define __IFabricInfrastructureTaskQueryResult_INTERFACE_DEFINED__

/* interface IFabricInfrastructureTaskQueryResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricInfrastructureTaskQueryResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("209e28bb-4f8b-4f03-88c9-7b54f2cd29f9")
    IFabricInfrastructureTaskQueryResult : public IUnknown
    {
    public:
        virtual const FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricInfrastructureTaskQueryResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricInfrastructureTaskQueryResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricInfrastructureTaskQueryResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricInfrastructureTaskQueryResult * This);
        
        const FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_LIST *( STDMETHODCALLTYPE *get_Result )( 
            IFabricInfrastructureTaskQueryResult * This);
        
        END_INTERFACE
    } IFabricInfrastructureTaskQueryResultVtbl;

    interface IFabricInfrastructureTaskQueryResult
    {
        CONST_VTBL struct IFabricInfrastructureTaskQueryResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricInfrastructureTaskQueryResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricInfrastructureTaskQueryResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricInfrastructureTaskQueryResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricInfrastructureTaskQueryResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricInfrastructureTaskQueryResult_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricInfrastructureServiceAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("8400d67e-7f19-45f3-bec2-b137da42106a")
FabricInfrastructureServiceAgent;
#endif


#ifndef __FabricInfrastructureService_MODULE_DEFINED__
#define __FabricInfrastructureService_MODULE_DEFINED__


/* module FabricInfrastructureService */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricInfrastructureServiceAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricInfrastructureServiceAgent);

#endif /* __FabricInfrastructureService_MODULE_DEFINED__ */
#endif /* __FabricInfrastructureServiceLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


