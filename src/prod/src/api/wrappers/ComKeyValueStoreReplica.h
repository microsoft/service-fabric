// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {2335b1e4-7253-48d9-8b9a-bbdc86697a93}
    static const GUID CLSID_ComKeyValueStoreReplica = 
    {0x2335b1e4,0x7253,0x48d9,{0x8b,0x9a,0xbb,0xdc,0x86,0x69,0x7a,0x93}};

    class ComKeyValueStoreReplica :
        public IFabricKeyValueStoreReplica6,
        private ComStatefulServiceReplica
    {
        DENY_COPY(ComKeyValueStoreReplica)

        BEGIN_COM_INTERFACE_LIST(ComKeyValueStoreReplica)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplica, IFabricKeyValueStoreReplica)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplica2, IFabricKeyValueStoreReplica2)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplica3, IFabricKeyValueStoreReplica3)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplica4, IFabricKeyValueStoreReplica4)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplica5, IFabricKeyValueStoreReplica5)
            COM_INTERFACE_ITEM(IID_IFabricKeyValueStoreReplica6, IFabricKeyValueStoreReplica6)
            COM_INTERFACE_ITEM(CLSID_ComKeyValueStoreReplica, ComKeyValueStoreReplica)
            COM_DELEGATE_TO_BASE_ITEM(ComStatefulServiceReplica)
        END_COM_INTERFACE_LIST()

    public:
        ComKeyValueStoreReplica(IKeyValueStoreReplicaPtr const & impl);
        virtual ~ComKeyValueStoreReplica();

        IKeyValueStoreReplicaPtr const & get_Impl() const { return impl_; }

        //
        // IFabricStatefulServiceReplica methods
        // 
        HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicator);
        
        HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);
        
        HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        
        HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context);
        
        void STDMETHODCALLTYPE Abort(void);

        // 
        // IFabricKeyValueStoreReplica methods
        // 
        HRESULT STDMETHODCALLTYPE GetCurrentEpoch( 
            /* [out] */ FABRIC_EPOCH *currentEpoch);
        
        HRESULT STDMETHODCALLTYPE UpdateReplicatorSettings( 
            /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings);
        
        HRESULT STDMETHODCALLTYPE CreateTransaction( 
            /* [retval][out] */ IFabricTransaction **transaction);
        
        HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value);
        
        HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber);
        
        HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT STDMETHODCALLTYPE GetMetadata( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT STDMETHODCALLTYPE Contains( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ BOOLEAN *result);
        
        HRESULT STDMETHODCALLTYPE Enumerate( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT STDMETHODCALLTYPE EnumerateByKey( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT STDMETHODCALLTYPE EnumerateMetadata( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);
        
        HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);

        // 
        // IFabricKeyValueStoreReplica2 methods
        // 
        HRESULT STDMETHODCALLTYPE Backup( 
            /* [in] */ LPCWSTR backupDirectory);

        HRESULT STDMETHODCALLTYPE Restore( 
            /* [in] */ LPCWSTR backupDirectory);

        HRESULT STDMETHODCALLTYPE CreateTransaction2( 
            /* [in] */ const FABRIC_KEY_VALUE_STORE_TRANSACTION_SETTINGS * settings,
            /* [retval][out] */ IFabricTransaction **transaction);

        // 
        // IFabricKeyValueStoreReplica3 methods
        // 

        HRESULT STDMETHODCALLTYPE BeginBackup(
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_STORE_BACKUP_OPTION backupOption,
            /* [in] */ IFabricStorePostBackupHandler *postBackupHandler,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndBackup(
            /* [in] */ IFabricAsyncOperationContext *context);

        // 
        // IFabricKeyValueStoreReplica4 methods
        // 

        HRESULT STDMETHODCALLTYPE BeginRestore(
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndRestore(
            /* [in] */ IFabricAsyncOperationContext *context);

        // 
        // IFabricKeyValueStoreReplica5 methods
        // 

        HRESULT STDMETHODCALLTYPE TryAdd( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [retval][out] */ BOOLEAN *added);
        
        HRESULT STDMETHODCALLTYPE TryRemove( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists);
        
        HRESULT STDMETHODCALLTYPE TryUpdate( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [in] */ LONG valueSizeInBytes,
            /* [size_is][in] */ const BYTE *value,
            /* [in] */ FABRIC_SEQUENCE_NUMBER checkSequenceNumber,
            /* [retval][out] */ BOOLEAN *exists);
        
        HRESULT STDMETHODCALLTYPE TryGet( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemResult **result);
        
        HRESULT STDMETHODCALLTYPE TryGetMetadata( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR key,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataResult **result);
        
        HRESULT STDMETHODCALLTYPE EnumerateByKey2( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemEnumerator **result);
        
        HRESULT STDMETHODCALLTYPE EnumerateMetadataByKey2( 
            /* [in] */ IFabricTransactionBase *transaction,
            /* [in] */ LPCWSTR keyPrefix,
            /* [in] */ BOOLEAN strictPrefix,
            /* [retval][out] */ IFabricKeyValueStoreItemMetadataEnumerator **result);

        // 
        // IFabricKeyValueStoreReplica6 methods
        // 

        HRESULT STDMETHODCALLTYPE BeginRestore2(
            /* [in] */ LPCWSTR backupDirectory,
            /* [in] */ FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS * settings,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

    private:

        class BackupOperationContext;
        class RestoreOperationContext;

        IKeyValueStoreReplicaPtr impl_;
    };
}
