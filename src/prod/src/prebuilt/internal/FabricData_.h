// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------



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

#ifndef __fabricdata__h__
#define __fabricdata__h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFabricBtree_FWD_DEFINED__
#define __IFabricBtree_FWD_DEFINED__
typedef interface IFabricBtree IFabricBtree;

#endif 	/* __IFabricBtree_FWD_DEFINED__ */


#ifndef __IFabricBtreeKey_FWD_DEFINED__
#define __IFabricBtreeKey_FWD_DEFINED__
typedef interface IFabricBtreeKey IFabricBtreeKey;

#endif 	/* __IFabricBtreeKey_FWD_DEFINED__ */


#ifndef __IFabricBtreeValue_FWD_DEFINED__
#define __IFabricBtreeValue_FWD_DEFINED__
typedef interface IFabricBtreeValue IFabricBtreeValue;

#endif 	/* __IFabricBtreeValue_FWD_DEFINED__ */


#ifndef __IFabricBtreeOperation_FWD_DEFINED__
#define __IFabricBtreeOperation_FWD_DEFINED__
typedef interface IFabricBtreeOperation IFabricBtreeOperation;

#endif 	/* __IFabricBtreeOperation_FWD_DEFINED__ */


#ifndef __IFabricBtreeScanPosition_FWD_DEFINED__
#define __IFabricBtreeScanPosition_FWD_DEFINED__
typedef interface IFabricBtreeScanPosition IFabricBtreeScanPosition;

#endif 	/* __IFabricBtreeScanPosition_FWD_DEFINED__ */


#ifndef __IFabricBTreeScan_FWD_DEFINED__
#define __IFabricBTreeScan_FWD_DEFINED__
typedef interface IFabricBTreeScan IFabricBTreeScan;

#endif 	/* __IFabricBTreeScan_FWD_DEFINED__ */


#ifndef __FabricBtree_FWD_DEFINED__
#define __FabricBtree_FWD_DEFINED__

#ifdef __cplusplus
typedef class FabricBtree FabricBtree;
#else
typedef struct FabricBtree FabricBtree;
#endif /* __cplusplus */

#endif 	/* __FabricBtree_FWD_DEFINED__ */


#ifndef __IFabricBtreeKey_FWD_DEFINED__
#define __IFabricBtreeKey_FWD_DEFINED__
typedef interface IFabricBtreeKey IFabricBtreeKey;

#endif 	/* __IFabricBtreeKey_FWD_DEFINED__ */


#ifndef __IFabricBtreeValue_FWD_DEFINED__
#define __IFabricBtreeValue_FWD_DEFINED__
typedef interface IFabricBtreeValue IFabricBtreeValue;

#endif 	/* __IFabricBtreeValue_FWD_DEFINED__ */


#ifndef __IFabricBtreeOperation_FWD_DEFINED__
#define __IFabricBtreeOperation_FWD_DEFINED__
typedef interface IFabricBtreeOperation IFabricBtreeOperation;

#endif 	/* __IFabricBtreeOperation_FWD_DEFINED__ */


#ifndef __IFabricBtreeScanPosition_FWD_DEFINED__
#define __IFabricBtreeScanPosition_FWD_DEFINED__
typedef interface IFabricBtreeScanPosition IFabricBtreeScanPosition;

#endif 	/* __IFabricBtreeScanPosition_FWD_DEFINED__ */


#ifndef __IFabricBTreeScan_FWD_DEFINED__
#define __IFabricBTreeScan_FWD_DEFINED__
typedef interface IFabricBTreeScan IFabricBTreeScan;

#endif 	/* __IFabricBTreeScan_FWD_DEFINED__ */


#ifndef __IFabricBtree_FWD_DEFINED__
#define __IFabricBtree_FWD_DEFINED__
typedef interface IFabricBtree IFabricBtree;

#endif 	/* __IFabricBtree_FWD_DEFINED__ */


/* header files for imported files */
#include "Unknwn.h"
#include "FabricTypes.h"
#include "FabricCommon.h"
#include "FabricRuntime.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fabricdata__0000_0000 */
/* [local] */ 

#if ( _MSC_VER >= 1020 )
#pragma once
#endif
typedef /* [uuid][v1_enum] */  DECLSPEC_UUID("e2d0ab74-f307-473d-8f99-20b387a240b8") 
enum FABRIC_BTREE_DATA_TYPE
    {
        FABRIC_BTREE_DATA_TYPE_INVALID	= 0,
        FABRIC_BTREE_DATA_TYPE_BINARY	= ( FABRIC_BTREE_DATA_TYPE_INVALID + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_BYTE	= ( FABRIC_BTREE_DATA_TYPE_BINARY + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_WCHAR	= ( FABRIC_BTREE_DATA_TYPE_BYTE + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_DATETIME	= ( FABRIC_BTREE_DATA_TYPE_WCHAR + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_TIMESPAN	= ( FABRIC_BTREE_DATA_TYPE_DATETIME + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_SHORT	= ( FABRIC_BTREE_DATA_TYPE_TIMESPAN + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_USHORT	= ( FABRIC_BTREE_DATA_TYPE_SHORT + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_INT	= ( FABRIC_BTREE_DATA_TYPE_USHORT + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_UINT	= ( FABRIC_BTREE_DATA_TYPE_INT + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_LONGLONG	= ( FABRIC_BTREE_DATA_TYPE_UINT + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_ULONGLONG	= ( FABRIC_BTREE_DATA_TYPE_LONGLONG + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_SINGLE	= ( FABRIC_BTREE_DATA_TYPE_ULONGLONG + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_DOUBLE	= ( FABRIC_BTREE_DATA_TYPE_SINGLE + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_GUID	= ( FABRIC_BTREE_DATA_TYPE_DOUBLE + 1 ) ,
        FABRIC_BTREE_DATA_TYPE_WSTRING	= ( FABRIC_BTREE_DATA_TYPE_GUID + 1 ) 
    } 	FABRIC_BTREE_DATA_TYPE;

typedef /* [uuid][v1_enum] */  DECLSPEC_UUID("5284f24f-f62f-4c33-894a-421d5fa35860") 
enum FABRIC_BTREE_OPERATION_TYPE
    {
        FABRIC_BTREE_OPERATION_INVALID	= 0,
        FABRIC_BTREE_OPERATION_INSERT	= ( FABRIC_BTREE_OPERATION_INVALID + 1 ) ,
        FABRIC_BTREE_OPERATION_DELETE	= ( FABRIC_BTREE_OPERATION_INSERT + 1 ) ,
        FABRIC_BTREE_OPERATION_UPDATE	= ( FABRIC_BTREE_OPERATION_DELETE + 1 ) ,
        FABRIC_BTREE_OPERATION_PARTIAL_UPDATE	= ( FABRIC_BTREE_OPERATION_UPDATE + 1 ) ,
        FABRIC_BTREE_OPERATION_ERASE	= ( FABRIC_BTREE_OPERATION_PARTIAL_UPDATE + 1 ) 
    } 	FABRIC_BTREE_OPERATION_TYPE;











extern RPC_IF_HANDLE __MIDL_itf_fabricdata__0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fabricdata__0000_0000_v0_0_s_ifspec;


#ifndef __FabricBTreeTypeLib_LIBRARY_DEFINED__
#define __FabricBTreeTypeLib_LIBRARY_DEFINED__

/* library FabricBTreeTypeLib */
/* [version][uuid] */ 


#pragma pack(push, 8)
typedef struct BTREE_KEY_COMPARISON_DESCRIPTION
    {
    FABRIC_BTREE_DATA_TYPE KeyDataType;
    INT LCID;
    ULONG MaximumKeySizeInBytes;
    BOOLEAN IsFixedLengthKey;
    void *reserved;
    } 	BTREE_KEY_COMPARISON_DESCRIPTION;

typedef struct BTREE_STORAGE_CONFIGURATION_DESCRIPTION
    {
    LPCWSTR Path;
    ULONG MaximumStorageInMB;
    BOOLEAN IsVolatile;
    ULONG MaximumMemoryInMB;
    ULONG MaximumPageSizeInKB;
    BOOLEAN StoreDataInline;
    ULONG RetriesBeforeTimeout;
    void *reserved;
    } 	BTREE_STORAGE_CONFIGURATION_DESCRIPTION;

typedef struct BTREE_CONFIGURATION_DESCRIPTION
    {
    GUID PartitionId;
    FABRIC_REPLICA_ID ReplicaId;
    const BTREE_STORAGE_CONFIGURATION_DESCRIPTION *StorageConfiguration;
    const BTREE_KEY_COMPARISON_DESCRIPTION *KeyComparison;
    void *reserved;
    } 	BTREE_CONFIGURATION_DESCRIPTION;

typedef struct BTREE_STATISTICS
    {
    INT MemoryUsageInBytes;
    INT StorageUsageInBytes;
    INT PageCount;
    INT RecordCount;
    void *reserved;
    } 	BTREE_STATISTICS;









#pragma pack(pop)

EXTERN_C const IID LIBID_FabricBTreeTypeLib;

#ifndef __IFabricBtree_INTERFACE_DEFINED__
#define __IFabricBtree_INTERFACE_DEFINED__

/* interface IFabricBtree */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBtree;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("16dbb404-3127-4c8e-995b-3b3d9a8ea099")
    IFabricBtree : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ const BTREE_CONFIGURATION_DESCRIPTION *configuration,
            /* [in] */ BOOLEAN storageExists,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ BOOLEAN eraseStorage,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginInsert( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndInsert( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginInsertWithOutput( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndInsertWithOutput( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUpsert( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpsert( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUpdate( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpdate( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginConditionalUpdate( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ ULONG conditionalCheckOffset,
            /* [in] */ IFabricBtreeValue *conditionalCheckValue,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndConditionalUpdate( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *conditionCheckSuccess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginUpdateWithOutput( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndUpdateWithOutput( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginConditionalPartialUpdate( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ ULONG partialUpdateOffset,
            /* [in] */ IFabricBtreeValue *partialUpdateValue,
            /* [in] */ ULONG conditionalCheckOffset,
            /* [in] */ IFabricOperationData *conditionalCheckValue,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndConditionalPartialUpdate( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *conditionCheckSuccess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDelete( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDelete( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *deleteSuccess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginConditionalDelete( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ ULONG conditionalCheckOffset,
            /* [in] */ IFabricOperationData *conditionalCheckValue,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndConditionalDelete( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *conditionCheckSuccess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDeleteWithOutput( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDeleteWithOutput( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginSeek( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndSeek( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginPartialSeek( 
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ ULONG partialSeekOffset,
            /* [in] */ ULONG partialSeekLength,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndPartialSeek( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginErase( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndErase( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [retval][out] */ BOOLEAN *eraseSuccess) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateScan( 
            /* [retval][out] */ IFabricBTreeScan **scan) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginOperationStable( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER stableOsn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOperationStable( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *actualStableOsn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCheckpoint( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkpointOsn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCheckpoint( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatistics( 
            /* [retval][out] */ const BTREE_STATISTICS **statistics) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginApplyWithOutput( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricOperationData *operation,
            /* [in] */ BOOLEAN decodeOnly,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndApplyWithOutput( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeOperation **operation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCopyState( 
            /* [in] */ FABRIC_SEQUENCE_NUMBER upToOsn,
            /* [retval][out] */ IFabricOperationDataStream **copyStateStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastCommittedSequenceNumber( 
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginApplyCopyData( 
            /* [in] */ IFabricOperationData *copyStreamData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndApplyCopyData( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBtreeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBtree * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBtree * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBtree * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricBtree * This,
            /* [in] */ const BTREE_CONFIGURATION_DESCRIPTION *configuration,
            /* [in] */ BOOLEAN storageExists,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricBtree * This,
            /* [in] */ BOOLEAN eraseStorage,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IFabricBtree * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginInsert )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndInsert )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes);
        
        HRESULT ( STDMETHODCALLTYPE *BeginInsertWithOutput )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndInsertWithOutput )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpsert )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpsert )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpdate )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpdate )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes);
        
        HRESULT ( STDMETHODCALLTYPE *BeginConditionalUpdate )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ ULONG conditionalCheckOffset,
            /* [in] */ IFabricBtreeValue *conditionalCheckValue,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndConditionalUpdate )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *conditionCheckSuccess);
        
        HRESULT ( STDMETHODCALLTYPE *BeginUpdateWithOutput )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricBtreeValue *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndUpdateWithOutput )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        HRESULT ( STDMETHODCALLTYPE *BeginConditionalPartialUpdate )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ ULONG partialUpdateOffset,
            /* [in] */ IFabricBtreeValue *partialUpdateValue,
            /* [in] */ ULONG conditionalCheckOffset,
            /* [in] */ IFabricOperationData *conditionalCheckValue,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndConditionalPartialUpdate )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *conditionCheckSuccess);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDelete )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDelete )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *deleteSuccess);
        
        HRESULT ( STDMETHODCALLTYPE *BeginConditionalDelete )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ ULONG conditionalCheckOffset,
            /* [in] */ IFabricOperationData *conditionalCheckValue,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndConditionalDelete )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ BOOLEAN *conditionCheckSuccess);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDeleteWithOutput )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndDeleteWithOutput )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [out] */ IFabricOperationData **undoBytes,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        HRESULT ( STDMETHODCALLTYPE *BeginSeek )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndSeek )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        HRESULT ( STDMETHODCALLTYPE *BeginPartialSeek )( 
            IFabricBtree * This,
            /* [in] */ IFabricBtreeKey *key,
            /* [in] */ ULONG partialSeekOffset,
            /* [in] */ ULONG partialSeekLength,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndPartialSeek )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        HRESULT ( STDMETHODCALLTYPE *BeginErase )( 
            IFabricBtree * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndErase )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [out] */ IFabricOperationData **redoBytes,
            /* [retval][out] */ BOOLEAN *eraseSuccess);
        
        HRESULT ( STDMETHODCALLTYPE *CreateScan )( 
            IFabricBtree * This,
            /* [retval][out] */ IFabricBTreeScan **scan);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOperationStable )( 
            IFabricBtree * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER stableOsn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOperationStable )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *actualStableOsn);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCheckpoint )( 
            IFabricBtree * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkpointOsn,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndCheckpoint )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            IFabricBtree * This,
            /* [retval][out] */ const BTREE_STATISTICS **statistics);
        
        HRESULT ( STDMETHODCALLTYPE *BeginApplyWithOutput )( 
            IFabricBtree * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER osn,
            /* [in] */ IFabricOperationData *operation,
            /* [in] */ BOOLEAN decodeOnly,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndApplyWithOutput )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeOperation **operation);
        
        HRESULT ( STDMETHODCALLTYPE *GetCopyState )( 
            IFabricBtree * This,
            /* [in] */ FABRIC_SEQUENCE_NUMBER upToOsn,
            /* [retval][out] */ IFabricOperationDataStream **copyStateStream);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastCommittedSequenceNumber )( 
            IFabricBtree * This,
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber);
        
        HRESULT ( STDMETHODCALLTYPE *BeginApplyCopyData )( 
            IFabricBtree * This,
            /* [in] */ IFabricOperationData *copyStreamData,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndApplyCopyData )( 
            IFabricBtree * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        END_INTERFACE
    } IFabricBtreeVtbl;

    interface IFabricBtree
    {
        CONST_VTBL struct IFabricBtreeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBtree_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBtree_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBtree_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBtree_BeginOpen(This,configuration,storageExists,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,configuration,storageExists,callback,context) ) 

#define IFabricBtree_EndOpen(This,context)	\
    ( (This)->lpVtbl -> EndOpen(This,context) ) 

#define IFabricBtree_BeginClose(This,eraseStorage,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,eraseStorage,callback,context) ) 

#define IFabricBtree_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricBtree_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define IFabricBtree_BeginInsert(This,key,value,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginInsert(This,key,value,osn,callback,context) ) 

#define IFabricBtree_EndInsert(This,context,redoBytes,undoBytes)	\
    ( (This)->lpVtbl -> EndInsert(This,context,redoBytes,undoBytes) ) 

#define IFabricBtree_BeginInsertWithOutput(This,key,value,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginInsertWithOutput(This,key,value,osn,callback,context) ) 

#define IFabricBtree_EndInsertWithOutput(This,context,redoBytes,undoBytes,value)	\
    ( (This)->lpVtbl -> EndInsertWithOutput(This,context,redoBytes,undoBytes,value) ) 

#define IFabricBtree_BeginUpsert(This,key,value,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginUpsert(This,key,value,osn,callback,context) ) 

#define IFabricBtree_EndUpsert(This,context,redoBytes,undoBytes,value)	\
    ( (This)->lpVtbl -> EndUpsert(This,context,redoBytes,undoBytes,value) ) 

#define IFabricBtree_BeginUpdate(This,key,value,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginUpdate(This,key,value,osn,callback,context) ) 

#define IFabricBtree_EndUpdate(This,context,redoBytes,undoBytes)	\
    ( (This)->lpVtbl -> EndUpdate(This,context,redoBytes,undoBytes) ) 

#define IFabricBtree_BeginConditionalUpdate(This,key,value,conditionalCheckOffset,conditionalCheckValue,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginConditionalUpdate(This,key,value,conditionalCheckOffset,conditionalCheckValue,osn,callback,context) ) 

#define IFabricBtree_EndConditionalUpdate(This,context,redoBytes,undoBytes,conditionCheckSuccess)	\
    ( (This)->lpVtbl -> EndConditionalUpdate(This,context,redoBytes,undoBytes,conditionCheckSuccess) ) 

#define IFabricBtree_BeginUpdateWithOutput(This,key,value,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginUpdateWithOutput(This,key,value,osn,callback,context) ) 

#define IFabricBtree_EndUpdateWithOutput(This,context,redoBytes,undoBytes,value)	\
    ( (This)->lpVtbl -> EndUpdateWithOutput(This,context,redoBytes,undoBytes,value) ) 

#define IFabricBtree_BeginConditionalPartialUpdate(This,key,partialUpdateOffset,partialUpdateValue,conditionalCheckOffset,conditionalCheckValue,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginConditionalPartialUpdate(This,key,partialUpdateOffset,partialUpdateValue,conditionalCheckOffset,conditionalCheckValue,osn,callback,context) ) 

#define IFabricBtree_EndConditionalPartialUpdate(This,context,redoBytes,undoBytes,conditionCheckSuccess)	\
    ( (This)->lpVtbl -> EndConditionalPartialUpdate(This,context,redoBytes,undoBytes,conditionCheckSuccess) ) 

#define IFabricBtree_BeginDelete(This,key,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginDelete(This,key,osn,callback,context) ) 

#define IFabricBtree_EndDelete(This,context,redoBytes,undoBytes,deleteSuccess)	\
    ( (This)->lpVtbl -> EndDelete(This,context,redoBytes,undoBytes,deleteSuccess) ) 

#define IFabricBtree_BeginConditionalDelete(This,key,conditionalCheckOffset,conditionalCheckValue,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginConditionalDelete(This,key,conditionalCheckOffset,conditionalCheckValue,osn,callback,context) ) 

#define IFabricBtree_EndConditionalDelete(This,context,redoBytes,undoBytes,conditionCheckSuccess)	\
    ( (This)->lpVtbl -> EndConditionalDelete(This,context,redoBytes,undoBytes,conditionCheckSuccess) ) 

#define IFabricBtree_BeginDeleteWithOutput(This,key,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginDeleteWithOutput(This,key,osn,callback,context) ) 

#define IFabricBtree_EndDeleteWithOutput(This,context,redoBytes,undoBytes,value)	\
    ( (This)->lpVtbl -> EndDeleteWithOutput(This,context,redoBytes,undoBytes,value) ) 

#define IFabricBtree_BeginSeek(This,key,callback,context)	\
    ( (This)->lpVtbl -> BeginSeek(This,key,callback,context) ) 

#define IFabricBtree_EndSeek(This,context,value)	\
    ( (This)->lpVtbl -> EndSeek(This,context,value) ) 

#define IFabricBtree_BeginPartialSeek(This,key,partialSeekOffset,partialSeekLength,callback,context)	\
    ( (This)->lpVtbl -> BeginPartialSeek(This,key,partialSeekOffset,partialSeekLength,callback,context) ) 

#define IFabricBtree_EndPartialSeek(This,context,value)	\
    ( (This)->lpVtbl -> EndPartialSeek(This,context,value) ) 

#define IFabricBtree_BeginErase(This,osn,callback,context)	\
    ( (This)->lpVtbl -> BeginErase(This,osn,callback,context) ) 

#define IFabricBtree_EndErase(This,context,redoBytes,eraseSuccess)	\
    ( (This)->lpVtbl -> EndErase(This,context,redoBytes,eraseSuccess) ) 

#define IFabricBtree_CreateScan(This,scan)	\
    ( (This)->lpVtbl -> CreateScan(This,scan) ) 

#define IFabricBtree_BeginOperationStable(This,stableOsn,callback,context)	\
    ( (This)->lpVtbl -> BeginOperationStable(This,stableOsn,callback,context) ) 

#define IFabricBtree_EndOperationStable(This,context,actualStableOsn)	\
    ( (This)->lpVtbl -> EndOperationStable(This,context,actualStableOsn) ) 

#define IFabricBtree_BeginCheckpoint(This,checkpointOsn,callback,context)	\
    ( (This)->lpVtbl -> BeginCheckpoint(This,checkpointOsn,callback,context) ) 

#define IFabricBtree_EndCheckpoint(This,context)	\
    ( (This)->lpVtbl -> EndCheckpoint(This,context) ) 

#define IFabricBtree_GetStatistics(This,statistics)	\
    ( (This)->lpVtbl -> GetStatistics(This,statistics) ) 

#define IFabricBtree_BeginApplyWithOutput(This,osn,operation,decodeOnly,callback,context)	\
    ( (This)->lpVtbl -> BeginApplyWithOutput(This,osn,operation,decodeOnly,callback,context) ) 

#define IFabricBtree_EndApplyWithOutput(This,context,operation)	\
    ( (This)->lpVtbl -> EndApplyWithOutput(This,context,operation) ) 

#define IFabricBtree_GetCopyState(This,upToOsn,copyStateStream)	\
    ( (This)->lpVtbl -> GetCopyState(This,upToOsn,copyStateStream) ) 

#define IFabricBtree_GetLastCommittedSequenceNumber(This,lastSequenceNumber)	\
    ( (This)->lpVtbl -> GetLastCommittedSequenceNumber(This,lastSequenceNumber) ) 

#define IFabricBtree_BeginApplyCopyData(This,copyStreamData,callback,context)	\
    ( (This)->lpVtbl -> BeginApplyCopyData(This,copyStreamData,callback,context) ) 

#define IFabricBtree_EndApplyCopyData(This,context)	\
    ( (This)->lpVtbl -> EndApplyCopyData(This,context) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBtree_INTERFACE_DEFINED__ */


#ifndef __IFabricBtreeKey_INTERFACE_DEFINED__
#define __IFabricBtreeKey_INTERFACE_DEFINED__

/* interface IFabricBtreeKey */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBtreeKey;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7cb23eac-731c-4b91-a95c-d84050c1ecc5")
    IFabricBtreeKey : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ ULONG *count,
            /* [retval][out] */ const BYTE **buffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBtreeKeyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBtreeKey * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBtreeKey * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBtreeKey * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IFabricBtreeKey * This,
            /* [out] */ ULONG *count,
            /* [retval][out] */ const BYTE **buffer);
        
        END_INTERFACE
    } IFabricBtreeKeyVtbl;

    interface IFabricBtreeKey
    {
        CONST_VTBL struct IFabricBtreeKeyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBtreeKey_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBtreeKey_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBtreeKey_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBtreeKey_GetBytes(This,count,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,count,buffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBtreeKey_INTERFACE_DEFINED__ */


#ifndef __IFabricBtreeValue_INTERFACE_DEFINED__
#define __IFabricBtreeValue_INTERFACE_DEFINED__

/* interface IFabricBtreeValue */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBtreeValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9f5b60af-3d9d-4d45-9094-b0b3c8188dc5")
    IFabricBtreeValue : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ ULONG *count,
            /* [retval][out] */ const BYTE **buffer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBtreeValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBtreeValue * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBtreeValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBtreeValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IFabricBtreeValue * This,
            /* [out] */ ULONG *count,
            /* [retval][out] */ const BYTE **buffer);
        
        END_INTERFACE
    } IFabricBtreeValueVtbl;

    interface IFabricBtreeValue
    {
        CONST_VTBL struct IFabricBtreeValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBtreeValue_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBtreeValue_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBtreeValue_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBtreeValue_GetBytes(This,count,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,count,buffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBtreeValue_INTERFACE_DEFINED__ */


#ifndef __IFabricBtreeOperation_INTERFACE_DEFINED__
#define __IFabricBtreeOperation_INTERFACE_DEFINED__

/* interface IFabricBtreeOperation */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBtreeOperation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1a3cc156-c5bd-41f8-b9bf-270d63559318")
    IFabricBtreeOperation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOperationType( 
            /* [retval][out] */ FABRIC_BTREE_OPERATION_TYPE *operationType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKey( 
            /* [retval][out] */ IFabricBtreeKey **key) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConditionalValue( 
            /* [retval][out] */ IFabricBtreeValue **conditionalValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConditionalOffset( 
            /* [retval][out] */ ULONG *conditionalOffset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPartialUpdateOffset( 
            /* [retval][out] */ ULONG *partialUpdateOffset) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBtreeOperationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBtreeOperation * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBtreeOperation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBtreeOperation * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetOperationType )( 
            IFabricBtreeOperation * This,
            /* [retval][out] */ FABRIC_BTREE_OPERATION_TYPE *operationType);
        
        HRESULT ( STDMETHODCALLTYPE *GetKey )( 
            IFabricBtreeOperation * This,
            /* [retval][out] */ IFabricBtreeKey **key);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            IFabricBtreeOperation * This,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        HRESULT ( STDMETHODCALLTYPE *GetConditionalValue )( 
            IFabricBtreeOperation * This,
            /* [retval][out] */ IFabricBtreeValue **conditionalValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetConditionalOffset )( 
            IFabricBtreeOperation * This,
            /* [retval][out] */ ULONG *conditionalOffset);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartialUpdateOffset )( 
            IFabricBtreeOperation * This,
            /* [retval][out] */ ULONG *partialUpdateOffset);
        
        END_INTERFACE
    } IFabricBtreeOperationVtbl;

    interface IFabricBtreeOperation
    {
        CONST_VTBL struct IFabricBtreeOperationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBtreeOperation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBtreeOperation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBtreeOperation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBtreeOperation_GetOperationType(This,operationType)	\
    ( (This)->lpVtbl -> GetOperationType(This,operationType) ) 

#define IFabricBtreeOperation_GetKey(This,key)	\
    ( (This)->lpVtbl -> GetKey(This,key) ) 

#define IFabricBtreeOperation_GetValue(This,value)	\
    ( (This)->lpVtbl -> GetValue(This,value) ) 

#define IFabricBtreeOperation_GetConditionalValue(This,conditionalValue)	\
    ( (This)->lpVtbl -> GetConditionalValue(This,conditionalValue) ) 

#define IFabricBtreeOperation_GetConditionalOffset(This,conditionalOffset)	\
    ( (This)->lpVtbl -> GetConditionalOffset(This,conditionalOffset) ) 

#define IFabricBtreeOperation_GetPartialUpdateOffset(This,partialUpdateOffset)	\
    ( (This)->lpVtbl -> GetPartialUpdateOffset(This,partialUpdateOffset) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBtreeOperation_INTERFACE_DEFINED__ */


#ifndef __IFabricBtreeScanPosition_INTERFACE_DEFINED__
#define __IFabricBtreeScanPosition_INTERFACE_DEFINED__

/* interface IFabricBtreeScanPosition */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBtreeScanPosition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ab74fcdb-f2e1-45a8-870a-31b3f8e50f96")
    IFabricBtreeScanPosition : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetKey( 
            /* [retval][out] */ IFabricBtreeKey **key) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [retval][out] */ IFabricBtreeValue **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBtreeScanPositionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBtreeScanPosition * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBtreeScanPosition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBtreeScanPosition * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetKey )( 
            IFabricBtreeScanPosition * This,
            /* [retval][out] */ IFabricBtreeKey **key);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            IFabricBtreeScanPosition * This,
            /* [retval][out] */ IFabricBtreeValue **value);
        
        END_INTERFACE
    } IFabricBtreeScanPositionVtbl;

    interface IFabricBtreeScanPosition
    {
        CONST_VTBL struct IFabricBtreeScanPositionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBtreeScanPosition_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBtreeScanPosition_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBtreeScanPosition_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBtreeScanPosition_GetKey(This,key)	\
    ( (This)->lpVtbl -> GetKey(This,key) ) 

#define IFabricBtreeScanPosition_GetValue(This,value)	\
    ( (This)->lpVtbl -> GetValue(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBtreeScanPosition_INTERFACE_DEFINED__ */


#ifndef __IFabricBTreeScan_INTERFACE_DEFINED__
#define __IFabricBTreeScan_INTERFACE_DEFINED__

/* interface IFabricBTreeScan */
/* [uuid][local][object] */ 


EXTERN_C const IID IID_IFabricBTreeScan;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("404c7900-99ed-49ee-9388-5063c185b741")
    IFabricBTreeScan : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ IFabricBtreeKey *beginKey,
            /* [in] */ IFabricBtreeKey *endKey,
            /* [in] */ IFabricBtreeKey *keyPrefixMatch,
            /* [in] */ BOOLEAN isValueNeeded,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeKey **keyBeforeStartPosition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginReset( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndReset( 
            /* [in] */ IFabricAsyncOperationContext *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginMoveNext( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndMoveNext( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeScanPosition **scanPosition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginPeekNextKey( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndPeekNextKey( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeKey **nextKey) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IFabricBTreeScanVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFabricBTreeScan * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFabricBTreeScan * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFabricBTreeScan * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginOpen )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricBtreeKey *beginKey,
            /* [in] */ IFabricBtreeKey *endKey,
            /* [in] */ IFabricBtreeKey *keyPrefixMatch,
            /* [in] */ BOOLEAN isValueNeeded,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndOpen )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeKey **keyBeforeStartPosition);
        
        HRESULT ( STDMETHODCALLTYPE *BeginClose )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndClose )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginReset )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndReset )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT ( STDMETHODCALLTYPE *BeginMoveNext )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndMoveNext )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeScanPosition **scanPosition);
        
        HRESULT ( STDMETHODCALLTYPE *BeginPeekNextKey )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT ( STDMETHODCALLTYPE *EndPeekNextKey )( 
            IFabricBTreeScan * This,
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricBtreeKey **nextKey);
        
        END_INTERFACE
    } IFabricBTreeScanVtbl;

    interface IFabricBTreeScan
    {
        CONST_VTBL struct IFabricBTreeScanVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFabricBTreeScan_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFabricBTreeScan_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFabricBTreeScan_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFabricBTreeScan_BeginOpen(This,beginKey,endKey,keyPrefixMatch,isValueNeeded,callback,context)	\
    ( (This)->lpVtbl -> BeginOpen(This,beginKey,endKey,keyPrefixMatch,isValueNeeded,callback,context) ) 

#define IFabricBTreeScan_EndOpen(This,context,keyBeforeStartPosition)	\
    ( (This)->lpVtbl -> EndOpen(This,context,keyBeforeStartPosition) ) 

#define IFabricBTreeScan_BeginClose(This,callback,context)	\
    ( (This)->lpVtbl -> BeginClose(This,callback,context) ) 

#define IFabricBTreeScan_EndClose(This,context)	\
    ( (This)->lpVtbl -> EndClose(This,context) ) 

#define IFabricBTreeScan_BeginReset(This,callback,context)	\
    ( (This)->lpVtbl -> BeginReset(This,callback,context) ) 

#define IFabricBTreeScan_EndReset(This,context)	\
    ( (This)->lpVtbl -> EndReset(This,context) ) 

#define IFabricBTreeScan_BeginMoveNext(This,callback,context)	\
    ( (This)->lpVtbl -> BeginMoveNext(This,callback,context) ) 

#define IFabricBTreeScan_EndMoveNext(This,context,scanPosition)	\
    ( (This)->lpVtbl -> EndMoveNext(This,context,scanPosition) ) 

#define IFabricBTreeScan_BeginPeekNextKey(This,callback,context)	\
    ( (This)->lpVtbl -> BeginPeekNextKey(This,callback,context) ) 

#define IFabricBTreeScan_EndPeekNextKey(This,context,nextKey)	\
    ( (This)->lpVtbl -> EndPeekNextKey(This,context,nextKey) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFabricBTreeScan_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FabricBtree;

#ifdef __cplusplus

class DECLSPEC_UUID("446888ef-0b42-41eb-a663-6fd1a72b293f")
FabricBtree;
#endif


#ifndef __FabricData_MODULE_DEFINED__
#define __FabricData_MODULE_DEFINED__


/* module FabricData */
/* [dllname][uuid] */ 

/* [entry] */ HRESULT FabricCreateBtree( 
    /* [retval][out] */ __RPC__deref_out_opt IFabricBtree **btree);

#endif /* __FabricData_MODULE_DEFINED__ */
#endif /* __FabricBTreeTypeLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


