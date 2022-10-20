

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

#ifndef __FabricContainerActivatorService__h__
#define __FabricContainerActivatorService__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricContainerActivatorService_FWD_DEFINED__
#define __IFabricContainerActivatorService_FWD_DEFINED__
typedef interface IFabricContainerActivatorService IFabricContainerActivatorService;

#endif 	/* __IFabricContainerActivatorService_FWD_DEFINED__ */


#ifndef __IFabricContainerActivatorService2_FWD_DEFINED__
#define __IFabricContainerActivatorService2_FWD_DEFINED__
typedef interface IFabricContainerActivatorService2 IFabricContainerActivatorService2;

#endif 	/* __IFabricContainerActivatorService2_FWD_DEFINED__ */


#ifndef __IFabricContainerActivatorServiceAgent_FWD_DEFINED__
#define __IFabricContainerActivatorServiceAgent_FWD_DEFINED__
typedef interface IFabricContainerActivatorServiceAgent IFabricContainerActivatorServiceAgent;

#endif 	/* __IFabricContainerActivatorServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricHostingSettingsResult_FWD_DEFINED__
#define __IFabricHostingSettingsResult_FWD_DEFINED__
typedef interface IFabricHostingSettingsResult IFabricHostingSettingsResult;

#endif 	/* __IFabricHostingSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricContainerApiExecutionResult_FWD_DEFINED__
#define __IFabricContainerApiExecutionResult_FWD_DEFINED__
typedef interface IFabricContainerApiExecutionResult IFabricContainerApiExecutionResult;

#endif 	/* __IFabricContainerApiExecutionResult_FWD_DEFINED__ */


#ifndef __FabricContainerActivatorServiceAgent_FWD_DEFINED__
#define __FabricContainerActivatorServiceAgent_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricContainerActivatorServiceAgent FabricContainerActivatorServiceAgent;
#else
typedef struct FabricContainerActivatorServiceAgent FabricContainerActivatorServiceAgent;
#endif /* __cplusplus */

#endif 	/* __FabricContainerActivatorServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricContainerActivatorService_FWD_DEFINED__
#define __IFabricContainerActivatorService_FWD_DEFINED__
typedef interface IFabricContainerActivatorService IFabricContainerActivatorService;

#endif 	/* __IFabricContainerActivatorService_FWD_DEFINED__ */


#ifndef __IFabricContainerActivatorService2_FWD_DEFINED__
#define __IFabricContainerActivatorService2_FWD_DEFINED__
typedef interface IFabricContainerActivatorService2 IFabricContainerActivatorService2;

#endif 	/* __IFabricContainerActivatorService2_FWD_DEFINED__ */


#ifndef __IFabricContainerActivatorServiceAgent_FWD_DEFINED__
#define __IFabricContainerActivatorServiceAgent_FWD_DEFINED__
typedef interface IFabricContainerActivatorServiceAgent IFabricContainerActivatorServiceAgent;

#endif 	/* __IFabricContainerActivatorServiceAgent_FWD_DEFINED__ */


#ifndef __IFabricContainerActivatorServiceAgent2_FWD_DEFINED__
#define __IFabricContainerActivatorServiceAgent2_FWD_DEFINED__
typedef interface IFabricContainerActivatorServiceAgent2 IFabricContainerActivatorServiceAgent2;

#endif 	/* __IFabricContainerActivatorServiceAgent2_FWD_DEFINED__ */


#ifndef __IFabricHostingSettingsResult_FWD_DEFINED__
#define __IFabricHostingSettingsResult_FWD_DEFINED__
typedef interface IFabricHostingSettingsResult IFabricHostingSettingsResult;

#endif 	/* __IFabricHostingSettingsResult_FWD_DEFINED__ */


#ifndef __IFabricContainerApiExecutionResult_FWD_DEFINED__
#define __IFabricContainerApiExecutionResult_FWD_DEFINED__
typedef interface IFabricContainerApiExecutionResult IFabricContainerApiExecutionResult;

#endif 	/* __IFabricContainerApiExecutionResult_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricCommon_.h"
#include "FabricRuntime.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_FabricContainerActivatorService__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif







extern RPC_IF_HANDLE __MIDL_itf_FabricContainerActivatorService__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_FabricContainerActivatorService__0000_0000_v0_0_s_ifspec;


#ifndef __FabricContainerActivatorServiceLib_LIBRARY_DEFINED__
#define __FabricContainerActivatorServiceLib_LIBRARY_DEFINED__

/* library FabricContainerActivatorServiceLib */
/* [version][uuid] */ 


#pragma pack(push, 8)






#pragma pack(pop)

EXTERN_C const IID LIBID_FabricContainerActivatorServiceLib;

#ifndef __IFabricContainerActivatorService_INTERFACE_DEFINED__
#define __IFabricContainerActivatorService_INTERFACE_DEFINED__

/* interface IFabricContainerActivatorService */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricContainerActivatorService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("111d2880-5b3e-48b3-895c-c62036830a14")
    IFabricContainerActivatorService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StartEventMonitoring( 
            /* [in] */ BOOLEAN isContainerServiceManaged,
            /* [in] */ ULONGLONG sinceTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginActivateContainer( 
            /* [in] */ FABRIC_CONTAINER_ACTIVATION_ARGS *activationParams,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndActivateContainer( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeactivateContainer( 
            /* [in] */ FABRIC_CONTAINER_DEACTIVATION_ARGS *deactivationParams,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeactivateContainer( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDownloadImages( 
            /* [in] */ FABRIC_CONTAINER_IMAGE_DESCRIPTION_LIST *images,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDownloadImages( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeleteImages( 
            /* [in] */ FABRIC_STRING_LIST *images,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeleteImages( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginInvokeContainerApi( 
            /* [in] */ FABRIC_CONTAINER_API_EXECUTION_ARGS *apiExecArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndInvokeContainerApi( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricContainerApiExecutionResult **result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricContainerActivatorServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricContainerActivatorService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricContainerActivatorService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricContainerActivatorService * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartEventMonitoring )( 
            IFabricContainerActivatorService * This,
            /* [in] */ BOOLEAN isContainerServiceManaged,
            /* [in] */ ULONGLONG sinceTime);
        
        HRESULT ( STDMETHODCALLTYPE *BeginActivateContainer )( 
            IFabricContainerActivatorService * This,
            /* [in] */ FABRIC_CONTAINER_ACTIVATION_ARGS *activationParams,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndActivateContainer )( 
            IFabricContainerActivatorService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeactivateContainer )( 
            IFabricContainerActivatorService * This,
            /* [in] */ FABRIC_CONTAINER_DEACTIVATION_ARGS *deactivationParams,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeactivateContainer )( 
            IFabricContainerActivatorService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDownloadImages )( 
            IFabricContainerActivatorService * This,
            /* [in] */ FABRIC_CONTAINER_IMAGE_DESCRIPTION_LIST *images,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDownloadImages )( 
            IFabricContainerActivatorService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteImages )( 
            IFabricContainerActivatorService * This,
            /* [in] */ FABRIC_STRING_LIST *images,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteImages )( 
            IFabricContainerActivatorService * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginInvokeContainerApi )( 
            IFabricContainerActivatorService * This,
            /* [in] */ FABRIC_CONTAINER_API_EXECUTION_ARGS *apiExecArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndInvokeContainerApi )( 
            IFabricContainerActivatorService * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricContainerApiExecutionResult **result);
        
        END_INTERFACE
    } IFabricContainerActivatorServiceVtbl;

    interface IFabricContainerActivatorService
    {
        CONST_VTBL struct IFabricContainerActivatorServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricContainerActivatorService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricContainerActivatorService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricContainerActivatorService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricContainerActivatorService_StartEventMonitoring(This,isContainerServiceManaged,sinceTime)	\
    ( (This)->lpVtbl -> StartEventMonitoring(This,isContainerServiceManaged,sinceTime) ) 

#define IFabricContainerActivatorService_BeginActivateContainer(This,activationParams,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginActivateContainer(This,activationParams,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService_EndActivateContainer(This,context,result)	\
    ( (This)->lpVtbl -> EndActivateContainer(This,context,result) ) 

#define IFabricContainerActivatorService_BeginDeactivateContainer(This,deactivationParams,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeactivateContainer(This,deactivationParams,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService_EndDeactivateContainer(This,context)	\
    ( (This)->lpVtbl -> EndDeactivateContainer(This,context) ) 

#define IFabricContainerActivatorService_BeginDownloadImages(This,images,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDownloadImages(This,images,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService_EndDownloadImages(This,context)	\
    ( (This)->lpVtbl -> EndDownloadImages(This,context) ) 

#define IFabricContainerActivatorService_BeginDeleteImages(This,images,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteImages(This,images,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService_EndDeleteImages(This,context)	\
    ( (This)->lpVtbl -> EndDeleteImages(This,context) ) 

#define IFabricContainerActivatorService_BeginInvokeContainerApi(This,apiExecArgs,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginInvokeContainerApi(This,apiExecArgs,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService_EndInvokeContainerApi(This,context,result)	\
    ( (This)->lpVtbl -> EndInvokeContainerApi(This,context,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricContainerActivatorService_INTERFACE_DEFINED__ */


#ifndef __IFabricContainerActivatorService2_INTERFACE_DEFINED__
#define __IFabricContainerActivatorService2_INTERFACE_DEFINED__

/* interface IFabricContainerActivatorService2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricContainerActivatorService2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8fdba659-674c-4464-ac64-21d410313b96")
    IFabricContainerActivatorService2 : public IFabricContainerActivatorService
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginContainerUpdateRoutes( 
            /* [in] */ FABRIC_CONTAINER_UPDATE_ROUTE_ARGS *updateExecArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndContainerUpdateRoutes( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricContainerActivatorService2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricContainerActivatorService2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricContainerActivatorService2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartEventMonitoring )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ BOOLEAN isContainerServiceManaged,
            /* [in] */ ULONGLONG sinceTime);
        
        HRESULT ( STDMETHODCALLTYPE *BeginActivateContainer )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ FABRIC_CONTAINER_ACTIVATION_ARGS *activationParams,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndActivateContainer )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeactivateContainer )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ FABRIC_CONTAINER_DEACTIVATION_ARGS *deactivationParams,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeactivateContainer )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDownloadImages )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ FABRIC_CONTAINER_IMAGE_DESCRIPTION_LIST *images,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDownloadImages )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteImages )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ FABRIC_STRING_LIST *images,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteImages )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginInvokeContainerApi )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ FABRIC_CONTAINER_API_EXECUTION_ARGS *apiExecArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndInvokeContainerApi )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricContainerApiExecutionResult **result);
        
        HRESULT ( STDMETHODCALLTYPE *BeginContainerUpdateRoutes )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ FABRIC_CONTAINER_UPDATE_ROUTE_ARGS *updateExecArgs,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndContainerUpdateRoutes )( 
            IFabricContainerActivatorService2 * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricContainerActivatorService2Vtbl;

    interface IFabricContainerActivatorService2
    {
        CONST_VTBL struct IFabricContainerActivatorService2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricContainerActivatorService2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricContainerActivatorService2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricContainerActivatorService2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricContainerActivatorService2_StartEventMonitoring(This,isContainerServiceManaged,sinceTime)	\
    ( (This)->lpVtbl -> StartEventMonitoring(This,isContainerServiceManaged,sinceTime) ) 

#define IFabricContainerActivatorService2_BeginActivateContainer(This,activationParams,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginActivateContainer(This,activationParams,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService2_EndActivateContainer(This,context,result)	\
    ( (This)->lpVtbl -> EndActivateContainer(This,context,result) ) 

#define IFabricContainerActivatorService2_BeginDeactivateContainer(This,deactivationParams,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeactivateContainer(This,deactivationParams,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService2_EndDeactivateContainer(This,context)	\
    ( (This)->lpVtbl -> EndDeactivateContainer(This,context) ) 

#define IFabricContainerActivatorService2_BeginDownloadImages(This,images,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDownloadImages(This,images,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService2_EndDownloadImages(This,context)	\
    ( (This)->lpVtbl -> EndDownloadImages(This,context) ) 

#define IFabricContainerActivatorService2_BeginDeleteImages(This,images,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteImages(This,images,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService2_EndDeleteImages(This,context)	\
    ( (This)->lpVtbl -> EndDeleteImages(This,context) ) 

#define IFabricContainerActivatorService2_BeginInvokeContainerApi(This,apiExecArgs,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginInvokeContainerApi(This,apiExecArgs,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService2_EndInvokeContainerApi(This,context,result)	\
    ( (This)->lpVtbl -> EndInvokeContainerApi(This,context,result) ) 


#define IFabricContainerActivatorService2_BeginContainerUpdateRoutes(This,updateExecArgs,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginContainerUpdateRoutes(This,updateExecArgs,timeoutMilliseconds,callback,context) ) 

#define IFabricContainerActivatorService2_EndContainerUpdateRoutes(This,context)	\
    ( (This)->lpVtbl -> EndContainerUpdateRoutes(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricContainerActivatorService2_INTERFACE_DEFINED__ */


#ifndef __IFabricContainerActivatorServiceAgent_INTERFACE_DEFINED__
#define __IFabricContainerActivatorServiceAgent_INTERFACE_DEFINED__

/* interface IFabricContainerActivatorServiceAgent */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricContainerActivatorServiceAgent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b40b7396-5d2a-471a-b6bc-6cfad1cb2061")
    IFabricContainerActivatorServiceAgent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ProcessContainerEvents( 
            /* [in] */ FABRIC_CONTAINER_EVENT_NOTIFICATION *notifiction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterContainerActivatorService( 
            /* [in] */ IFabricContainerActivatorService *activatorService) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricContainerActivatorServiceAgentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricContainerActivatorServiceAgent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricContainerActivatorServiceAgent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricContainerActivatorServiceAgent * This);
        
        HRESULT ( STDMETHODCALLTYPE *ProcessContainerEvents )( 
            IFabricContainerActivatorServiceAgent * This,
            /* [in] */ FABRIC_CONTAINER_EVENT_NOTIFICATION *notifiction);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterContainerActivatorService )( 
            IFabricContainerActivatorServiceAgent * This,
            /* [in] */ IFabricContainerActivatorService *activatorService);
        
        END_INTERFACE
    } IFabricContainerActivatorServiceAgentVtbl;

    interface IFabricContainerActivatorServiceAgent
    {
        CONST_VTBL struct IFabricContainerActivatorServiceAgentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricContainerActivatorServiceAgent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricContainerActivatorServiceAgent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricContainerActivatorServiceAgent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricContainerActivatorServiceAgent_ProcessContainerEvents(This,notifiction)	\
    ( (This)->lpVtbl -> ProcessContainerEvents(This,notifiction) ) 

#define IFabricContainerActivatorServiceAgent_RegisterContainerActivatorService(This,activatorService)	\
    ( (This)->lpVtbl -> RegisterContainerActivatorService(This,activatorService) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricContainerActivatorServiceAgent_INTERFACE_DEFINED__ */


#ifndef __IFabricHostingSettingsResult_INTERFACE_DEFINED__
#define __IFabricHostingSettingsResult_INTERFACE_DEFINED__

/* interface IFabricHostingSettingsResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricHostingSettingsResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("24e88e38-deb8-4bd9-9b55-ca67d6134850")
    IFabricHostingSettingsResult : public IUnknown
    {
    public:
        virtual const FABRIC_HOSTING_SETTINGS *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricHostingSettingsResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricHostingSettingsResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricHostingSettingsResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricHostingSettingsResult * This);
        
        const FABRIC_HOSTING_SETTINGS *( STDMETHODCALLTYPE *get_Result )( 
            IFabricHostingSettingsResult * This);
        
        END_INTERFACE
    } IFabricHostingSettingsResultVtbl;

    interface IFabricHostingSettingsResult
    {
        CONST_VTBL struct IFabricHostingSettingsResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricHostingSettingsResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricHostingSettingsResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricHostingSettingsResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricHostingSettingsResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricHostingSettingsResult_INTERFACE_DEFINED__ */


#ifndef __IFabricContainerApiExecutionResult_INTERFACE_DEFINED__
#define __IFabricContainerApiExecutionResult_INTERFACE_DEFINED__

/* interface IFabricContainerApiExecutionResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricContainerApiExecutionResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c40d5451-32c4-481a-b340-a9e771cc6937")
    IFabricContainerApiExecutionResult : public IUnknown
    {
    public:
        virtual const FABRIC_CONTAINER_API_EXECUTION_RESPONSE *STDMETHODCALLTYPE get_Result( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricContainerApiExecutionResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricContainerApiExecutionResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricContainerApiExecutionResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricContainerApiExecutionResult * This);
        
        const FABRIC_CONTAINER_API_EXECUTION_RESPONSE *( STDMETHODCALLTYPE *get_Result )( 
            IFabricContainerApiExecutionResult * This);
        
        END_INTERFACE
    } IFabricContainerApiExecutionResultVtbl;

    interface IFabricContainerApiExecutionResult
    {
        CONST_VTBL struct IFabricContainerApiExecutionResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricContainerApiExecutionResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricContainerApiExecutionResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricContainerApiExecutionResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricContainerApiExecutionResult_get_Result(This)	\
    ( (This)->lpVtbl -> get_Result(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricContainerApiExecutionResult_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricContainerActivatorServiceAgent;

#ifdef __cplusplus

class DECLSPEC_UUID("16c96a7a-a6d2-481c-b057-678ad8fa105e")
FabricContainerActivatorServiceAgent;
#endif


#ifndef __FabricContainerActivatorService_MODULE_DEFINED__
#define __FabricContainerActivatorService_MODULE_DEFINED__


/* module FabricContainerActivatorService */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricContainerActivatorServiceAgent( 
    /* [in] */ __RPC__in REFIID riid,
    /* [retval][out] */ __RPC__deref_out_opt void **fabricContainerActivatorServiceAgent);

/* [entry] */ HRESULT LoadHostingSettings( 
    /* [retval][out] */ __RPC__deref_out_opt void **fabricHostingSettings);

#endif /* __FabricContainerActivatorService_MODULE_DEFINED__ */
#endif /* __FabricContainerActivatorServiceLib_LIBRARY_DEFINED__ */

#ifndef __IFabricContainerActivatorServiceAgent2_INTERFACE_DEFINED__
#define __IFabricContainerActivatorServiceAgent2_INTERFACE_DEFINED__

/* interface IFabricContainerActivatorServiceAgent2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricContainerActivatorServiceAgent2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ac2bcdde-3987-4bf0-ac91-989948faac85")
    IFabricContainerActivatorServiceAgent2 : public IFabricContainerActivatorServiceAgent
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterContainerActivatorService( 
            /* [in] */ IFabricContainerActivatorService2 *activatorService) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricContainerActivatorServiceAgent2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricContainerActivatorServiceAgent2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricContainerActivatorServiceAgent2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricContainerActivatorServiceAgent2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ProcessContainerEvents )( 
            IFabricContainerActivatorServiceAgent2 * This,
            /* [in] */ FABRIC_CONTAINER_EVENT_NOTIFICATION *notifiction);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterContainerActivatorService )( 
            IFabricContainerActivatorServiceAgent2 * This,
            /* [in] */ IFabricContainerActivatorService *activatorService);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterContainerActivatorService )( 
            IFabricContainerActivatorServiceAgent2 * This,
            /* [in] */ IFabricContainerActivatorService2 *activatorService);
        
        END_INTERFACE
    } IFabricContainerActivatorServiceAgent2Vtbl;

    interface IFabricContainerActivatorServiceAgent2
    {
        CONST_VTBL struct IFabricContainerActivatorServiceAgent2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricContainerActivatorServiceAgent2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricContainerActivatorServiceAgent2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricContainerActivatorServiceAgent2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricContainerActivatorServiceAgent2_ProcessContainerEvents(This,notifiction)	\
    ( (This)->lpVtbl -> ProcessContainerEvents(This,notifiction) ) 

#define IFabricContainerActivatorServiceAgent2_RegisterContainerActivatorService(This,activatorService)	\
    ( (This)->lpVtbl -> RegisterContainerActivatorService(This,activatorService) ) 


#define IFabricContainerActivatorServiceAgent2_RegisterContainerActivatorService(This,activatorService)	\
    ( (This)->lpVtbl -> RegisterContainerActivatorService(This,activatorService) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricContainerActivatorServiceAgent2_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


