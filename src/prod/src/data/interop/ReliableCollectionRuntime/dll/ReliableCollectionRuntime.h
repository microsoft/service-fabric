// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define RELIABLECOLLECTION_API_VERSION 0x100

#if !defined(_EXPORTING) && defined(WIN32)
#define CLASS_DECLSPEC    __declspec(dllimport)  
#else
#define CLASS_DECLSPEC 
#endif  

typedef void* StateProviderHandle;
typedef void* StoreHandle;
typedef void* TransactionHandle;
typedef void* CancellationTokenSourceHandle;
typedef void* StoreKeyEnumeratorHandle;
typedef void* StoreKeyValueAsyncEnumeratorHandle;
typedef void* TxnReplicatorHandle;
typedef void* StateProviderEnumeratorHandle;
typedef void* BufferHandle;
typedef void* PrimaryReplicatorHandle;

#if !defined(WIN32) && !defined(UNDER_PAL) 
typedef int32_t HRESULT;
typedef const unsigned short* LPCWSTR;
typedef int32_t BOOL;
typedef unsigned short WCHAR;
#ifndef TRUE
    #define TRUE 1
    #define FALSE 0
#endif
#define __in
#define __out
typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;
#endif

enum Store_LockMode : uint32_t
{
    Store_LockMode_Free,
    Store_LockMode_Shared,
    Store_LockMode_Exclusive,
    Store_LockMode_Update
};

enum Backup_Option : uint32_t
{
    Backup_Option_Invalid,
    Backup_Option_Full,
    Backup_Option_Incremental
};

enum Restore_Policy : uint32_t
{
    Restore_Policy_Invalid,
    Restore_Policy_Safe,
    Restore_policy_Force
};

struct Buffer
{
    char* Bytes;
    uint32_t Length;
    BufferHandle Handle;
};

struct Epoch
{
    int64_t DataLossNumber;
    int64_t ConfigurationNumber;
    void *Reserved;
};

struct Backup_Version
{
    struct Epoch epoch;
    int64_t lsn;
};

struct Backup_Info
{
    GUID backupId;
    LPCWSTR directoryPath;
    Backup_Option option;
    Backup_Version version;
};

struct Backup_Info2
{
    uint32_t Size;                  // For versioning
    GUID backupId;
    LPCWSTR directoryPath;
    Backup_Option option;
    Backup_Version version;
    Backup_Version startVersion;
    GUID parentbackupId;
};

struct TxnReplicator_Settings
{
    uint64_t Flags;
    uint32_t RetryIntervalMilliseconds;
    uint32_t BatchAcknowledgementIntervalMilliseconds;
    void* ReplicatorAddress;
    uint32_t InitialCopyQueueSize;
    uint32_t MaxCopyQueueSize;
    void* SecurityCredentials;
    BOOL SecondaryClearAcknowledgedOperations;
    uint32_t MaxReplicationMessageSize;
    uint32_t InitialPrimaryReplicationQueueSize;
    uint32_t MaxPrimaryReplicationQueueSize;
    uint32_t MaxPrimaryReplicationQueueMemorySize;
    uint32_t InitialSecondaryReplicationQueueSize;
    uint32_t MaxSecondaryReplicationQueueSize;
    uint32_t MaxSecondaryReplicationQueueMemorySize;
    void* ReplicatorListenAddress;
    void* ReplicatorPublishAddress;
    uint32_t MaxStreamSizeInMB;
    uint32_t MaxMetadataSizeInKB;
    uint32_t MaxRecordSizeInKB;
    uint32_t MaxWriteQueueDepthInKB;
    uint32_t CheckpointThresholdInMB;
    uint32_t MaxAccumulatedBackupLogSizeInMB;
    BOOL OptimizeForLocalSSD;
    BOOL OptimizeLogForLowerDiskUsage;
    uint32_t SlowApiMonitoringDurationSeconds;
    uint32_t MinLogSizeInMB;
    uint32_t TruncationThresholdFactor;
    uint32_t ThrottlingThresholdFactor;
    void* SharedLogId;
    void* SharedLogPath;
};

enum TxnReplicator_Settings_Flags : uint64_t
{
    None = 0x0,
    RetryInterval = 0x1,
    BatchAcknowledgementInterval = 0x2,
    ReplicatorAddress = 0x4,
    InitialCopyQueueSize = 0x8,
    MaxCopyQueueSize = 0x10,
    SecurityCredentials = 0x20,
    SecondaryClearAcknowledgedOperations = 0x40,
    MaxReplicationMessageSize = 0x80,
    InitialPrimaryReplicationQueueSize = 0x100,
    MaxPrimaryReplicationQueueSize = 0x200,
    MaxPrimaryReplicationQueueMemorySize = 0x400,
    InitialSecondaryReplicationQueueSize = 0x800,
    MaxSecondaryReplicationQueueSize = 0x1000,
    MaxSecondaryReplicationQueueMemorySize = 0x2000,
    ReplicatorListenAddress = 0x4000,
    ReplicatorPublishAddress = 0x8000,
    MaxStreamSize = 0x10000,
    MaxMetadataSize = 0x20000,
    MaxRecordSize = 0x40000,
    MaxWriteQueueDepth = 0x80000,
    CheckpointThreshold = 0x100000,
    MaxAccumulatedBackupLogSize = 0x200000,
    OptimizeForLocalSSD = 0x400000,
    OptimizeLogForLowerDiskUsage = 0x800000,
    SlowApiMonitoringDuration = 0x1000000,
    MinLogSize = 0x2000000,
    TruncationThresholdFactor = 0x4000000,
    ThrottlingThresholdFactor = 0x8000000,
    SharedLogId = 0x10000000,
    SharedLogPath = 0x20000000
};

#define StateProvider_Info_V1_Size 16
enum StateProvider_Kind : uint32_t
{
    StateProvider_Kind_Invalid = 0,
    StateProvider_Kind_Store = 1,
    StateProvider_Kind_ConcurrentQueue,
    StateProvider_Kind_ReliableDictionary_Compat
};

struct StateProvider_Info
{
    uint32_t Size;
    StateProvider_Kind Kind;
    LPCWSTR LangMetadata;
};

#ifdef __cplusplus 
extern "C" 
#endif
{
    CLASS_DECLSPEC HRESULT ReliableCollectionRuntime_StartTraceSessions();
    CLASS_DECLSPEC HRESULT ReliableCollectionRuntime_Initialize(uint16_t apiVersion);
    CLASS_DECLSPEC HRESULT ReliableCollectionRuntime_Initialize2(uint16_t apiVersion, BOOL standAloneMode);

    /*************************************
    * Transaction APIs
    *************************************/
    // Release and Dispose (for back compat only)
    CLASS_DECLSPEC void Transaction_Release(
        __in TransactionHandle txn) noexcept;

    CLASS_DECLSPEC void Transaction_Dispose(
        __in TransactionHandle txn) noexcept;

    CLASS_DECLSPEC void Transaction_AddRef(
        __in TransactionHandle txn) noexcept;

    typedef void(*fnNotifyAsyncCompletion)(void* ctx, HRESULT status);

    CLASS_DECLSPEC HRESULT Transaction_CommitAsync(
        __in TransactionHandle txn,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete) noexcept;

    CLASS_DECLSPEC HRESULT Transaction_Abort(
        __in TransactionHandle txn) noexcept;

    struct Transaction_Info
    {
        uint32_t Size;                  // For versioning
        int64_t CommitSequenceNumber;
        int64_t Id;
    };

    CLASS_DECLSPEC HRESULT Transaction_GetInfo(__in TransactionHandle txnHandle, __out Transaction_Info *info);

    typedef void(*fnNotifyGetVisibilitySequenceNumberCompletion)(__in void* ctx, __in HRESULT status, __in int64_t vsn);

    CLASS_DECLSPEC HRESULT Transaction_GetVisibilitySequenceNumberAsync(
        __in TransactionHandle txnHandle,
        __out int64_t *sequenceNumber,
        __in fnNotifyGetVisibilitySequenceNumberCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    /*************************************
     * TStore APIs
     *************************************/
    CLASS_DECLSPEC void Store_Release(
        __in StateProviderHandle storeHandle);

    CLASS_DECLSPEC void Store_AddRef(
        __in StateProviderHandle storeHandle);

    typedef void(*fnNotifyGetAsyncCompletion)(void* ctx, HRESULT status, BOOL r, size_t objectHandle, void* bytes, uint32_t bytesLength, int64_t versionSequenceNumber);

    CLASS_DECLSPEC HRESULT Store_ConditionalGetAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCWSTR key,
        __in int64_t timeout,
        __in Store_LockMode lockMode,
        __out size_t* objectHandle,
        __out Buffer* value,
        __out int64_t* versionSequenceNumber,
        __out CancellationTokenSourceHandle* cts,
        __out BOOL* found,
        __in fnNotifyGetAsyncCompletion callback,      // callback to be called when call does not complete synchronously
        __in void* ctx,                      // Context to be passed in when making callback
        __out BOOL* synchronousComplete);    // if call completed synchronously

    CLASS_DECLSPEC HRESULT Store_AddAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCWSTR key,
        __in size_t objectHandle,               // handle of object to be stored  
        __in void* bytes,                    // serialized byte array of object
        __in uint32_t bytesLength,             // byte array length
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    typedef void(*fnNotifyUpdateAsyncCompletion)(void* ctx, HRESULT status, BOOL updated);

    CLASS_DECLSPEC HRESULT Store_ConditionalUpdateAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCWSTR key,
        __in size_t objectHandle,               // handle of object to be stored  
        __in void* bytes,                    // serialized byte array of object
        __in uint32_t bytesLength,                // byte array length
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in int64_t conditionalVersion,
        __out BOOL* updated,
        __in fnNotifyUpdateAsyncCompletion callback,
        __in void* ctx,                
        __out BOOL* synchronousComplete);    

    typedef void(*fnNotifyRemoveAsyncCompletion)(void* ctx, HRESULT status, BOOL removed);

    CLASS_DECLSPEC HRESULT Store_ConditionalRemoveAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCWSTR key,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in int64_t conditionalVersion,
        __out BOOL* removed,
        __in fnNotifyRemoveAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    CLASS_DECLSPEC HRESULT Store_GetCount(
        __in StateProviderHandle store,
        __out int64_t* count);

    typedef void(*fnNotifyCreateKeyEnumeratorAsyncCompletion)(void* ctx, HRESULT status, StoreKeyEnumeratorHandle enumerator);

    CLASS_DECLSPEC HRESULT Store_CreateKeyEnumeratorAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCWSTR firstKey,
        __in LPCWSTR lastKey,
        __out StoreKeyEnumeratorHandle* enumerator,
        __in fnNotifyCreateKeyEnumeratorAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    typedef void(*fnNotifyCreateEnumeratorAsyncCompletion)(void* ctx, HRESULT status, StoreKeyValueAsyncEnumeratorHandle enumerator);

    CLASS_DECLSPEC HRESULT Store_CreateEnumeratorAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __out StoreKeyValueAsyncEnumeratorHandle* enumerator,
        __in fnNotifyCreateEnumeratorAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    CLASS_DECLSPEC HRESULT Store_CreateRangedEnumeratorAsync(
		__in StoreHandle store,
		__in TransactionHandle txn,
		__in LPCWSTR firstKey,
		__in LPCWSTR lastKey,
		__out StoreKeyValueAsyncEnumeratorHandle* enumerator,
		__in fnNotifyCreateEnumeratorAsyncCompletion callback,
		__in void* ctx,
		__out BOOL* synchronousComplete);

    typedef void(*fnNotifyContainsKeyAsyncCompletion)(void* ctx, HRESULT status, BOOL found);

    CLASS_DECLSPEC HRESULT Store_ContainsKeyAsync(
        __in StateProviderHandle store,
        __in TransactionHandle txn,
        __in LPCWSTR key,
        __in int64_t timeout,
        __in Store_LockMode lockMode,
        __out CancellationTokenSourceHandle* cts,
        __out BOOL* found,
        __in fnNotifyContainsKeyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    /*************************************
    * ReliableConcurrentQueue APIs
    *************************************/
    typedef void(*fnNotifyTryDequeueAsyncCompletion)(void* ctx, HRESULT status, BOOL succeeded, size_t objectHandle, void* bytes, uint32_t bytesLength);

    CLASS_DECLSPEC HRESULT ConcurrentQueue_EnqueueAsync(
        __in StateProviderHandle concurrentQueue,
        __in TransactionHandle txn,
        __in size_t objectHandle,               // handle of object to be stored  
        __in void* bytes,                       // serialized byte array of object
        __in uint32_t bytesLength,              // byte array length
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    CLASS_DECLSPEC HRESULT ConcurrentQueue_TryDequeueAsync(
        __in StateProviderHandle concurrentQueue,
        __in TransactionHandle txn,
        __in int64_t timeout,
        __out size_t* objectHandle,
        __out Buffer* value,
        __out CancellationTokenSourceHandle* cts,
        __out BOOL* succeeded,
        __in fnNotifyTryDequeueAsyncCompletion callback,      // callback to be called when call does not complete synchronously
        __in void* ctx,                      // Context to be passed in when making callback
        __out BOOL* synchronousComplete);    // if call completed synchronously

    CLASS_DECLSPEC HRESULT ConcurrentQueue_GetCount(
        __in StateProviderHandle concurrentQueue,
        __out int64_t* count);

    /*************************************
    * StateProvider APIs
    *************************************/
    CLASS_DECLSPEC HRESULT StateProvider_GetInfo(
        __in StateProviderHandle stateProvider,
        __in LPCWSTR lang,
        __out StateProvider_Info* stateProviderInfo);

    CLASS_DECLSPEC void StateProvider_AddRef(
        __in StateProviderHandle stateProviderHandle);

    CLASS_DECLSPEC void StateProvider_Release(
        __in StateProviderHandle stateProviderHandle);

    /*************************************
     * TxnReplicator APIs
     *************************************/

    struct TxnReplicator_Info
    {
        uint32_t Size;                  // For versioning
        int64_t LastStableSequenceNumber;
        int64_t LastCommittedSequenceNumber;
        Epoch CurrentEpoch;
    };

    CLASS_DECLSPEC HRESULT TxnReplicator_GetInfo(
        __in TxnReplicatorHandle txnReplicator,
        __out TxnReplicator_Info* info) noexcept;

    CLASS_DECLSPEC HRESULT TxnReplicator_CreateTransaction(
        __in TxnReplicatorHandle txnReplicator,
        __out TransactionHandle* txn) noexcept;

    CLASS_DECLSPEC void TxnReplicator_Release(
        __in TxnReplicatorHandle txnReplicator) noexcept;

    typedef void(*fnNotifyGetOrAddStateProviderAsyncCompletion)(void* ctx, HRESULT status, StateProviderHandle store, BOOL exist);

    CLASS_DECLSPEC HRESULT TxnReplicator_GetOrAddStateProviderAsync(
        __in TxnReplicatorHandle txnReplicator,
        __in TransactionHandle txn,
        __in LPCWSTR name,
        __in LPCWSTR lang,
        __in StateProvider_Info* stateProviderInfo,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __out StateProviderHandle* stateProvider,
        __out BOOL* alreadyExist,
        __in fnNotifyGetOrAddStateProviderAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete) noexcept;

    CLASS_DECLSPEC HRESULT TxnReplicator_AddStateProviderAsync(
        __in TxnReplicatorHandle txnReplicator,
        __in TransactionHandle txn,
        __in LPCWSTR name,
        __in LPCWSTR lang,
        __in StateProvider_Info* stateProviderInfo,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete) noexcept;

    CLASS_DECLSPEC HRESULT TxnReplicator_RemoveStateProviderAsync(
        __in TxnReplicatorHandle txnReplicator,
        __in TransactionHandle txn,
        __in LPCWSTR name,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL*synchronousComplete) noexcept;

    CLASS_DECLSPEC HRESULT TxnReplicator_CreateEnumerator(
        __in TxnReplicatorHandle txnReplicator,
        __in BOOL parentsOnly,
        __out StateProviderEnumeratorHandle* enumerator) noexcept;

    CLASS_DECLSPEC HRESULT TxnReplicator_GetStateProvider(
        __in TxnReplicatorHandle txnReplicator,
        __in LPCWSTR name,
        __out StateProviderHandle* store) noexcept;

    typedef void(*fnNotifyUploadAsyncCompletion)(void* ctx, BOOL uploaded);
    typedef void(*fnUploadAsync)(void* ctx, Backup_Info backup_Info, fnNotifyUploadAsyncCompletion uploadCallbackEnd, void* uploadAsyncCompletionCtx);
    typedef void(*fnUploadAsync2)(void* ctx, Backup_Info2* backup_Info, uint32_t size_backup_Info, fnNotifyUploadAsyncCompletion uploadCallbackEnd, void* uploadAsyncCompletionCtx);

    CLASS_DECLSPEC HRESULT TxnReplicator_BackupAsync(
        __in TxnReplicatorHandle txnReplicator,
        __in fnUploadAsync uploadAsyncCallback,
        __in Backup_Option backupOption,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    CLASS_DECLSPEC HRESULT TxnReplicator_BackupAsync2(
        __in TxnReplicatorHandle txnReplicator,
        __in fnUploadAsync2 uploadAsyncCallback,
        __in Backup_Option backupOption,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle* cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    CLASS_DECLSPEC HRESULT TxnReplicator_RestoreAsync(
        __in TxnReplicatorHandle txnReplicator,
        __in LPCWSTR backupFolder,
        __in Restore_Policy restorePolicy,
        __in int64_t timeout,
        __out CancellationTokenSourceHandle*  cts,
        __in fnNotifyAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronouscomplete);


    CLASS_DECLSPEC HRESULT GetTxnReplicator(
        __in int64_t replicaId,
        __in void* statefulServicePartition,
        __in void* dataLossHandler,
        __in TxnReplicator_Settings const* replicatorSettings,
        __in LPCWSTR configPackageName,
        __in LPCWSTR replicatorSettingsSectionName,
        __in LPCWSTR replicatorSecuritySectionName,
        __out PrimaryReplicatorHandle* primaryReplicator,
        __out TxnReplicatorHandle* txnReplicatorHandle);

    CLASS_DECLSPEC HRESULT PrimaryReplicator_UpdateReplicatorSettings(
        __in PrimaryReplicatorHandle primaryReplicator,
        __in TxnReplicator_Settings const* replicatorSettings);

    /*************************************
     * CancellationTokenSource APIs
     *************************************/
    CLASS_DECLSPEC void CancellationTokenSource_Cancel(
        CancellationTokenSourceHandle cts);
    CLASS_DECLSPEC void CancellationTokenSource_Release(
        CancellationTokenSourceHandle cts);


    /*************************************
     * StateProviderEnumerator APIs
     *************************************/
    CLASS_DECLSPEC void StateProviderEnumerator_Release(
        __in StateProviderEnumeratorHandle enumerator);

    CLASS_DECLSPEC void StateProviderEnumerator_AddRef(
        __in StateProviderEnumeratorHandle enumerator);

    CLASS_DECLSPEC HRESULT StateProviderEnumerator_MoveNext(
        __in StateProviderEnumeratorHandle enumerator,
        __out BOOL* advanced,
        __out LPCWSTR* providerName,
        __out StateProviderHandle* provider);


    /*************************************
     * StoreKeyEnumerator APIs
     *************************************/
    CLASS_DECLSPEC void StoreKeyEnumerator_Release(
        StoreKeyEnumeratorHandle enumerator);

    CLASS_DECLSPEC void StoreKeyEnumerator_AddRef(
        StoreKeyEnumeratorHandle enumerator);

    CLASS_DECLSPEC HRESULT StoreKeyEnumerator_MoveNext(
        __in StoreKeyEnumeratorHandle enumerator,
        __out BOOL* advanced,
        __out LPCWSTR* key);


    /*************************************
     * StoreKeyValueEnumerator APIs
     *************************************/
    CLASS_DECLSPEC void StoreKeyValueEnumerator_Release(
        __in StoreKeyValueAsyncEnumeratorHandle enumerator);

    typedef void(*fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion)(void* ctx, HRESULT status, BOOL advanced, LPCWSTR key, size_t objectHandle, void* byteBuffer, uint32_t bufferLength, int64_t versionSequenceNumber);

    CLASS_DECLSPEC HRESULT StoreKeyValueEnumerator_MoveNextAsync(
        __in StoreKeyValueAsyncEnumeratorHandle enumerator,
        __out CancellationTokenSourceHandle* cts,
        __out BOOL* advanced,
        __out LPCWSTR* key,
        __out size_t* objectHandle,
        __out Buffer* value,
        __out int64_t* versionSequenceNumber,
        __in fnNotifyStoreKeyValueEnumeratorMoveNextAsyncCompletion callback,
        __in void* ctx,
        __out BOOL* synchronousComplete);

    CLASS_DECLSPEC void Buffer_Release(BufferHandle handle);

    /*************************************
    * Notification APIs
    *************************************/

    // This matches NotifyDictionaryChangedAction
    enum StoreChangeKind : uint32_t
    {
        StoreChangeKind_Add = 0,
        StoreChangeKind_Update = 1,
        StoreChangeKind_Remove = 2,
        StoreChangeKind_Clear = 3,
        StoreChangeKind_Rebuild = 4
    };

    struct StoreChangeData_Add
    {
        int64_t CommitSequnceNumber;
        TransactionHandle Transaction;
        LPCWSTR Key;
        char *Value;            // @TODO - Use BufferHandle for batching
        uint32_t Length;
    };

    struct StoreChangeData_Update
    {
        int64_t CommitSequnceNumber;
        TransactionHandle Transaction;
        LPCWSTR Key;
        char *Value;            // @TODO - Use BufferHandle for batching
        uint32_t Length;
    };

    struct StoreChangeData_Remove 
    {
        int64_t CommitSequnceNumber;
        TransactionHandle Transaction;
        LPCWSTR Key;
    };

    struct StoreChangeData_Clear
    {
        // @TODO - NYI. Need to decide if store is implementing ClearAll.
    };

    typedef void(*fnAsyncCompletionCallback)(void* ctx);
    
    struct StoreChangeData_Rebuild 
    {
        StoreKeyValueAsyncEnumeratorHandle Enumerator;
        fnAsyncCompletionCallback Callback;
        void *Context;
    };

    typedef void(*fnNotifyStoreChangeCallback)(void* ctx, StateProviderHandle stateProvider, StoreChangeKind storeChangeKind, void *pData);
    typedef void(*fnCleanupContextCallback)(void* ctx);

    CLASS_DECLSPEC HRESULT Store_SetNotifyStoreChangeCallback(
        __in StateProviderHandle stateProvider,
        __in fnNotifyStoreChangeCallback callback, 
        __in fnCleanupContextCallback cleanupCallback,
        __in void *ctx);

    // These need to match DICTIONARY_CHANGE_HANDLER_MASK definition exactly
    enum NotifyStoreChangeCallbackMask : uint32_t
    {
        Add = 0x1,
        Update = 0x2,
        Remove = 0x4,
        Clear = 0x8,
        Rebuild = 0x10
    };

    CLASS_DECLSPEC HRESULT Store_SetNotifyStoreChangeCallbackMask(
        __in StateProviderHandle stateProviderHandle,
        __in NotifyStoreChangeCallbackMask mask);

    // Matches NotifyStateManagerChangedAction
    enum StateManagerChangeKind : uint32_t
    {
        StateManagerChangeKind_Add = 0,
        StateManagerChangeKind_Remove = 1,
        StateManagerChangeKind_Rebuild = 2
    };

    struct StateManagerChangeData_SingleEntityChanged
    {
        TransactionHandle Transaction;
        StateProviderHandle StateProvider;
        LPCWSTR StateProviderName;
    };

    struct StateManagerChangeData_Rebuild
    {
        StateProviderEnumeratorHandle StateProviders;
    };

    typedef void(*fnNotifyStateManagerChangeCallback)(void* ctx, TxnReplicatorHandle txnReplicator, StateManagerChangeKind changeKind, void *pData);

    CLASS_DECLSPEC HRESULT TxnReplicator_SetNotifyStateManagerChangeCallback(
        __in TxnReplicatorHandle txnReplicator,
        __in fnNotifyStateManagerChangeCallback callback, 
        __in fnCleanupContextCallback cleanupCallback,
        __in void *ctx);

    // Matches NotifyTransactionChangedAction
    enum TransactionChangeKind : uint32_t
    {
        TransactionChangeKind_Commit = 0,
    };

    struct TransactionChangeData_Commit
    {
        TransactionHandle Transaction;
    };

    typedef void(*fnNotifyTransactionChangeCallback)(void* ctx, TxnReplicatorHandle txnReplicator, TransactionChangeKind changeKind, void *pData);

    CLASS_DECLSPEC HRESULT TxnReplicator_SetNotifyTransactionChangeCallback(
        __in TxnReplicatorHandle txnReplicator,
        __in fnNotifyTransactionChangeCallback callback,
        __in fnCleanupContextCallback cleanupCallback,
        __in void *ctx);

    /*************************************
    * Test Apis
    *************************************/
    CLASS_DECLSPEC void Test_UseEnv(BOOL enable);

}
