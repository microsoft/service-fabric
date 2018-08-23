

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

#ifndef __fabriccommon_h__
#define __fabriccommon_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricAsyncOperationCallback_FWD_DEFINED__
#define __IFabricAsyncOperationCallback_FWD_DEFINED__
typedef interface IFabricAsyncOperationCallback IFabricAsyncOperationCallback;

#endif 	/* __IFabricAsyncOperationCallback_FWD_DEFINED__ */


#ifndef __IFabricAsyncOperationContext_FWD_DEFINED__
#define __IFabricAsyncOperationContext_FWD_DEFINED__
typedef interface IFabricAsyncOperationContext IFabricAsyncOperationContext;

#endif 	/* __IFabricAsyncOperationContext_FWD_DEFINED__ */


#ifndef __IFabricStringResult_FWD_DEFINED__
#define __IFabricStringResult_FWD_DEFINED__
typedef interface IFabricStringResult IFabricStringResult;

#endif 	/* __IFabricStringResult_FWD_DEFINED__ */


#ifndef __IFabricStringListResult_FWD_DEFINED__
#define __IFabricStringListResult_FWD_DEFINED__
typedef interface IFabricStringListResult IFabricStringListResult;

#endif 	/* __IFabricStringListResult_FWD_DEFINED__ */


#ifndef __IFabricGetReplicatorStatusResult_FWD_DEFINED__
#define __IFabricGetReplicatorStatusResult_FWD_DEFINED__
typedef interface IFabricGetReplicatorStatusResult IFabricGetReplicatorStatusResult;

#endif 	/* __IFabricGetReplicatorStatusResult_FWD_DEFINED__ */


#ifndef __IFabricAsyncOperationCallback_FWD_DEFINED__
#define __IFabricAsyncOperationCallback_FWD_DEFINED__
typedef interface IFabricAsyncOperationCallback IFabricAsyncOperationCallback;

#endif 	/* __IFabricAsyncOperationCallback_FWD_DEFINED__ */


#ifndef __IFabricAsyncOperationContext_FWD_DEFINED__
#define __IFabricAsyncOperationContext_FWD_DEFINED__
typedef interface IFabricAsyncOperationContext IFabricAsyncOperationContext;

#endif 	/* __IFabricAsyncOperationContext_FWD_DEFINED__ */


#ifndef __IFabricStringResult_FWD_DEFINED__
#define __IFabricStringResult_FWD_DEFINED__
typedef interface IFabricStringResult IFabricStringResult;

#endif 	/* __IFabricStringResult_FWD_DEFINED__ */


#ifndef __IFabricStringListResult_FWD_DEFINED__
#define __IFabricStringListResult_FWD_DEFINED__
typedef interface IFabricStringListResult IFabricStringListResult;

#endif 	/* __IFabricStringListResult_FWD_DEFINED__ */


#ifndef __IFabricGetReplicatorStatusResult_FWD_DEFINED__
#define __IFabricGetReplicatorStatusResult_FWD_DEFINED__
typedef interface IFabricGetReplicatorStatusResult IFabricGetReplicatorStatusResult;

#endif 	/* __IFabricGetReplicatorStatusResult_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricTypes.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabriccommon_0000_0000 */
/* [local] */ 

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif







extern RPC_IF_HANDLE __MIDL_itf_fabriccommon_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabriccommon_0000_0000_v0_0_s_ifspec;


#ifndef __FabricCommonLib_LIBRARY_DEFINED__
#define __FabricCommonLib_LIBRARY_DEFINED__

/* library FabricCommonLib */
/* [version][helpstring][uuid] */ 


#pragma pack(push, 8)






#pragma pack(pop)

EXTERN_C const IID LIBID_FabricCommonLib;

#ifndef __IFabricAsyncOperationCallback_INTERFACE_DEFINED__
#define __IFabricAsyncOperationCallback_INTERFACE_DEFINED__

/* interface IFabricAsyncOperationCallback */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricAsyncOperationCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("86f08d7e-14dd-4575-8489-b1d5d679029c")
    IFabricAsyncOperationCallback : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE Invoke( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricAsyncOperationCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricAsyncOperationCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricAsyncOperationCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricAsyncOperationCallback * This);
        
        void ( STDMETHODCALLTYPE *Invoke )( 
            IFabricAsyncOperationCallback * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricAsyncOperationCallbackVtbl;

    interface IFabricAsyncOperationCallback
    {
        CONST_VTBL struct IFabricAsyncOperationCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricAsyncOperationCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricAsyncOperationCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricAsyncOperationCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricAsyncOperationCallback_Invoke(This,context)	\
    ( (This)->lpVtbl -> Invoke(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricAsyncOperationCallback_INTERFACE_DEFINED__ */


#ifndef __IFabricAsyncOperationContext_INTERFACE_DEFINED__
#define __IFabricAsyncOperationContext_INTERFACE_DEFINED__

/* interface IFabricAsyncOperationContext */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricAsyncOperationContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("841720bf-c9e8-4e6f-9c3f-6b7f4ac73bcd")
    IFabricAsyncOperationContext : public IUnknown
    {
    public:
        virtual BOOLEAN STDMETHODCALLTYPE IsCompleted( void) = 0;
        
        virtual BOOLEAN STDMETHODCALLTYPE CompletedSynchronously( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE get_Callback( 
            /* [retval][out] */ IFabricAsyncOperationCallback **callback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricAsyncOperationContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricAsyncOperationContext * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricAsyncOperationContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricAsyncOperationContext * This);
        
        BOOLEAN ( STDMETHODCALLTYPE *IsCompleted )( 
            IFabricAsyncOperationContext * This);
        
        BOOLEAN ( STDMETHODCALLTYPE *CompletedSynchronously )( 
            IFabricAsyncOperationContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *get_Callback )( 
            IFabricAsyncOperationContext * This,
            /* [retval][out] */ IFabricAsyncOperationCallback **callback);
        
        HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IFabricAsyncOperationContext * This);
        
        END_INTERFACE
    } IFabricAsyncOperationContextVtbl;

    interface IFabricAsyncOperationContext
    {
        CONST_VTBL struct IFabricAsyncOperationContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricAsyncOperationContext_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricAsyncOperationContext_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricAsyncOperationContext_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricAsyncOperationContext_IsCompleted(This)	\
    ( (This)->lpVtbl -> IsCompleted(This) ) 

#define IFabricAsyncOperationContext_CompletedSynchronously(This)	\
    ( (This)->lpVtbl -> CompletedSynchronously(This) ) 

#define IFabricAsyncOperationContext_get_Callback(This,callback)	\
    ( (This)->lpVtbl -> get_Callback(This,callback) ) 

#define IFabricAsyncOperationContext_Cancel(This)	\
    ( (This)->lpVtbl -> Cancel(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricAsyncOperationContext_INTERFACE_DEFINED__ */


#ifndef __IFabricStringResult_INTERFACE_DEFINED__
#define __IFabricStringResult_INTERFACE_DEFINED__

/* interface IFabricStringResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStringResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4ae69614-7d0f-4cd4-b836-23017000d132")
    IFabricStringResult : public IUnknown
    {
    public:
        virtual LPCWSTR STDMETHODCALLTYPE get_String( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStringResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStringResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStringResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStringResult * This);
        
        LPCWSTR ( STDMETHODCALLTYPE *get_String )( 
            IFabricStringResult * This);
        
        END_INTERFACE
    } IFabricStringResultVtbl;

    interface IFabricStringResult
    {
        CONST_VTBL struct IFabricStringResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStringResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStringResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStringResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStringResult_get_String(This)	\
    ( (This)->lpVtbl -> get_String(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStringResult_INTERFACE_DEFINED__ */


#ifndef __IFabricStringListResult_INTERFACE_DEFINED__
#define __IFabricStringListResult_INTERFACE_DEFINED__

/* interface IFabricStringListResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricStringListResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("afab1c53-757b-4b0e-8b7e-237aeee6bfe9")
    IFabricStringListResult : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStrings( 
            /* [out] */ ULONG *itemCount,
            /* [retval][out] */ const LPCWSTR **bufferedItems) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricStringListResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricStringListResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricStringListResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricStringListResult * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStrings )( 
            IFabricStringListResult * This,
            /* [out] */ ULONG *itemCount,
            /* [retval][out] */ const LPCWSTR **bufferedItems);
        
        END_INTERFACE
    } IFabricStringListResultVtbl;

    interface IFabricStringListResult
    {
        CONST_VTBL struct IFabricStringListResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricStringListResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricStringListResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricStringListResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricStringListResult_GetStrings(This,itemCount,bufferedItems)	\
    ( (This)->lpVtbl -> GetStrings(This,itemCount,bufferedItems) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricStringListResult_INTERFACE_DEFINED__ */


#ifndef __IFabricGetReplicatorStatusResult_INTERFACE_DEFINED__
#define __IFabricGetReplicatorStatusResult_INTERFACE_DEFINED__

/* interface IFabricGetReplicatorStatusResult */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricGetReplicatorStatusResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30E10C61-A710-4F99-A623-BB1403265186")
    IFabricGetReplicatorStatusResult : public IUnknown
    {
    public:
        virtual const FABRIC_REPLICATOR_STATUS_QUERY_RESULT *STDMETHODCALLTYPE get_ReplicatorStatus( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricGetReplicatorStatusResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricGetReplicatorStatusResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricGetReplicatorStatusResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricGetReplicatorStatusResult * This);
        
        const FABRIC_REPLICATOR_STATUS_QUERY_RESULT *( STDMETHODCALLTYPE *get_ReplicatorStatus )( 
            IFabricGetReplicatorStatusResult * This);
        
        END_INTERFACE
    } IFabricGetReplicatorStatusResultVtbl;

    interface IFabricGetReplicatorStatusResult
    {
        CONST_VTBL struct IFabricGetReplicatorStatusResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricGetReplicatorStatusResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricGetReplicatorStatusResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricGetReplicatorStatusResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricGetReplicatorStatusResult_get_ReplicatorStatus(This)	\
    ( (This)->lpVtbl -> get_ReplicatorStatus(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricGetReplicatorStatusResult_INTERFACE_DEFINED__ */



#ifndef __FabricCommonModule_MODULE_DEFINED__
#define __FabricCommonModule_MODULE_DEFINED__


/* module FabricCommonModule */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT FabricGetLastErrorMessage( 
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **message);

/* [entry] */ HRESULT FabricEncryptText( 
    /* [in] */ __RPC__in LPCWSTR text,
    /* [in] */ __RPC__in LPCWSTR certThumbprint,
    /* [in] */ __RPC__in LPCWSTR certStoreName,
    /* [in] */ FABRIC_X509_STORE_LOCATION certStoreLocation,
    /* [in] */ __RPC__in LPCSTR algorithmOid,
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **encryptedValue);

/* [entry] */ HRESULT FabricEncryptText2( 
    /* [in] */ __RPC__in LPCWSTR text,
    /* [in] */ __RPC__in LPCWSTR certFilePath,
    /* [in] */ __RPC__in LPCSTR algorithmOid,
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **encryptedValue);

/* [entry] */ HRESULT FabricDecryptText( 
    /* [in] */ __RPC__in LPCWSTR encryptedText,
    /* [in] */ FABRIC_X509_STORE_LOCATION certStoreLocation,
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **decryptedText);

/* [entry] */ HRESULT FabricEncryptValue( 
    /* [in] */ __RPC__in LPCWSTR certThumbprint,
    /* [in] */ __RPC__in LPCWSTR certStoreName,
    /* [in] */ __RPC__in LPCWSTR text,
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **encryptedValue);

/* [entry] */ HRESULT FabricDecryptValue( 
    /* [in] */ __RPC__in LPCWSTR encryptedValue,
    /* [retval][out] */ __RPC__deref_out_opt IFabricStringResult **decryptedValue);

#endif /* __FabricCommonModule_MODULE_DEFINED__ */
#endif /* __FabricCommonLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


