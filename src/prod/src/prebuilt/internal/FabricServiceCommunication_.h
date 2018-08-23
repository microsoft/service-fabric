

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

#ifndef __fabricservicecommunication__h__
#define __fabricservicecommunication__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricCommunicationMessageSender_FWD_DEFINED__
#define __IFabricCommunicationMessageSender_FWD_DEFINED__
typedef interface IFabricCommunicationMessageSender IFabricCommunicationMessageSender;

#endif 	/* __IFabricCommunicationMessageSender_FWD_DEFINED__ */


#ifndef __IFabricServiceConnectionHandler_FWD_DEFINED__
#define __IFabricServiceConnectionHandler_FWD_DEFINED__
typedef interface IFabricServiceConnectionHandler IFabricServiceConnectionHandler;

#endif 	/* __IFabricServiceConnectionHandler_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationClient_FWD_DEFINED__
#define __IFabricServiceCommunicationClient_FWD_DEFINED__
typedef interface IFabricServiceCommunicationClient IFabricServiceCommunicationClient;

#endif 	/* __IFabricServiceCommunicationClient_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationClient2_FWD_DEFINED__
#define __IFabricServiceCommunicationClient2_FWD_DEFINED__
typedef interface IFabricServiceCommunicationClient2 IFabricServiceCommunicationClient2;

#endif 	/* __IFabricServiceCommunicationClient2_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationListener_FWD_DEFINED__
#define __IFabricServiceCommunicationListener_FWD_DEFINED__
typedef interface IFabricServiceCommunicationListener IFabricServiceCommunicationListener;

#endif 	/* __IFabricServiceCommunicationListener_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationMessage_FWD_DEFINED__
#define __IFabricServiceCommunicationMessage_FWD_DEFINED__
typedef interface IFabricServiceCommunicationMessage IFabricServiceCommunicationMessage;

#endif 	/* __IFabricServiceCommunicationMessage_FWD_DEFINED__ */


#ifndef __IFabricClientConnection_FWD_DEFINED__
#define __IFabricClientConnection_FWD_DEFINED__
typedef interface IFabricClientConnection IFabricClientConnection;

#endif 	/* __IFabricClientConnection_FWD_DEFINED__ */


#ifndef __IFabricServiceConnectionEventHandler_FWD_DEFINED__
#define __IFabricServiceConnectionEventHandler_FWD_DEFINED__
typedef interface IFabricServiceConnectionEventHandler IFabricServiceConnectionEventHandler;

#endif 	/* __IFabricServiceConnectionEventHandler_FWD_DEFINED__ */


#ifndef __IFabricCommunicationMessageHandler_FWD_DEFINED__
#define __IFabricCommunicationMessageHandler_FWD_DEFINED__
typedef interface IFabricCommunicationMessageHandler IFabricCommunicationMessageHandler;

#endif 	/* __IFabricCommunicationMessageHandler_FWD_DEFINED__ */


#ifndef __IFabricCommunicationMessageSender_FWD_DEFINED__
#define __IFabricCommunicationMessageSender_FWD_DEFINED__
typedef interface IFabricCommunicationMessageSender IFabricCommunicationMessageSender;

#endif 	/* __IFabricCommunicationMessageSender_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationMessage_FWD_DEFINED__
#define __IFabricServiceCommunicationMessage_FWD_DEFINED__
typedef interface IFabricServiceCommunicationMessage IFabricServiceCommunicationMessage;

#endif 	/* __IFabricServiceCommunicationMessage_FWD_DEFINED__ */


#ifndef __IFabricCommunicationMessageHandler_FWD_DEFINED__
#define __IFabricCommunicationMessageHandler_FWD_DEFINED__
typedef interface IFabricCommunicationMessageHandler IFabricCommunicationMessageHandler;

#endif 	/* __IFabricCommunicationMessageHandler_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationListener_FWD_DEFINED__
#define __IFabricServiceCommunicationListener_FWD_DEFINED__
typedef interface IFabricServiceCommunicationListener IFabricServiceCommunicationListener;

#endif 	/* __IFabricServiceCommunicationListener_FWD_DEFINED__ */


#ifndef __IFabricClientConnection_FWD_DEFINED__
#define __IFabricClientConnection_FWD_DEFINED__
typedef interface IFabricClientConnection IFabricClientConnection;

#endif 	/* __IFabricClientConnection_FWD_DEFINED__ */


#ifndef __IFabricServiceConnectionHandler_FWD_DEFINED__
#define __IFabricServiceConnectionHandler_FWD_DEFINED__
typedef interface IFabricServiceConnectionHandler IFabricServiceConnectionHandler;

#endif 	/* __IFabricServiceConnectionHandler_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationClient_FWD_DEFINED__
#define __IFabricServiceCommunicationClient_FWD_DEFINED__
typedef interface IFabricServiceCommunicationClient IFabricServiceCommunicationClient;

#endif 	/* __IFabricServiceCommunicationClient_FWD_DEFINED__ */


#ifndef __IFabricServiceCommunicationClient2_FWD_DEFINED__
#define __IFabricServiceCommunicationClient2_FWD_DEFINED__
typedef interface IFabricServiceCommunicationClient2 IFabricServiceCommunicationClient2;

#endif 	/* __IFabricServiceCommunicationClient2_FWD_DEFINED__ */


#ifndef __IFabricServiceConnectionEventHandler_FWD_DEFINED__
#define __IFabricServiceConnectionEventHandler_FWD_DEFINED__
typedef interface IFabricServiceConnectionEventHandler IFabricServiceConnectionEventHandler;

#endif 	/* __IFabricServiceConnectionEventHandler_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"
#include "FabricTypes_.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricservicecommunication__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif











extern RPC_IF_HANDLE __MIDL_itf_fabricservicecommunication__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricservicecommunication__0000_0000_v0_0_s_ifspec;


#ifndef __FabricServiceCommunication_Lib_LIBRARY_DEFINED__
#define __FabricServiceCommunication_Lib_LIBRARY_DEFINED__

/* library FabricServiceCommunication_Lib */
/* [version][uuid] */ 


#pragma pack(push, 8)
typedef LPCWSTR COMMUNICATION_CLIENT_ID;











#pragma pack(pop)

EXTERN_C const IID LIBID_FabricServiceCommunication_Lib;

#ifndef __IFabricCommunicationMessageSender_INTERFACE_DEFINED__
#define __IFabricCommunicationMessageSender_INTERFACE_DEFINED__

/* interface IFabricCommunicationMessageSender */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCommunicationMessageSender;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fdf2bcd7-14f9-463f-9b70-ae3b5ff9d83f")
    IFabricCommunicationMessageSender : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRequest( 
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRequest( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendMessage( 
            /* [in] */ IFabricServiceCommunicationMessage *message) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCommunicationMessageSenderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCommunicationMessageSender * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCommunicationMessageSender * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCommunicationMessageSender * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRequest )( 
            IFabricCommunicationMessageSender * This,
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRequest )( 
            IFabricCommunicationMessageSender * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            IFabricCommunicationMessageSender * This,
            /* [in] */ IFabricServiceCommunicationMessage *message);
        
        END_INTERFACE
    } IFabricCommunicationMessageSenderVtbl;

    interface IFabricCommunicationMessageSender
    {
        CONST_VTBL struct IFabricCommunicationMessageSenderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCommunicationMessageSender_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCommunicationMessageSender_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCommunicationMessageSender_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCommunicationMessageSender_BeginRequest(This,message,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRequest(This,message,timeoutMilliseconds,callback,context) ) 

#define IFabricCommunicationMessageSender_EndRequest(This,context,reply)	\
    ( (This)->lpVtbl -> EndRequest(This,context,reply) ) 

#define IFabricCommunicationMessageSender_SendMessage(This,message)	\
    ( (This)->lpVtbl -> SendMessage(This,message) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCommunicationMessageSender_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceConnectionHandler_INTERFACE_DEFINED__
#define __IFabricServiceConnectionHandler_INTERFACE_DEFINED__

/* interface IFabricServiceConnectionHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceConnectionHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b069692d-e8f0-4f25-a3b6-b2992598a64c")
    IFabricServiceConnectionHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginProcessConnect( 
            /* [in] */ IFabricClientConnection *clientConnection,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndProcessConnect( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginProcessDisconnect( 
            /* [in] */ LPCWSTR clientId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndProcessDisconnect( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceConnectionHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceConnectionHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceConnectionHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceConnectionHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginProcessConnect )( 
            IFabricServiceConnectionHandler * This,
            /* [in] */ IFabricClientConnection *clientConnection,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndProcessConnect )( 
            IFabricServiceConnectionHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginProcessDisconnect )( 
            IFabricServiceConnectionHandler * This,
            /* [in] */ LPCWSTR clientId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndProcessDisconnect )( 
            IFabricServiceConnectionHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricServiceConnectionHandlerVtbl;

    interface IFabricServiceConnectionHandler
    {
        CONST_VTBL struct IFabricServiceConnectionHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceConnectionHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceConnectionHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceConnectionHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceConnectionHandler_BeginProcessConnect(This,clientConnection,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginProcessConnect(This,clientConnection,timeoutMilliseconds,callback,context) ) 

#define IFabricServiceConnectionHandler_EndProcessConnect(This,context)	\
    ( (This)->lpVtbl -> EndProcessConnect(This,context) ) 

#define IFabricServiceConnectionHandler_BeginProcessDisconnect(This,clientId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginProcessDisconnect(This,clientId,timeoutMilliseconds,callback,context) ) 

#define IFabricServiceConnectionHandler_EndProcessDisconnect(This,context)	\
    ( (This)->lpVtbl -> EndProcessDisconnect(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceConnectionHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceCommunicationClient_INTERFACE_DEFINED__
#define __IFabricServiceCommunicationClient_INTERFACE_DEFINED__

/* interface IFabricServiceCommunicationClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceCommunicationClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("255ecbe8-96b8-4f47-9e2c-1235dba3220a")
    IFabricServiceCommunicationClient : public IFabricCommunicationMessageSender
    {
    public:
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceCommunicationClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceCommunicationClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceCommunicationClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceCommunicationClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRequest )( 
            IFabricServiceCommunicationClient * This,
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRequest )( 
            IFabricServiceCommunicationClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            IFabricServiceCommunicationClient * This,
            /* [in] */ IFabricServiceCommunicationMessage *message);
        
        END_INTERFACE
    } IFabricServiceCommunicationClientVtbl;

    interface IFabricServiceCommunicationClient
    {
        CONST_VTBL struct IFabricServiceCommunicationClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceCommunicationClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceCommunicationClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceCommunicationClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceCommunicationClient_BeginRequest(This,message,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRequest(This,message,timeoutMilliseconds,callback,context) ) 

#define IFabricServiceCommunicationClient_EndRequest(This,context,reply)	\
    ( (This)->lpVtbl -> EndRequest(This,context,reply) ) 

#define IFabricServiceCommunicationClient_SendMessage(This,message)	\
    ( (This)->lpVtbl -> SendMessage(This,message) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceCommunicationClient_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceCommunicationClient2_INTERFACE_DEFINED__
#define __IFabricServiceCommunicationClient2_INTERFACE_DEFINED__

/* interface IFabricServiceCommunicationClient2 */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceCommunicationClient2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("73b2cac5-4278-475b-82e6-1e33ebe20767")
    IFabricServiceCommunicationClient2 : public IFabricServiceCommunicationClient
    {
    public:
        virtual void STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceCommunicationClient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceCommunicationClient2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceCommunicationClient2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceCommunicationClient2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRequest )( 
            IFabricServiceCommunicationClient2 * This,
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRequest )( 
            IFabricServiceCommunicationClient2 * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            IFabricServiceCommunicationClient2 * This,
            /* [in] */ IFabricServiceCommunicationMessage *message);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricServiceCommunicationClient2 * This);
        
        END_INTERFACE
    } IFabricServiceCommunicationClient2Vtbl;

    interface IFabricServiceCommunicationClient2
    {
        CONST_VTBL struct IFabricServiceCommunicationClient2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceCommunicationClient2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceCommunicationClient2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceCommunicationClient2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceCommunicationClient2_BeginRequest(This,message,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRequest(This,message,timeoutMilliseconds,callback,context) ) 

#define IFabricServiceCommunicationClient2_EndRequest(This,context,reply)	\
    ( (This)->lpVtbl -> EndRequest(This,context,reply) ) 

#define IFabricServiceCommunicationClient2_SendMessage(This,message)	\
    ( (This)->lpVtbl -> SendMessage(This,message) ) 



#define IFabricServiceCommunicationClient2_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceCommunicationClient2_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceCommunicationListener_INTERFACE_DEFINED__
#define __IFabricServiceCommunicationListener_INTERFACE_DEFINED__

/* interface IFabricServiceCommunicationListener */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceCommunicationListener;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ad5d9f82-d62c-4819-9938-668540248e97")
    IFabricServiceCommunicationListener : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual void STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceCommunicationListenerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceCommunicationListener * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceCommunicationListener * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceCommunicationListener * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricServiceCommunicationListener * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricServiceCommunicationListener * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricServiceCommunicationListener * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricServiceCommunicationListener * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricServiceCommunicationListener * This);
        
        END_INTERFACE
    } IFabricServiceCommunicationListenerVtbl;

    interface IFabricServiceCommunicationListener
    {
        CONST_VTBL struct IFabricServiceCommunicationListenerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceCommunicationListener_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceCommunicationListener_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceCommunicationListener_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceCommunicationListener_BeginOpen(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,callback,context) ) 

#define IFabricServiceCommunicationListener_EndOpen(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndOpen(This,context,serviceAddress) ) 

#define IFabricServiceCommunicationListener_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricServiceCommunicationListener_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricServiceCommunicationListener_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceCommunicationListener_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceCommunicationMessage_INTERFACE_DEFINED__
#define __IFabricServiceCommunicationMessage_INTERFACE_DEFINED__

/* interface IFabricServiceCommunicationMessage */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceCommunicationMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc6e168a-dbd4-4ce1-a3dc-5f33494f4972")
    IFabricServiceCommunicationMessage : public IUnknown
    {
    public:
        virtual FABRIC_MESSAGE_BUFFER *STDMETHODCALLTYPE Get_Body( void) = 0;
        
        virtual FABRIC_MESSAGE_BUFFER *STDMETHODCALLTYPE Get_Headers( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceCommunicationMessageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceCommunicationMessage * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceCommunicationMessage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceCommunicationMessage * This);
        
        FABRIC_MESSAGE_BUFFER *( STDMETHODCALLTYPE *Get_Body )( 
            IFabricServiceCommunicationMessage * This);
        
        FABRIC_MESSAGE_BUFFER *( STDMETHODCALLTYPE *Get_Headers )( 
            IFabricServiceCommunicationMessage * This);
        
        END_INTERFACE
    } IFabricServiceCommunicationMessageVtbl;

    interface IFabricServiceCommunicationMessage
    {
        CONST_VTBL struct IFabricServiceCommunicationMessageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceCommunicationMessage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceCommunicationMessage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceCommunicationMessage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceCommunicationMessage_Get_Body(This)	\
    ( (This)->lpVtbl -> Get_Body(This) ) 

#define IFabricServiceCommunicationMessage_Get_Headers(This)	\
    ( (This)->lpVtbl -> Get_Headers(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceCommunicationMessage_INTERFACE_DEFINED__ */


#ifndef __IFabricClientConnection_INTERFACE_DEFINED__
#define __IFabricClientConnection_INTERFACE_DEFINED__

/* interface IFabricClientConnection */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricClientConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("60ae1ab3-5f00-404d-8f89-96485c8b013e")
    IFabricClientConnection : public IFabricCommunicationMessageSender
    {
    public:
        virtual COMMUNICATION_CLIENT_ID STDMETHODCALLTYPE get_ClientId( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricClientConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricClientConnection * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricClientConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricClientConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRequest )( 
            IFabricClientConnection * This,
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRequest )( 
            IFabricClientConnection * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            IFabricClientConnection * This,
            /* [in] */ IFabricServiceCommunicationMessage *message);
        
        COMMUNICATION_CLIENT_ID ( STDMETHODCALLTYPE *get_ClientId )( 
            IFabricClientConnection * This);
        
        END_INTERFACE
    } IFabricClientConnectionVtbl;

    interface IFabricClientConnection
    {
        CONST_VTBL struct IFabricClientConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricClientConnection_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricClientConnection_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricClientConnection_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricClientConnection_BeginRequest(This,message,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRequest(This,message,timeoutMilliseconds,callback,context) ) 

#define IFabricClientConnection_EndRequest(This,context,reply)	\
    ( (This)->lpVtbl -> EndRequest(This,context,reply) ) 

#define IFabricClientConnection_SendMessage(This,message)	\
    ( (This)->lpVtbl -> SendMessage(This,message) ) 


#define IFabricClientConnection_get_ClientId(This)	\
    ( (This)->lpVtbl -> get_ClientId(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricClientConnection_INTERFACE_DEFINED__ */


#ifndef __IFabricServiceConnectionEventHandler_INTERFACE_DEFINED__
#define __IFabricServiceConnectionEventHandler_INTERFACE_DEFINED__

/* interface IFabricServiceConnectionEventHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricServiceConnectionEventHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("77f434b1-f9e9-4cb1-b0c4-c7ea2984aa8d")
    IFabricServiceConnectionEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnConnected( 
            /* [in] */ LPCWSTR connectionAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDisconnected( 
            /* [in] */ LPCWSTR connectionAddress,
            /* [in] */ HRESULT error) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricServiceConnectionEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricServiceConnectionEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricServiceConnectionEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricServiceConnectionEventHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnConnected )( 
            IFabricServiceConnectionEventHandler * This,
            /* [in] */ LPCWSTR connectionAddress);
        
        HRESULT ( STDMETHODCALLTYPE *OnDisconnected )( 
            IFabricServiceConnectionEventHandler * This,
            /* [in] */ LPCWSTR connectionAddress,
            /* [in] */ HRESULT error);
        
        END_INTERFACE
    } IFabricServiceConnectionEventHandlerVtbl;

    interface IFabricServiceConnectionEventHandler
    {
        CONST_VTBL struct IFabricServiceConnectionEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricServiceConnectionEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricServiceConnectionEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricServiceConnectionEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricServiceConnectionEventHandler_OnConnected(This,connectionAddress)	\
    ( (This)->lpVtbl -> OnConnected(This,connectionAddress) ) 

#define IFabricServiceConnectionEventHandler_OnDisconnected(This,connectionAddress,error)	\
    ( (This)->lpVtbl -> OnDisconnected(This,connectionAddress,error) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricServiceConnectionEventHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricCommunicationMessageHandler_INTERFACE_DEFINED__
#define __IFabricCommunicationMessageHandler_INTERFACE_DEFINED__

/* interface IFabricCommunicationMessageHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricCommunicationMessageHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7e010010-80b2-453c-aab3-a73f0790dfac")
    IFabricCommunicationMessageHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginProcessRequest( 
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndProcessRequest( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HandleOneWay( 
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricServiceCommunicationMessage *message) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricCommunicationMessageHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricCommunicationMessageHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricCommunicationMessageHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricCommunicationMessageHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginProcessRequest )( 
            IFabricCommunicationMessageHandler * This,
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndProcessRequest )( 
            IFabricCommunicationMessageHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply);
        
        HRESULT ( STDMETHODCALLTYPE *HandleOneWay )( 
            IFabricCommunicationMessageHandler * This,
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricServiceCommunicationMessage *message);
        
        END_INTERFACE
    } IFabricCommunicationMessageHandlerVtbl;

    interface IFabricCommunicationMessageHandler
    {
        CONST_VTBL struct IFabricCommunicationMessageHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricCommunicationMessageHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricCommunicationMessageHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricCommunicationMessageHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricCommunicationMessageHandler_BeginProcessRequest(This,clientId,message,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginProcessRequest(This,clientId,message,timeoutMilliseconds,callback,context) ) 

#define IFabricCommunicationMessageHandler_EndProcessRequest(This,context,reply)	\
    ( (This)->lpVtbl -> EndProcessRequest(This,context,reply) ) 

#define IFabricCommunicationMessageHandler_HandleOneWay(This,clientId,message)	\
    ( (This)->lpVtbl -> HandleOneWay(This,clientId,message) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricCommunicationMessageHandler_INTERFACE_DEFINED__ */



#ifndef __FabricServiceCommunication_MODULE_DEFINED__
#define __FabricServiceCommunication_MODULE_DEFINED__


/* module FabricServiceCommunication */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateServiceCommunicationListener( 
    /* [in] */ __RPC__in REFIID interfaceId,
    /* [in] */ __RPC__in FABRIC_SERVICE_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in FABRIC_SERVICE_LISTENER_ADDRESS *address,
    /* [in] */ __RPC__in_opt IFabricCommunicationMessageHandler *clientRequestHandler,
    /* [in] */ __RPC__in_opt IFabricServiceConnectionHandler *clientConnectionHandler,
    /* [retval][out] */ __RPC__deref_out_opt IFabricServiceCommunicationListener **listener);

/* [entry] */ HRESULT CreateServiceCommunicationClient( 
    /* [in] */ __RPC__in REFIID interfaceId,
    /* [in] */ __RPC__in FABRIC_SERVICE_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in LPCWSTR connectionAddress,
    /* [in] */ __RPC__in_opt IFabricCommunicationMessageHandler *notificationHandler,
    /* [in] */ __RPC__in_opt IFabricServiceConnectionEventHandler *connectionEventHandler,
    /* [retval][out] */ __RPC__deref_out_opt IFabricServiceCommunicationClient **client);

#endif /* __FabricServiceCommunication_MODULE_DEFINED__ */
#endif /* __FabricServiceCommunication_Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


