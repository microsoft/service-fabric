

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

#ifndef __FabricTransport__h__
#define __FabricTransport__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricTransportConnectionHandler_FWD_DEFINED__
#define __IFabricTransportConnectionHandler_FWD_DEFINED__
typedef interface IFabricTransportConnectionHandler IFabricTransportConnectionHandler;

#endif 	/* __IFabricTransportConnectionHandler_FWD_DEFINED__ */


#ifndef __IFabricTransportMessage_FWD_DEFINED__
#define __IFabricTransportMessage_FWD_DEFINED__
typedef interface IFabricTransportMessage IFabricTransportMessage;

#endif 	/* __IFabricTransportMessage_FWD_DEFINED__ */


#ifndef __IFabricTransportMessageHandler_FWD_DEFINED__
#define __IFabricTransportMessageHandler_FWD_DEFINED__
typedef interface IFabricTransportMessageHandler IFabricTransportMessageHandler;

#endif 	/* __IFabricTransportMessageHandler_FWD_DEFINED__ */


#ifndef __IFabricTransportCallbackMessageHandler_FWD_DEFINED__
#define __IFabricTransportCallbackMessageHandler_FWD_DEFINED__
typedef interface IFabricTransportCallbackMessageHandler IFabricTransportCallbackMessageHandler;

#endif 	/* __IFabricTransportCallbackMessageHandler_FWD_DEFINED__ */


#ifndef __IFabricTransportListener_FWD_DEFINED__
#define __IFabricTransportListener_FWD_DEFINED__
typedef interface IFabricTransportListener IFabricTransportListener;

#endif 	/* __IFabricTransportListener_FWD_DEFINED__ */


#ifndef __IFabricTransportClient_FWD_DEFINED__
#define __IFabricTransportClient_FWD_DEFINED__
typedef interface IFabricTransportClient IFabricTransportClient;

#endif 	/* __IFabricTransportClient_FWD_DEFINED__ */


#ifndef __IFabricTransportClientEventHandler_FWD_DEFINED__
#define __IFabricTransportClientEventHandler_FWD_DEFINED__
typedef interface IFabricTransportClientEventHandler IFabricTransportClientEventHandler;

#endif 	/* __IFabricTransportClientEventHandler_FWD_DEFINED__ */


#ifndef __IFabricTransportClientConnection_FWD_DEFINED__
#define __IFabricTransportClientConnection_FWD_DEFINED__
typedef interface IFabricTransportClientConnection IFabricTransportClientConnection;

#endif 	/* __IFabricTransportClientConnection_FWD_DEFINED__ */


#ifndef __IFabricTransportMessageDisposer_FWD_DEFINED__
#define __IFabricTransportMessageDisposer_FWD_DEFINED__
typedef interface IFabricTransportMessageDisposer IFabricTransportMessageDisposer;

#endif 	/* __IFabricTransportMessageDisposer_FWD_DEFINED__ */


#ifndef __IFabricTransportMessage_FWD_DEFINED__
#define __IFabricTransportMessage_FWD_DEFINED__
typedef interface IFabricTransportMessage IFabricTransportMessage;

#endif 	/* __IFabricTransportMessage_FWD_DEFINED__ */


#ifndef __IFabricTransportMessageDisposer_FWD_DEFINED__
#define __IFabricTransportMessageDisposer_FWD_DEFINED__
typedef interface IFabricTransportMessageDisposer IFabricTransportMessageDisposer;

#endif 	/* __IFabricTransportMessageDisposer_FWD_DEFINED__ */


#ifndef __IFabricTransportMessageHandler_FWD_DEFINED__
#define __IFabricTransportMessageHandler_FWD_DEFINED__
typedef interface IFabricTransportMessageHandler IFabricTransportMessageHandler;

#endif 	/* __IFabricTransportMessageHandler_FWD_DEFINED__ */


#ifndef __IFabricTransportListener_FWD_DEFINED__
#define __IFabricTransportListener_FWD_DEFINED__
typedef interface IFabricTransportListener IFabricTransportListener;

#endif 	/* __IFabricTransportListener_FWD_DEFINED__ */


#ifndef __IFabricTransportClient_FWD_DEFINED__
#define __IFabricTransportClient_FWD_DEFINED__
typedef interface IFabricTransportClient IFabricTransportClient;

#endif 	/* __IFabricTransportClient_FWD_DEFINED__ */


#ifndef __IFabricTransportClientConnection_FWD_DEFINED__
#define __IFabricTransportClientConnection_FWD_DEFINED__
typedef interface IFabricTransportClientConnection IFabricTransportClientConnection;

#endif 	/* __IFabricTransportClientConnection_FWD_DEFINED__ */


#ifndef __IFabricTransportConnectionHandler_FWD_DEFINED__
#define __IFabricTransportConnectionHandler_FWD_DEFINED__
typedef interface IFabricTransportConnectionHandler IFabricTransportConnectionHandler;

#endif 	/* __IFabricTransportConnectionHandler_FWD_DEFINED__ */


#ifndef __IFabricTransportCallbackMessageHandler_FWD_DEFINED__
#define __IFabricTransportCallbackMessageHandler_FWD_DEFINED__
typedef interface IFabricTransportCallbackMessageHandler IFabricTransportCallbackMessageHandler;

#endif 	/* __IFabricTransportCallbackMessageHandler_FWD_DEFINED__ */


#ifndef __IFabricTransportClientEventHandler_FWD_DEFINED__
#define __IFabricTransportClientEventHandler_FWD_DEFINED__
typedef interface IFabricTransportClientEventHandler IFabricTransportClientEventHandler;

#endif 	/* __IFabricTransportClientEventHandler_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricCommon.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabrictransport__0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif











extern RPC_IF_HANDLE __MIDL_itf_fabrictransport__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabrictransport__0000_0000_v0_0_s_ifspec;


#ifndef __FabricTransport_Lib_LIBRARY_DEFINED__
#define __FabricTransport_Lib_LIBRARY_DEFINED__

/* library FabricTransport_Lib */
/* [version][uuid] */ 


#pragma pack(push, 8)
typedef LPCWSTR COMMUNICATION_CLIENT_ID;










typedef struct FABRIC_TRANSPORT_SETTINGS
    {
    ULONG OperationTimeoutInSeconds;
    ULONG KeepAliveTimeoutInSeconds;
    ULONG MaxMessageSize;
    ULONG MaxConcurrentCalls;
    ULONG MaxQueueSize;
    const FABRIC_SECURITY_CREDENTIALS *SecurityCredentials;
    void *Reserved;
    } 	FABRIC_TRANSPORT_SETTINGS;

typedef struct FABRIC_TRANSPORT_LISTEN_ADDRESS
    {
    LPCWSTR IPAddressOrFQDN;
    ULONG Port;
    LPCWSTR Path;
    } 	FABRIC_TRANSPORT_LISTEN_ADDRESS;

typedef struct FABRIC_TRANSPORT_MESSAGE_BUFFER
    {
    ULONG BufferSize;
    /* [size_is] */ BYTE *Buffer;
    } 	FABRIC_TRANSPORT_MESSAGE_BUFFER;


#pragma pack(pop)

EXTERN_C const IID LIBID_FabricTransport_Lib;

#ifndef __IFabricTransportConnectionHandler_INTERFACE_DEFINED__
#define __IFabricTransportConnectionHandler_INTERFACE_DEFINED__

/* interface IFabricTransportConnectionHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportConnectionHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b069692d-e8f0-4f25-a3b6-b2992598a64c")
    IFabricTransportConnectionHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginProcessConnect( 
            /* [in] */ IFabricTransportClientConnection *clientConnection,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndProcessConnect( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginProcessDisconnect( 
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndProcessDisconnect( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportConnectionHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportConnectionHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportConnectionHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportConnectionHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginProcessConnect )( 
            IFabricTransportConnectionHandler * This,
            /* [in] */ IFabricTransportClientConnection *clientConnection,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndProcessConnect )( 
            IFabricTransportConnectionHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginProcessDisconnect )( 
            IFabricTransportConnectionHandler * This,
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndProcessDisconnect )( 
            IFabricTransportConnectionHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricTransportConnectionHandlerVtbl;

    interface IFabricTransportConnectionHandler
    {
        CONST_VTBL struct IFabricTransportConnectionHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportConnectionHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportConnectionHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportConnectionHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportConnectionHandler_BeginProcessConnect(This,clientConnection,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginProcessConnect(This,clientConnection,timeoutMilliseconds,callback,context) ) 

#define IFabricTransportConnectionHandler_EndProcessConnect(This,context)	\
    ( (This)->lpVtbl -> EndProcessConnect(This,context) ) 

#define IFabricTransportConnectionHandler_BeginProcessDisconnect(This,clientId,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginProcessDisconnect(This,clientId,timeoutMilliseconds,callback,context) ) 

#define IFabricTransportConnectionHandler_EndProcessDisconnect(This,context)	\
    ( (This)->lpVtbl -> EndProcessDisconnect(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportConnectionHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportMessage_INTERFACE_DEFINED__
#define __IFabricTransportMessage_INTERFACE_DEFINED__

/* interface IFabricTransportMessage */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b4357dab-ef06-465f-b453-938f3b0ad4b5")
    IFabricTransportMessage : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE GetHeaderAndBodyBuffer( 
            /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **headerBuffer,
            /* [out] */ ULONG *msgBufferCount,
            /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **MsgBuffers) = 0;
        
        virtual void STDMETHODCALLTYPE Dispose( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportMessageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportMessage * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportMessage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportMessage * This);
        
        void ( STDMETHODCALLTYPE *GetHeaderAndBodyBuffer )( 
            IFabricTransportMessage * This,
            /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **headerBuffer,
            /* [out] */ ULONG *msgBufferCount,
            /* [out] */ const FABRIC_TRANSPORT_MESSAGE_BUFFER **MsgBuffers);
        
        void ( STDMETHODCALLTYPE *Dispose )( 
            IFabricTransportMessage * This);
        
        END_INTERFACE
    } IFabricTransportMessageVtbl;

    interface IFabricTransportMessage
    {
        CONST_VTBL struct IFabricTransportMessageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportMessage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportMessage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportMessage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportMessage_GetHeaderAndBodyBuffer(This,headerBuffer,msgBufferCount,MsgBuffers)	\
    ( (This)->lpVtbl -> GetHeaderAndBodyBuffer(This,headerBuffer,msgBufferCount,MsgBuffers) ) 

#define IFabricTransportMessage_Dispose(This)	\
    ( (This)->lpVtbl -> Dispose(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportMessage_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportMessageHandler_INTERFACE_DEFINED__
#define __IFabricTransportMessageHandler_INTERFACE_DEFINED__

/* interface IFabricTransportMessageHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportMessageHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6815bdb4-1479-4c44-8b9d-57d6d0cc9d64")
    IFabricTransportMessageHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginProcessRequest( 
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricTransportMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndProcessRequest( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricTransportMessage **reply) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HandleOneWay( 
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricTransportMessage *message) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportMessageHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportMessageHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportMessageHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportMessageHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginProcessRequest )( 
            IFabricTransportMessageHandler * This,
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricTransportMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndProcessRequest )( 
            IFabricTransportMessageHandler * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricTransportMessage **reply);
        
        HRESULT ( STDMETHODCALLTYPE *HandleOneWay )( 
            IFabricTransportMessageHandler * This,
            /* [in] */ COMMUNICATION_CLIENT_ID clientId,
            /* [in] */ IFabricTransportMessage *message);
        
        END_INTERFACE
    } IFabricTransportMessageHandlerVtbl;

    interface IFabricTransportMessageHandler
    {
        CONST_VTBL struct IFabricTransportMessageHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportMessageHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportMessageHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportMessageHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportMessageHandler_BeginProcessRequest(This,clientId,message,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginProcessRequest(This,clientId,message,timeoutMilliseconds,callback,context) ) 

#define IFabricTransportMessageHandler_EndProcessRequest(This,context,reply)	\
    ( (This)->lpVtbl -> EndProcessRequest(This,context,reply) ) 

#define IFabricTransportMessageHandler_HandleOneWay(This,clientId,message)	\
    ( (This)->lpVtbl -> HandleOneWay(This,clientId,message) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportMessageHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportCallbackMessageHandler_INTERFACE_DEFINED__
#define __IFabricTransportCallbackMessageHandler_INTERFACE_DEFINED__

/* interface IFabricTransportCallbackMessageHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportCallbackMessageHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9ba8ac7a-3464-4774-b9b9-1d7f0f1920ba")
    IFabricTransportCallbackMessageHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE HandleOneWay( 
            /* [in] */ IFabricTransportMessage *message) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportCallbackMessageHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportCallbackMessageHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportCallbackMessageHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportCallbackMessageHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *HandleOneWay )( 
            IFabricTransportCallbackMessageHandler * This,
            /* [in] */ IFabricTransportMessage *message);
        
        END_INTERFACE
    } IFabricTransportCallbackMessageHandlerVtbl;

    interface IFabricTransportCallbackMessageHandler
    {
        CONST_VTBL struct IFabricTransportCallbackMessageHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportCallbackMessageHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportCallbackMessageHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportCallbackMessageHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportCallbackMessageHandler_HandleOneWay(This,message)	\
    ( (This)->lpVtbl -> HandleOneWay(This,message) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportCallbackMessageHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportListener_INTERFACE_DEFINED__
#define __IFabricTransportListener_INTERFACE_DEFINED__

/* interface IFabricTransportListener */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportListener;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1b63a266-1eeb-4f3e-8886-521458980d10")
    IFabricTransportListener : public IUnknown
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

    typedef struct IFabricTransportListenerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportListener * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportListener * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportListener * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricTransportListener * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricTransportListener * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricTransportListener * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricTransportListener * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricTransportListener * This);
        
        END_INTERFACE
    } IFabricTransportListenerVtbl;

    interface IFabricTransportListener
    {
        CONST_VTBL struct IFabricTransportListenerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportListener_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportListener_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportListener_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportListener_BeginOpen(This,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,callback,context) ) 

#define IFabricTransportListener_EndOpen(This,context,serviceAddress)	\
    ( (This)->lpVtbl -> EndOpen(This,context,serviceAddress) ) 

#define IFabricTransportListener_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricTransportListener_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricTransportListener_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportListener_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportClient_INTERFACE_DEFINED__
#define __IFabricTransportClient_INTERFACE_DEFINED__

/* interface IFabricTransportClient */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5b0634fe-6a52-4bd9-8059-892c72c1d73a")
    IFabricTransportClient : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginRequest( 
            /* [in] */ IFabricTransportMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRequest( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricTransportMessage **reply) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Send( 
            /* [in] */ IFabricTransportMessage *message) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual void STDMETHODCALLTYPE Abort( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRequest )( 
            IFabricTransportClient * This,
            /* [in] */ IFabricTransportMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndRequest )( 
            IFabricTransportClient * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricTransportMessage **reply);
        
        HRESULT ( STDMETHODCALLTYPE *Send )( 
            IFabricTransportClient * This,
            /* [in] */ IFabricTransportMessage *message);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricTransportClient * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricTransportClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricTransportClient * This,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricTransportClient * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void ( STDMETHODCALLTYPE *Abort )( 
            IFabricTransportClient * This);
        
        END_INTERFACE
    } IFabricTransportClientVtbl;

    interface IFabricTransportClient
    {
        CONST_VTBL struct IFabricTransportClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportClient_BeginRequest(This,message,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginRequest(This,message,timeoutMilliseconds,callback,context) ) 

#define IFabricTransportClient_EndRequest(This,context,reply)	\
    ( (This)->lpVtbl -> EndRequest(This,context,reply) ) 

#define IFabricTransportClient_Send(This,message)	\
    ( (This)->lpVtbl -> Send(This,message) ) 

#define IFabricTransportClient_BeginOpen(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,timeoutMilliseconds,callback,context) ) 

#define IFabricTransportClient_EndOpen(This,context)	\
    ( (This)->lpVtbl -> EndOpen(This,context) ) 

#define IFabricTransportClient_BeginClose(This,timeoutMilliseconds,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,timeoutMilliseconds,callback,context) ) 

#define IFabricTransportClient_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricTransportClient_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportClient_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportClientEventHandler_INTERFACE_DEFINED__
#define __IFabricTransportClientEventHandler_INTERFACE_DEFINED__

/* interface IFabricTransportClientEventHandler */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportClientEventHandler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4935ab6f-a8bc-4b10-a69e-7a3ba3324892")
    IFabricTransportClientEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnConnected( 
            /* [in] */ LPCWSTR connectionAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDisconnected( 
            /* [in] */ LPCWSTR connectionAddress,
            /* [in] */ HRESULT error) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportClientEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportClientEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportClientEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportClientEventHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnConnected )( 
            IFabricTransportClientEventHandler * This,
            /* [in] */ LPCWSTR connectionAddress);
        
        HRESULT ( STDMETHODCALLTYPE *OnDisconnected )( 
            IFabricTransportClientEventHandler * This,
            /* [in] */ LPCWSTR connectionAddress,
            /* [in] */ HRESULT error);
        
        END_INTERFACE
    } IFabricTransportClientEventHandlerVtbl;

    interface IFabricTransportClientEventHandler
    {
        CONST_VTBL struct IFabricTransportClientEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportClientEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportClientEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportClientEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportClientEventHandler_OnConnected(This,connectionAddress)	\
    ( (This)->lpVtbl -> OnConnected(This,connectionAddress) ) 

#define IFabricTransportClientEventHandler_OnDisconnected(This,connectionAddress,error)	\
    ( (This)->lpVtbl -> OnDisconnected(This,connectionAddress,error) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportClientEventHandler_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportClientConnection_INTERFACE_DEFINED__
#define __IFabricTransportClientConnection_INTERFACE_DEFINED__

/* interface IFabricTransportClientConnection */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportClientConnection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a54c17f7-fe94-4838-b14d-e9b5c258e2d0")
    IFabricTransportClientConnection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Send( 
            /* [in] */ IFabricTransportMessage *message) = 0;
        
        virtual COMMUNICATION_CLIENT_ID STDMETHODCALLTYPE get_ClientId( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportClientConnectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportClientConnection * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportClientConnection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportClientConnection * This);
        
        HRESULT ( STDMETHODCALLTYPE *Send )( 
            IFabricTransportClientConnection * This,
            /* [in] */ IFabricTransportMessage *message);
        
        COMMUNICATION_CLIENT_ID ( STDMETHODCALLTYPE *get_ClientId )( 
            IFabricTransportClientConnection * This);
        
        END_INTERFACE
    } IFabricTransportClientConnectionVtbl;

    interface IFabricTransportClientConnection
    {
        CONST_VTBL struct IFabricTransportClientConnectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportClientConnection_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportClientConnection_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportClientConnection_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportClientConnection_Send(This,message)	\
    ( (This)->lpVtbl -> Send(This,message) ) 

#define IFabricTransportClientConnection_get_ClientId(This)	\
    ( (This)->lpVtbl -> get_ClientId(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportClientConnection_INTERFACE_DEFINED__ */


#ifndef __IFabricTransportMessageDisposer_INTERFACE_DEFINED__
#define __IFabricTransportMessageDisposer_INTERFACE_DEFINED__

/* interface IFabricTransportMessageDisposer */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricTransportMessageDisposer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("914097f3-a821-46ea-b3d9-feafe5f7c4a9")
    IFabricTransportMessageDisposer : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE Dispose( 
            /* [in] */ ULONG Count,
            /* [size_is][in] */ IFabricTransportMessage **messages) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricTransportMessageDisposerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricTransportMessageDisposer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricTransportMessageDisposer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricTransportMessageDisposer * This);
        
        void ( STDMETHODCALLTYPE *Dispose )( 
            IFabricTransportMessageDisposer * This,
            /* [in] */ ULONG Count,
            /* [size_is][in] */ IFabricTransportMessage **messages);
        
        END_INTERFACE
    } IFabricTransportMessageDisposerVtbl;

    interface IFabricTransportMessageDisposer
    {
        CONST_VTBL struct IFabricTransportMessageDisposerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricTransportMessageDisposer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricTransportMessageDisposer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricTransportMessageDisposer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricTransportMessageDisposer_Dispose(This,Count,messages)	\
    ( (This)->lpVtbl -> Dispose(This,Count,messages) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricTransportMessageDisposer_INTERFACE_DEFINED__ */



#ifndef __FabricTransport_MODULE_DEFINED__
#define __FabricTransport_MODULE_DEFINED__


/* module FabricTransport */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT CreateFabricTransportListener( 
    /* [in] */ __RPC__in REFIID interfaceId,
    /* [in] */ __RPC__in FABRIC_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in FABRIC_TRANSPORT_LISTEN_ADDRESS *address,
    /* [in] */ __RPC__in_opt IFabricTransportMessageHandler *requestHandler,
    /* [in] */ __RPC__in_opt IFabricTransportConnectionHandler *connectionHandler,
    /* [in] */ __RPC__in_opt IFabricTransportMessageDisposer *disposeProcessor,
    /* [retval][out] */ __RPC__deref_out_opt IFabricTransportListener **listener);

/* [entry] */ HRESULT CreateFabricTransportClient( 
    /* [in] */ __RPC__in REFIID interfaceId,
    /* [in] */ __RPC__in FABRIC_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in LPCWSTR connectionAddress,
    /* [in] */ __RPC__in_opt IFabricTransportCallbackMessageHandler *notificationHandler,
    /* [in] */ __RPC__in_opt IFabricTransportClientEventHandler *clientEventHandler,
    /* [in] */ __RPC__in_opt IFabricTransportMessageDisposer *messageDisposer,
    /* [retval][out] */ __RPC__deref_out_opt IFabricTransportClient **client);

#endif /* __FabricTransport_MODULE_DEFINED__ */
#endif /* __FabricTransport_Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


