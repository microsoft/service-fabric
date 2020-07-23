// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Data::TStore;
using namespace ktl;
using namespace Reliability::ReplicationComponent;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("TSReplicatedStore");

Global<KUriView> TSReplicatedStore::TStoreProviderName = make_global<KUriView>(L"fabric:/kvs");
Global<KStringView> TSReplicatedStore::TStoreProviderType = make_global<KStringView>(L"KVS");

#define COM_TX_REPLICATOR_RUNTIME_CONFIGS_TAG 'crST'

class TSReplicatedStore::ComTransactionalReplicatorRuntimeConfigurations 
    : public IFabricTransactionalReplicatorRuntimeConfigurations
    , public KObject<ComTransactionalReplicatorRuntimeConfigurations>
    , public KShared<ComTransactionalReplicatorRuntimeConfigurations>
    , public PartitionedReplicaTraceComponent<TraceTaskCodes::ReplicatedStore>
{
    K_BEGIN_COM_INTERFACE_LIST(ComTransactionalReplicatorRuntimeConfigurations)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransactionalReplicatorRuntimeConfigurations)
        COM_INTERFACE_ITEM(IID_IFabricTransactionalReplicatorRuntimeConfigurations, IFabricTransactionalReplicatorRuntimeConfigurations)
    K_END_COM_INTERFACE_LIST()

private:

    ComTransactionalReplicatorRuntimeConfigurations(
        PartitionedReplicaTraceComponent const & trace,
        KString::SPtr const & workDirectory)
        : PartitionedReplicaTraceComponent(trace)
        , workDirectory_(workDirectory)
    {
        WriteNoise(
            TraceComponent,
            "{0} ComTransactionalReplicatorRuntimeConfigurations::ctor: {1}",
            this->TraceId,
            TraceThis);
    }

public:

    ~ComTransactionalReplicatorRuntimeConfigurations()
    {
        WriteNoise(
            TraceComponent,
            "{0} ComTransactionalReplicatorRuntimeConfigurations::dtor: {1}",
            this->TraceId,
            TraceThis);
    }

public:

    static NTSTATUS Create(
        PartitionedReplicaTraceComponent const & trace,
        wstring const & workDirectory,
        __in KAllocator & allocator,
        __out KSharedPtr<ComTransactionalReplicatorRuntimeConfigurations> & result)
    {
        KString::SPtr kWorkDirectory;
        auto status = KString::Create(kWorkDirectory, allocator, workDirectory.c_str());
        if (NT_ERROR(status)) { return status; }

        auto * pointer = _new(COM_TX_REPLICATOR_RUNTIME_CONFIGS_TAG, allocator) ComTransactionalReplicatorRuntimeConfigurations(trace, kWorkDirectory);

        if (pointer == nullptr) { return SF_STATUS_INVALID_OPERATION; }
        if (NT_ERROR(pointer->Status())) { return pointer->Status(); }

        result = KSharedPtr<ComTransactionalReplicatorRuntimeConfigurations>(pointer);
        
        return STATUS_SUCCESS;
    }

public:

    //
    // IFabricTransactionalReplicatorRuntimeConfigurations
    //

    LPCWSTR STDMETHODCALLTYPE get_WorkDirectory(void) { return *workDirectory_; }

private:
    KString::SPtr workDirectory_;
};

#define COM_STATE_PROVIDER_FACTORY_TAG 'fpST'

// Taken from test\FabricTest\ComTStoreService
//
// K_COM interfaces are needed for lifetime tracking 
//
class TSReplicatedStore::ComStateProviderFactory
    : public IFabricStateProvider2Factory
    , public KObject<ComStateProviderFactory>
    , public KShared<ComStateProviderFactory>
    , public PartitionedReplicaTraceComponent<TraceTaskCodes::ReplicatedStore>
{
    K_BEGIN_COM_INTERFACE_LIST(ComStateProviderFactory)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateProvider2Factory)
        COM_INTERFACE_ITEM(IID_IFabricStateProvider2Factory, IFabricStateProvider2Factory)
    K_END_COM_INTERFACE_LIST()

private:

    ComStateProviderFactory(
        PartitionedReplicaTraceComponent const & trace,
        Data::TStore::StoreStateProviderFactory::SPtr const & factory,
        __in KAllocator & allocator)
        : PartitionedReplicaTraceComponent(trace)
        , innerFactory_(factory)
        , allocator_(allocator)
    {
        WriteNoise(
            TraceComponent,
            "{0} ComStateProviderFactory::ctor: {1}",
            this->TraceId,
            TraceThis);
    }

public:

    ~ComStateProviderFactory()
    {
        WriteNoise(
            TraceComponent,
            "{0} ComStateProviderFactory::dtor: {1}",
            this->TraceId,
            TraceThis);
    }

public:

    static NTSTATUS Create(
        PartitionedReplicaTraceComponent const & trace,
        __in KAllocator & allocator,
        __out KSharedPtr<ComStateProviderFactory> & result)
    {
        auto factory = StoreStateProviderFactory::CreateStringUTF16BufferFactory(allocator);
        auto * pointer = _new(COM_STATE_PROVIDER_FACTORY_TAG, allocator) ComStateProviderFactory(trace, factory, allocator);

        if (pointer == nullptr) { return SF_STATUS_INVALID_OPERATION; }
        if (NT_ERROR(pointer->Status())) { return pointer->Status(); }

        result = KSharedPtr<ComStateProviderFactory>(pointer);
        
        return STATUS_SUCCESS;
    }

public:

    // 
    // IFabricStateProvider2Factory
    //

    HRESULT Create(
        /* [in] */ FABRIC_STATE_PROVIDER_FACTORY_ARGUMENTS const * factoryArguments,
        /* [retval][out] */ void ** stateProvider) override
    {
        KStringView typeString(factoryArguments->typeName);
        KUriView nameUri(factoryArguments->name);

        KString::SPtr typeStringSPtr = KString::Create(typeString, allocator_);
        KUri::CSPtr nameUriCSPtr;
        NTSTATUS status = KUri::Create(nameUri, allocator_, nameUriCSPtr);
        if (NT_ERROR(status)) { return HRESULT_FROM_NT(status); }

        KGuid partitionId(factoryArguments->partitionId);
        LONG64 replicaId = factoryArguments->replicaId;
        FABRIC_OPERATION_DATA_BUFFER_LIST * bufferList = factoryArguments->initializationParameters;

        Data::Utilities::OperationData::SPtr initializationParameters = nullptr;
        if (bufferList->Count != 0)
        {
            initializationParameters = Data::Utilities::OperationData::Create(allocator_);

            for (ULONG32 i = 0; i < bufferList->Count; i++)
            {
                ULONG bufferSize = bufferList->Buffers[i].BufferSize;
                BYTE * bufferContext = bufferList->Buffers[i].Buffer;

                KBuffer::SPtr buffer;
                status = KBuffer::Create(bufferSize, buffer, allocator_);
                if (NT_ERROR(status)) { return HRESULT_FROM_NT(status); }

                BYTE * pointer = static_cast<BYTE *>(buffer->GetBuffer());
                KMemCpySafe(pointer, bufferSize, bufferContext, bufferSize);

                initializationParameters->Append(*buffer);
            }
        }

        TxnReplicator::IStateProvider2::SPtr outStateProvider;
        Data::StateManager::FactoryArguments factoryArg(*nameUriCSPtr, factoryArguments->stateProviderId, *typeStringSPtr, partitionId, replicaId, initializationParameters.RawPtr());

        status = innerFactory_->Create(factoryArg, outStateProvider);
        if (NT_ERROR(status)) { return HRESULT_FROM_NT(status); }

        *stateProvider = outStateProvider.Detach();
        return S_OK;
    }

private:
    Data::TStore::StoreStateProviderFactory::SPtr innerFactory_;
    KAllocator & allocator_;
};

class TSReplicatedStore::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(
        __in TSReplicatedStore & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent, true) //skipCompleteOnCancel_
        , owner_(owner)
        , replicatorResult_()
        , completionEvent_(false)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ComPointer<IFabricReplicator> & result)
    {
        auto casted = AsyncOperation::End<OpenAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            result = casted->replicatorResult_;
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        this->StartOpen(thisSPtr);
    }

    void OnCancel() override
    {
        // RA can potentially call Open->Cancel->Open->Close when deleting a replica.
        // The local store may still be open (or opening) when the cancel call aborts
        // the first replica open, causing the subsequent re-open to fail.
        //
        // Although the Open->Cancel->Open sequence should be optimized by RA, protect
        // against it at the store level by blocking cancel.
        //
        // We can block cancel in this case because skipCompleteOnCancel_ is being set 
        // to true and cancel will be called with forceComplete = false. Otherwise,
        // OnCancel() would deadlock waiting for OnCompleted().
        //
        // See also ReplicatedStore.OpenAsyncOperation.cpp
        //
        // Setting skipCompleteOnCancel_ on the child async operation alone is insufficient
        // since the parent async operation can can complete with E_ABORT independent of
        // child async operations (see AsyncOperation.cpp).
        //
        owner_.WriteInfo(TraceComponent, "{0} blocking cancel for open completion", owner_.TraceId);

        completionEvent_.WaitOne();

        owner_.WriteInfo(TraceComponent, "{0} cancel unblocked", owner_.TraceId);
    }

    void OnCompleted() override
    {
        completionEvent_.Set();
    }
private:

    void StartOpen(AsyncOperationSPtr const & thisSPtr)
    {
        auto selfRoot = thisSPtr;

        // Open may be coming from a KTL thread. OnOpen initializes
        // the replicator by calling CreateTransactionalReplicatorInternal
        // on the partition. In the TSLocalStore case, 
        // ComMockPartition::CreateTransactionalReplicatorInternal calls
        // SyncAwait when opening the logging replicator.
        //
        Threadpool::Post([this, selfRoot] { this->DoOpen(selfRoot); });
    }

private:

    void DoOpen(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.OnOpen(replicatorResult_);

        this->TryComplete(thisSPtr, error);
    }

    TSReplicatedStore & owner_;
    ComPointer<IFabricReplicator> replicatorResult_;
    ManualResetEvent completionEvent_;
};

class TSReplicatedStore::ChangeRoleAsyncOperation : public AsyncOperation
{
public:
    ChangeRoleAsyncOperation(
        __in TSReplicatedStore & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<ChangeRoleAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        this->StartChangeRole(thisSPtr);
    }

private:
    
    void StartChangeRole(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.OnChangeRole();

        this->TryComplete(thisSPtr, error);
    }

    TSReplicatedStore & owner_;
};

class TSReplicatedStore::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        __in TSReplicatedStore & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<CloseAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        this->StartClose(thisSPtr);
    }

private:
    
    void StartClose(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.OnClose();

        this->TryComplete(thisSPtr, error);
    }

    TSReplicatedStore & owner_;
};

//
// TSReplicatedStore
//

TSReplicatedStore::TSReplicatedStore(
    Guid const & partitionId,
    ::FABRIC_REPLICA_ID replicaId,
    ReplicatorSettingsUPtr && replicatorSettings,
    TSReplicatedStoreSettingsUPtr && storeSettings,
    Api::IStoreEventHandlerPtr const & storeEventHandler,
    Api::ISecondaryEventHandlerPtr const & secondaryEventHandler,
    ComponentRoot const & root)
    : RootedObject(root)
    , PartitionedReplicaTraceComponent(partitionId, replicaId)
    , TSComponent()
    , replicatorSettings_(move(replicatorSettings))
    , storeSettings_(move(storeSettings))
    , ktlSystem_()
    , stateReplicator_()
    , transactionalReplicator_()
    , txReplicatorLock_()
    , innerStoreLock_()
    , storeEventHandler_(storeEventHandler)
    , secondaryEventHandler_(secondaryEventHandler)
    , changeHandler_()
    , isInitializationActive_(false)
    , initializationTimer_()
    , initializationTimerLock_()
    , initializedEvent_(false)
    , replicaRole_(FABRIC_REPLICA_ROLE_NONE)
    , isActive_(true)
    , queryStatusDetailsLock_()
    , queryStatusDetails_()
    , migrationDetails_()
    , testHookContext_()
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor: {1}",
        this->TraceId,
        TraceThis);
}

TSReplicatedStore::~TSReplicatedStore() 
{ 
    WriteNoise(
        TraceComponent, 
        "{0} dtor: {1}",
        this->TraceId,
        TraceThis);
}

::FABRIC_TRANSACTION_ISOLATION_LEVEL TSReplicatedStore::GetDefaultIsolationLevel()
{
    return FABRIC_TRANSACTION_ISOLATION_LEVEL_SNAPSHOT;
}

ErrorCode TSReplicatedStore::CreateTransaction(__out TransactionSPtr & txSPtr)
{
    if (!this->HasReadStatus() && !this->HasWriteStatus()) { return ErrorCodeValue::NotPrimary; }

    TxnReplicator::ITransactionalReplicator::SPtr txReplicator;
    auto error = this->TryGetTxReplicator(txReplicator);
    if (!error.IsSuccess()) { return error; }

    TxnReplicator::Transaction::SPtr innerTx;
    NTSTATUS status = txReplicator->CreateTransaction(innerTx);

    if (!NT_SUCCESS(status))
    {
        return this->FromNtStatus("CreateTransaction", status);
    }

    // Ignore bool return value (found/created)
    //
    IKvsTransaction::SPtr storeTx;
    TRY_CATCH_VOID( this->TryGetTStore()->CreateOrFindTransaction(*innerTx, storeTx) );

    // Functionally, snapshot isolation is not needed, but deadlocks were encountered with ReadCommitted
    //
    TRY_CATCH_VOID( storeTx->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot );

    txSPtr = TSTransaction::Create(this->PartitionedReplicaId, move(innerTx), move(storeTx));

    return ErrorCodeValue::Success;
}

FILETIME TSReplicatedStore::GetStoreUtcFILETIME()
{
    // TODO: Is this needed?
    //
    FILETIME result = { 0 };
    return  result;
}

ErrorCode TSReplicatedStore::CreateEnumerationByTypeAndKey(
    __in TransactionSPtr const & txSPtr,
    __in wstring const & type,
    __in wstring const & key,
    __out EnumerationSPtr & enumSPtr)
{
    if (!this->HasReadStatus()) { return ErrorCodeValue::NotPrimary; }

    KString::SPtr kKey;
    auto error = this->CreateKey(type, key, kKey);
    if (!error.IsSuccess()) { return error; }

    auto storeTx = this->GetTStoreTransaction(txSPtr);
    auto innerStore = this->TryGetTStore();

    IKvsEnumerator::SPtr innerEnum;
    TRY_CATCH_VOID( innerEnum = SyncAwait(innerStore->CreateKeyEnumeratorAsync(*storeTx, kKey)) );

    enumSPtr = TSEnumeration::Create(
        this->PartitionedReplicaId,
        type,
        key,
        innerStore,
        move(storeTx),
        move(innerEnum));

    return error;
}

ErrorCode TSReplicatedStore::ReadExact(
    __in TransactionSPtr const & txSPtr,
    __in wstring const & type,
    __in wstring const & key,
    __out vector<byte> & value,
    __out __int64 & operationLsn)
{
    FILETIME unusedLastModified;
    return ReadExact(txSPtr, type, key, value, operationLsn, unusedLastModified);
}

ErrorCode TSReplicatedStore::ReadExact(
    __in TransactionSPtr const & txSPtr,
    __in wstring const & type,
    __in wstring const & key,
    __out vector<byte> & value,
    __out __int64 & operationLsn,
    __out FILETIME & lastModified)
{
    // TODO: Not supported by V2 stack
    lastModified = {0};

    if (!this->HasReadStatus()) { return ErrorCodeValue::NotPrimary; }

    KString::SPtr kKey;
    auto error = this->CreateKey(type, key, kKey);
    if (!error.IsSuccess()) { return error; }

    auto storeTx = this->GetTStoreTransaction(txSPtr);
    auto innerStore = this->TryGetTStore();

    bool exists = false;
    IKvsGetEntry entry;
    TRY_CATCH_VOID( exists = SyncAwait(innerStore->ConditionalGetAsync(
        *storeTx, 
        kKey,
        TimeSpan::MaxValue,
        entry, // out
        CancellationToken::None)) );

    if (exists)
    {
        value = ToByteVector(entry.Value);
        operationLsn = entry.Key;

        return ErrorCodeValue::Success;
    }
    else
    {
        return ErrorCodeValue::NotFound;
    }
}

bool TSReplicatedStore::GetIsActivePrimary() const
{
    return (replicaRole_ == FABRIC_REPLICA_ROLE_PRIMARY && isActive_.load());
}

ErrorCode TSReplicatedStore::GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER & result)
{
    TxnReplicator::ITransactionalReplicator::SPtr txReplicator;
    auto error = this->TryGetTxReplicator(txReplicator);
    if (!error.IsSuccess()) { return error; }

    NTSTATUS status = txReplicator->GetLastCommittedSequenceNumber(result);

    if (!NT_SUCCESS(status))
    {
        return this->FromNtStatus("GetLastCommittedSequenceNumber", status);
    }

    return ErrorCodeValue::Success;
}

ErrorCode TSReplicatedStore::CreateSimpleTransaction(
    __out TransactionSPtr & txSPtr)
{
    return this->CreateTransaction(txSPtr);
}

ErrorCode TSReplicatedStore::Insert(
    __in TransactionSPtr const & txSPtr,
    __in wstring const & type,
    __in wstring const & key,
    __in void const * value,
    __in size_t valueSize)
{
    if (!this->HasWriteStatus()) { return ErrorCodeValue::NotPrimary; }

    KString::SPtr kKey;
    auto error = this->CreateKey(type, key, kKey);
    if (!error.IsSuccess()) { return error; }

    KBuffer::SPtr buffer;
    error = this->CreateBuffer(value, valueSize, buffer);
    if (!error.IsSuccess()) { return error; }

    TRY_CATCH_VOID( SyncAwait(this->TryGetTStore()->AddAsync(
        *this->GetTStoreTransaction(txSPtr),
        kKey,
        buffer,
        storeSettings_->TStoreLockTimeout,
        CancellationToken::None)) );

    WriteNoise(
        TraceComponent, 
        "{0} Insert({1}, {2}) {3} bytes", 
        this->TraceId,
        type,
        key,
        valueSize);

    return error;
}

ErrorCode TSReplicatedStore::Update(
    __in TransactionSPtr const & txSPtr,
    __in wstring const & type,
    __in wstring const & key,
    __in _int64 checkSequenceNumber,
    __in wstring const &,
    __in_opt void const * value,
    __in size_t valueSize)
{
    if (!this->HasWriteStatus()) { return ErrorCodeValue::NotPrimary; }

    KString::SPtr kKey;
    auto error = this->CreateKey(type, key, kKey);
    if (!error.IsSuccess()) { return error; }

    KBuffer::SPtr buffer;
    error = this->CreateBuffer(value, valueSize, buffer);
    if (!error.IsSuccess()) { return error; }

    // TStore accepts 0 as a valid LSN for conditional checks.
    // EseLocalStore reserves 0 to mean ignore conditional checks.
    //
    bool success = false;
    TRY_CATCH_VOID( success = SyncAwait(this->TryGetTStore()->ConditionalUpdateAsync(
        *this->GetTStoreTransaction(txSPtr),
        kKey,
        buffer,
        storeSettings_->TStoreLockTimeout,
        CancellationToken::None,
        (checkSequenceNumber == ILocalStore::SequenceNumberIgnore) ? -1 : checkSequenceNumber)) );

    // TODO: Optimize in TStore by distinguishing version mismatch and not found in ConditionalUpdateAsync()
    //
    bool exists = true;
    if (!success)
    {
        IKvsGetEntry unusedEntry;
        TRY_CATCH_VOID( exists = SyncAwait(this->TryGetTStore()->ConditionalGetAsync(
            *this->GetTStoreTransaction(txSPtr),
            kKey,
            storeSettings_->TStoreLockTimeout,
            unusedEntry,
            CancellationToken::None)) );
    }

    WriteNoise(
        TraceComponent, 
        "{0} Update({1}, {2}, lsn={3}) {4} bytes success={5} exists={6}",
        this->TraceId,
        type,
        key,
        checkSequenceNumber,
        valueSize,
        success,
        exists);

    return success ? ErrorCodeValue::Success : (exists ? ErrorCodeValue::StoreWriteConflict : ErrorCodeValue::StoreRecordNotFound);
}

ErrorCode TSReplicatedStore::Delete(
    __in TransactionSPtr const & txSPtr,
    __in wstring const & type,
    __in wstring const & key,
    __in _int64 checkSequenceNumber)
{
    if (!this->HasWriteStatus()) { return ErrorCodeValue::NotPrimary; }

    KString::SPtr kKey;
    auto error = this->CreateKey(type, key, kKey);
    if (!error.IsSuccess()) { return error; }

    bool success = false;
    TRY_CATCH_VOID( success = SyncAwait(this->TryGetTStore()->ConditionalRemoveAsync(
        *this->GetTStoreTransaction(txSPtr),
        kKey,
        storeSettings_->TStoreLockTimeout,
        CancellationToken::None,
        (checkSequenceNumber == ILocalStore::SequenceNumberIgnore) ? -1 : checkSequenceNumber)) );

    // TODO: Optimize in TStore by distinguishing version mismatch and not found in ConditionalRemoveAsync()
    //
    bool exists = true;
    if (!success)
    {
        IKvsGetEntry unusedEntry;
        TRY_CATCH_VOID( exists = SyncAwait(this->TryGetTStore()->ConditionalGetAsync(
            *this->GetTStoreTransaction(txSPtr),
            kKey,
            storeSettings_->TStoreLockTimeout,
            unusedEntry,
            CancellationToken::None)) );
    }

    WriteNoise(
        TraceComponent, 
        "{0} Delete({1}, {2}, lsn={3}) success={4} exists={5}",
        this->TraceId,
        type,
        key,
        checkSequenceNumber,
        success,
        exists);

    return success ? ErrorCodeValue::Success : (exists ? ErrorCodeValue::StoreWriteConflict : ErrorCodeValue::StoreRecordNotFound);
}

// TODO: Used by HM
//
ErrorCode TSReplicatedStore::SetThrottleCallback(ThrottleCallback const &)
{
    WriteWarning(
        TraceComponent,
        "{0} SetThrottleCallback not supported",
        this->TraceId);

    return ErrorCodeValue::Success;
}

ErrorCode TSReplicatedStore::GetCurrentEpoch(__out FABRIC_EPOCH & epoch) const
{
    TxnReplicator::ITransactionalReplicator::SPtr txReplicator;
    auto error = this->TryGetTxReplicator(txReplicator);
    if (!error.IsSuccess()) { return error; }

    NTSTATUS status = txReplicator->GetCurrentEpoch(epoch);

    if (!NT_SUCCESS(status))
    {
        return this->FromNtStatus("GetCurrentEpoch", status);
    }

    return ErrorCodeValue::Success;
}

ErrorCode TSReplicatedStore::UpdateReplicatorSettings(FABRIC_REPLICATOR_SETTINGS const & replicatorSettings)
{
    AcquireWriteLock lock(partitionLock_);

    if (stateReplicator_.GetRawPointer() == nullptr)
    {
        return ErrorCodeValue::ObjectClosed;
    }

    auto error = ErrorCode::FromHResult(stateReplicator_->UpdateReplicatorSettings(&replicatorSettings));

    if (error.IsSuccess())
    {
        error = ReplicatorSettings::FromPublicApi(replicatorSettings, replicatorSettings_);
    }

    return error;
}

ErrorCode TSReplicatedStore::UpdateReplicatorSettings(ReplicatorSettingsUPtr const & inputSettings)
{
    AcquireWriteLock lock(partitionLock_);

    if (stateReplicator_.GetRawPointer() == nullptr)
    {
        return ErrorCodeValue::ObjectClosed;
    }

    ScopedHeap heap;
    auto replicatorSettings = this->CreateReplicatorSettings(heap, inputSettings);

    auto hr = stateReplicator_->UpdateReplicatorSettings(replicatorSettings);
    if (SUCCEEDED(hr))
    {
        replicatorSettings_ = inputSettings->Clone();
    }

    return ErrorCode::FromHResult(hr);
}

ErrorCode TSReplicatedStore::GetQueryStatus(__out Api::IStatefulServiceReplicaStatusResultPtr & result)
{
    wstring queryStatusDetails;
    shared_ptr<MigrationQueryResult> migrationDetails;
    {
        AcquireReadLock lock(queryStatusDetailsLock_);

        queryStatusDetails = queryStatusDetails_;

        if (migrationDetails_.get() != nullptr)
        {
            migrationDetails = make_shared<MigrationQueryResult>(*migrationDetails_);
        }
    }

    size_t estimatedRowCount = 0;
    auto innerStore = this->TryGetTStore();
    if (innerStore.RawPtr() != nullptr)
    {
        estimatedRowCount = innerStore->Count;
    }

    auto resultSPtr = make_shared<KeyValueStoreQueryResult>(
        estimatedRowCount,
        0, // TODO: estimatedDbSize
        shared_ptr<wstring>(), // copyNotificationPrefix
        0, // copyNotificationProgress
        queryStatusDetails,
        ProviderKind::TStore,
        move(migrationDetails)); // migration details

    WriteInfo(
        TraceComponent,
        "{0}: QueryStatus: {1}",
        this->TraceId,
        *resultSPtr);

    auto wrapperSPtr = make_shared<KeyValueStoreQueryResultWrapper>(move(resultSPtr));

    result = Api::IStatefulServiceReplicaStatusResultPtr(
        wrapperSPtr.get(),
        wrapperSPtr->CreateComponentRoot());

    return ErrorCodeValue::Success;

}

void TSReplicatedStore::SetQueryStatusDetails(wstring const & status)
{
    AcquireWriteLock lock(queryStatusDetailsLock_);

    queryStatusDetails_ = status;
}

void TSReplicatedStore::SetMigrationQueryResult(unique_ptr<MigrationQueryResult> && details)
{
    AcquireWriteLock lock(queryStatusDetailsLock_);

    migrationDetails_ = move(details);
}

void TSReplicatedStore::Test_SetTestHookContext(TestHooks::TestHookContext const & testHookContext)
{
    testHookContext_ = testHookContext;
}

AsyncOperationSPtr TSReplicatedStore::BeginOpen(
    ::FABRIC_REPLICA_OPEN_MODE openMode,
    ComPointer<::IFabricStatefulServicePartition> const & partition,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(openMode);
    
    {
        AcquireWriteLock lock(partitionLock_);

        partition_ = partition;
    }

    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        callback,
        parent);
}

ErrorCode TSReplicatedStore::EndOpen(
    AsyncOperationSPtr const & operation, 
    __out ComPointer<::IFabricReplicator> & replicator)
{
    return OpenAsyncOperation::End(operation, replicator);
}

AsyncOperationSPtr TSReplicatedStore::BeginChangeRole(
    ::FABRIC_REPLICA_ROLE role,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    replicaRole_ = role;

    return AsyncOperation::CreateAndStart<ChangeRoleAsyncOperation>(
        *this,
        callback,
        parent);
}

ErrorCode TSReplicatedStore::EndChangeRole(
    AsyncOperationSPtr const & operation, 
    __out wstring & result)
{
    UNREFERENCED_PARAMETER(result);
    return ChangeRoleAsyncOperation::End(operation);
}

AsyncOperationSPtr TSReplicatedStore::BeginClose(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        callback,
        parent);
}

ErrorCode TSReplicatedStore::EndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void TSReplicatedStore::Abort()
{
    ManualResetEvent event(false);

    auto operation = this->BeginClose(
        [&](AsyncOperationSPtr const &) { event.Set(); }, 
        this->Root.CreateAsyncOperationRoot());

    event.WaitOne();

    this->EndClose(operation).ReadValue();
}

AsyncOperationSPtr TSReplicatedStore::BeginOnDataLoss(
    __in AsyncCallback const & callback,
    __in AsyncOperationSPtr const & parent)
{
    if (storeEventHandler_.get() != nullptr)
    {
        WriteInfo(
            TraceComponent,
            "{0} BeginOnDataLoss",
            this->TraceId);

        return storeEventHandler_->BeginOnDataLoss(callback, parent);
    }
    else
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
    }
}

ErrorCode TSReplicatedStore::EndOnDataLoss(
    __in AsyncOperationSPtr const & operation,
    __out bool & isStateChanged)
{
    if (storeEventHandler_.get() != nullptr)
    {
        auto error = storeEventHandler_->EndOnDataLoss(operation, isStateChanged);

        WriteInfo(
            TraceComponent,
            "{0} EndOnDataLoss: error={1} isStateChanged={2}",
            this->TraceId,
            error,
            isStateChanged);

        return error;
    }
    else
    {
        isStateChanged = false;
        return CompletedAsyncOperation::End(operation);
    }
}

void TSReplicatedStore::TryUnregisterChangeHandler()
{
    if (changeHandler_.RawPtr() != nullptr)
    {
        changeHandler_->TryUnregister();
        
        changeHandler_.Reset();
    }
}

KAllocator & TSReplicatedStore::GetAllocator()
{
    return ktlSystem_->PagedAllocator();
}

ErrorCode TSReplicatedStore::InitializeReplicator(__inout ComPointer<IFabricReplicator> & replicatorResult)
{
    auto partition = this->TryGetPartition();
    if (partition.GetRawPointer() == nullptr)
    {
        WriteWarning(
            TraceComponent,
            "{0} TryGetPartition failed",
            this->TraceId);

        return ErrorCodeValue::InvalidState;
    }

    ComPointer<IFabricInternalStatefulServicePartition> internalPartition;
    auto hr = partition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress());
    if (FAILED(hr))
    {
        WriteWarning(
            TraceComponent,
            "{0} QueryInterface(IID_IFabricInternalStatefulServicePartition) failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr);
    }

    hr = internalPartition->GetKtlSystem(reinterpret_cast<void**>(&ktlSystem_));
    if (FAILED(hr))
    {
        WriteWarning(
            TraceComponent,
            "{0} GetKtlSystem failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr);
    }

    KSharedPtr<ComTransactionalReplicatorRuntimeConfigurations> runtimeConfigs;
    auto status = ComTransactionalReplicatorRuntimeConfigurations::Create(*this, storeSettings_->WorkingDirectory, this->GetAllocator(), runtimeConfigs);
    if (NT_ERROR(status))
    {
        return this->FromNtStatus("ComTransactionalReplicatorRuntimeConfigurations::Create", status);
    }

    KSharedPtr<ComStateProviderFactory> stateProviderFactory;
    status = ComStateProviderFactory::Create(*this, this->GetAllocator(), stateProviderFactory);
    if (NT_ERROR(status))
    {
        return this->FromNtStatus("ComStateProviderFactory::Create", status);
    }

    auto dataLossHandler = make_com<ComDataLossHandler>(IDataLossHandlerPtr(this, this->Root.CreateComponentRoot()));

    ScopedHeap heap;
    auto replicatorSettings = this->CreateReplicatorSettings(heap, replicatorSettings_);

    TRANSACTIONAL_REPLICATOR_SETTINGS txnReplicatorSettings = {0};
    KTLLOGGER_SHARED_LOG_SETTINGS txnSharedLogSettings = {L"", L"", 0, 0, 0};

    if (storeSettings_->SharedLogSettings.get() != nullptr)
    {
        WriteInfo(
            TraceComponent,
            "{0} overriding shared log settings: {1}",
            this->TraceId,
            *(storeSettings_->SharedLogSettings));

        storeSettings_->SharedLogSettings->ToPublicApi(heap, txnSharedLogSettings);
    }

    if (storeSettings_->TxnReplicatorSettings.get() != nullptr)
    {
        WriteInfo(
            TraceComponent,
            "{0} overriding transactional replicator settings: {1}",
            this->TraceId,
            *(storeSettings_->TxnReplicatorSettings));

        storeSettings_->TxnReplicatorSettings->ToPublicApi(txnReplicatorSettings);
    }

    if (storeSettings_->NotificationMode != SecondaryNotificationMode::None)
    {
        txnReplicatorSettings.Flags = FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT;
        txnReplicatorSettings.EnableSecondaryCommitApplyAcknowledgement = TRUE;
    }

    ComPointer<IFabricPrimaryReplicator> primaryReplicator;
    HANDLE txnReplicatorHandle = nullptr;

    hr = internalPartition->CreateTransactionalReplicatorInternal(
        runtimeConfigs.RawPtr(),
        stateProviderFactory.RawPtr(),
        dataLossHandler.GetRawPointer(),
        replicatorSettings,
        &txnReplicatorSettings,
        &txnSharedLogSettings,
        primaryReplicator.InitializationAddress(),
        &txnReplicatorHandle);

    if (FAILED(hr))
    {
        WriteWarning(
            TraceComponent,
            "{0} CreateTransactionalReplicatorInternal failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr);
    }

    hr = primaryReplicator->QueryInterface(IID_IFabricTransactionalReplicator, stateReplicator_.VoidInitializationAddress());
    if (FAILED(hr)) 
    { 
        WriteWarning(
            TraceComponent,
            "{0} QueryInterface(IID_IFabricTransactionalReplicator) failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr); 
    }

    hr = primaryReplicator->QueryInterface(IID_IFabricReplicator, replicatorResult.VoidInitializationAddress());
    if (FAILED(hr)) 
    { 
        WriteWarning(
            TraceComponent,
            "{0} QueryInterface(IID_IFabricReplicator) failed: {1}",
            this->TraceId,
            hr);

        return ErrorCode::FromHResult(hr); 
    }

    TxnReplicator::ITransactionalReplicator::SPtr txReplicator;
    {
        AcquireWriteLock lock(txReplicatorLock_);

        transactionalReplicator_.Attach(reinterpret_cast<TxnReplicator::ITransactionalReplicator *>(txnReplicatorHandle));

        txReplicator = transactionalReplicator_;
    }

    if (secondaryEventHandler_.get() != nullptr)
    {
        if (storeSettings_->NotificationMode == SecondaryNotificationMode::None)
        {
            WriteWarning(
                TraceComponent,
                "{0} notification mode cannot be none: requested={1}",
                this->TraceId,
                storeSettings_->NotificationMode); 

            return ErrorCodeValue::InvalidArgument;
        }

        status = TSChangeHandler::Create(*this, move(secondaryEventHandler_), changeHandler_);

        if (NT_SUCCESS(status))
        {
            WriteInfo(
                TraceComponent,
                "{0} secondary notifications enabled: {1}",
                this->TraceId,
                storeSettings_->NotificationMode);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} TSChangeHandler::Create failed: {1}",
                this->TraceId,
                hr);

            return this->FromNtStatus("TSChangeHandler::Create", status);
        }

        txReplicator->RegisterStateManagerChangeHandler(*changeHandler_.RawPtr());

    } // secondary event handler

    txReplicator->Test_SetTestHookContext(testHookContext_);

    return ErrorCodeValue::Success;
}

void TSReplicatedStore::RetryInitializeStore()
{
    if (this->GetIsActivePrimary())
    {
        this->ScheduleInitializeStore(StoreConfig::GetConfig().TStoreInitializationRetryDelay);
    }
    else
    {
        WriteInfo(TraceComponent, "{0} cancelling TStore initialization retry", this->TraceId);
    }
}

void TSReplicatedStore::ScheduleInitializeStore(TimeSpan const & delay)
{
    auto selfRoot = this->Root.CreateComponentRoot();
    {
        AcquireWriteLock lock(initializationTimerLock_);

        if (initializationTimer_.get() != nullptr)
        {
            initializationTimer_->Cancel();
        }

        initializationTimer_ = Timer::Create(TraceComponent, [&, selfRoot](TimerSPtr const & timer) 
        { 
            timer->Cancel();
            this->InitializeStoreTask(selfRoot); 
        });
        initializationTimer_->Change(delay);
    }
}

ktl::Task TSReplicatedStore::InitializeStoreTask(ComponentRootSPtr const & root)
{
    shared_ptr<ScopedActiveFlag> activeFlag;
    if (!ScopedActiveFlag::TryAcquire(isInitializationActive_, activeFlag))
    {
        WriteInfo(TraceComponent, "{0} TStore initialization already active", this->TraceId);

        co_return;
    }

    if (!this->InternalHasReadStatus())
    {
        WriteInfo(TraceComponent, "{0} no read status - scheduling TStore initialization retry", this->TraceId);

        this->RetryInitializeStore();

        co_return;
    }

    {
        AcquireReadLock lock(innerStoreLock_);

        if (innerStore_.RawPtr() != nullptr)
        {
            WriteInfo(TraceComponent, "{0} TStore already initialized", this->TraceId);

            co_return;
        }
    }

    auto error = co_await this->InitializeStoreAsync(root);

    if (!error.IsSuccess())
    {
        this->RetryInitializeStore();
    }

    co_return;
}

ktl::Awaitable<ErrorCode> TSReplicatedStore::InitializeStoreAsync(ComponentRootSPtr const & root)
{
    WriteInfo(TraceComponent, "{0} initializing TStore", this->TraceId);

    ComponentRootSPtr selfRoot = root;

    TxnReplicator::ITransactionalReplicator::SPtr txReplicator;
    auto error = this->TryGetTxReplicator(txReplicator);
    if (!error.IsSuccess()) { co_return error; }

    TxnReplicator::IStateProvider2::SPtr stateProvider;
    NTSTATUS status = txReplicator->Get(TStoreProviderName, stateProvider);

    if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        if (!this->InternalHasWriteStatus())
        {
            WriteInfo(TraceComponent, "{0} no write status - scheduling TStore initialization retry", this->TraceId);

            co_return ErrorCodeValue::NoWriteQuorum;
        }

        WriteInfo(TraceComponent, "{0} creating new TStore", this->TraceId);

        {
            TxnReplicator::Transaction::SPtr transaction;
            KFinally([&] {transaction->Dispose(); });

            status = txReplicator->CreateTransaction(transaction);

            if (!NT_SUCCESS(status))
            {
                co_return this->FromNtStatus("CreateTransaction", status);
            }

            status = co_await txReplicator->AddAsync(
                *transaction,
                TStoreProviderName,
                TStoreProviderType);
            if (!NT_SUCCESS(status))
            {
                co_return this->FromNtStatus("TransactionalReplicator::AddAsync", status);
            }

            status = co_await transaction->CommitAsync();
            if (!NT_SUCCESS(status))
            {
                co_return this->FromNtStatus("CommitAsync", status);
            }
        }

        status = txReplicator->Get(TStoreProviderName, stateProvider);
        if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
        {
            WriteWarning(
                TraceComponent,
                "{0} failed to get state provider '{1}'",
                this->TraceId,
                static_cast<LPCWSTR>(*TStoreProviderName));

            co_return ErrorCodeValue::OperationFailed;
        }
    }
    
    if (!NT_SUCCESS(status))
    {
        co_return this->FromNtStatus("TransactionalReplicator::Get", status);
    }

    bool success = false;
    {
        AcquireWriteLock lock(innerStoreLock_);

        innerStore_ = IKvs::SPtr(dynamic_cast<IKvs*>(stateProvider.RawPtr()));

        success = (innerStore_.RawPtr() != nullptr);
    }

    if (!success)
    {
        WriteWarning(
            TraceComponent,
            "{0} dynamic_pointer_cast<IStore> failed",
            this->TraceId);

        co_return ErrorCodeValue::OperationFailed;
    }

    initializedEvent_.Set();

    WriteInfo(TraceComponent, "{0} TStore initialized", this->TraceId);

    co_return ErrorCodeValue::Success;
}

void TSReplicatedStore::CancelInitializationTimer()
{
    AcquireWriteLock lock(initializationTimerLock_);

    if (initializationTimer_)
    {
        initializationTimer_->Cancel();
        initializationTimer_.reset();
    }
}

void TSReplicatedStore::WaitForInitialization()
{
    initializedEvent_.WaitOne();
}

::FABRIC_REPLICA_ROLE TSReplicatedStore::GetReplicatorRole() const
{
    TxnReplicator::ITransactionalReplicator::SPtr txReplicator;
    auto error = this->TryGetTxReplicator(txReplicator);
    if (!error.IsSuccess()) { return FABRIC_REPLICA_ROLE_NONE; }

    return txReplicator->Role;
}

void TSReplicatedStore::TransientFault(wstring const & message, ErrorCode const & error)
{
    WriteWarning(
        TraceComponent,
        "{0} TransientFault: reason={1} error={2} details='{3}'",
        this->TraceId,
        message,
        error,
        error.Message);

    partition_->ReportFault(FABRIC_FAULT_TYPE_TRANSIENT);
}

ErrorCode TSReplicatedStore::OnOpen(__inout ComPointer<IFabricReplicator> & replicatorResult)
{
    WriteInfo(
        TraceComponent,
        "{0} OnOpen {1}",
        this->TraceId,
        storeSettings_->WorkingDirectory);

    auto error = this->InitializeReplicator(replicatorResult);

    if (!error.IsSuccess())
    {
        this->Abort();
    }

    return error;
}

ErrorCode TSReplicatedStore::OnChangeRole()
{
    WriteInfo(
        TraceComponent,
        "{0} OnChangeRole {1}",
        this->TraceId,
        replicaRole_);

    if (this->GetIsActivePrimary())
    {
        this->ScheduleInitializeStore(TimeSpan::Zero);
    }
    else
    {
        this->CancelInitializationTimer();
    }

    return ErrorCodeValue::Success;
}

ErrorCode TSReplicatedStore::OnClose()
{
    WriteInfo(
        TraceComponent,
        "{0} OnClose",
        this->TraceId);

    if (isActive_.exchange(false))
    {
        this->TryUnregisterChangeHandler();

        {
            AcquireWriteLock lock(txReplicatorLock_);

            if (transactionalReplicator_.RawPtr() != nullptr)
            {
                transactionalReplicator_->UnRegisterStateManagerChangeHandler();

                transactionalReplicator_.Reset();
            }
        }

        AcquireWriteLock lock(partitionLock_);

        stateReplicator_.Release();

        storeEventHandler_ = IStoreEventHandlerPtr();

        secondaryEventHandler_ = ISecondaryEventHandlerPtr();

        partition_.Release();
    }

    return ErrorCodeValue::Success;
}

bool TSReplicatedStore::ApplicationCanReadWrite()
{
    return (this->GetIsActivePrimary() && this->TryGetTStore().RawPtr() != nullptr);
}

bool TSReplicatedStore::HasReadStatus()
{
    if (!this->ApplicationCanReadWrite()) { return false; }

    return this->InternalHasReadStatus();
}

bool TSReplicatedStore::InternalHasReadStatus()
{
    auto partition = this->TryGetPartition();
    if (partition.GetRawPointer() == nullptr) { return false; }

    FABRIC_SERVICE_PARTITION_ACCESS_STATUS status;
    auto hr = partition->GetReadStatus(&status);

    if (FAILED(hr))
    {
        this->WriteWarning(
            TraceComponent,
            "{0} GetReadStatus failed: {1}",
            this->TraceId,
            hr);

        return false;
    }

    return (status == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
}

bool TSReplicatedStore::HasWriteStatus()
{
    if (!this->ApplicationCanReadWrite()) { return false; }

    return this->InternalHasWriteStatus();
}

bool TSReplicatedStore::InternalHasWriteStatus()
{
    auto partition = this->TryGetPartition();
    if (partition.GetRawPointer() == nullptr) { return false; }

    FABRIC_SERVICE_PARTITION_ACCESS_STATUS status;
    auto hr = partition->GetWriteStatus(&status);

    if (FAILED(hr))
    {
        this->WriteWarning(
            TraceComponent,
            "{0} GetWriteStatus failed: {1}",
            this->TraceId,
            hr);

        return false;
    }

    return (status == FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
}

ComPointer<IFabricStatefulServicePartition> TSReplicatedStore::TryGetPartition()
{
    AcquireReadLock lock(partitionLock_);

    return partition_;
}

IKvs::SPtr TSReplicatedStore::TryGetTStore() const
{
    AcquireReadLock lock(innerStoreLock_);

    return innerStore_;
}

IKvsTransaction::SPtr TSReplicatedStore::GetTStoreTransaction(TransactionSPtr const & txSPtr)
{
    TSTransaction * castedTx = dynamic_cast<TSTransaction *>(txSPtr.get()); 
    ASSERT_IF(castedTx == nullptr, "Failed to cast to TSTransaction");
    return castedTx->GetTStoreTransaction();
}

ErrorCode TSReplicatedStore::TryGetTxReplicator(__out TxnReplicator::ITransactionalReplicator::SPtr & txReplicator) const
{
    AcquireReadLock lock(txReplicatorLock_);

    if (transactionalReplicator_.RawPtr() == nullptr)
    {
        return ErrorCodeValue::ObjectClosed;
    }
    else
    {
        txReplicator = transactionalReplicator_;

        return ErrorCodeValue::Success;
    }
}

FABRIC_REPLICATOR_SETTINGS const * TSReplicatedStore::CreateReplicatorSettings(
    __in ScopedHeap & heap, 
    Reliability::ReplicationComponent::ReplicatorSettingsUPtr const & inputSettings)
{
    auto resultSettings = heap.AddItem<FABRIC_REPLICATOR_SETTINGS>();
    inputSettings->ToPublicApi(heap, *resultSettings);

    if (resultSettings->Reserved != nullptr)
    {
        auto ex1 = reinterpret_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(resultSettings->Reserved);

        if (ex1->Reserved != nullptr)
        {
            auto ex2 = reinterpret_cast<FABRIC_REPLICATOR_SETTINGS_EX2*>(ex1->Reserved);

            // Not supported by v2 replicator yet
            //
            ex2->UseStreamFaultsAndEndOfStreamOperationAck = FALSE;
        }
    }

    return resultSettings.GetRawPointer();
}

ErrorCode TSReplicatedStore::CreateKey(std::wstring const & type, std::wstring const & key, __out KString::SPtr & result)
{
    return TSComponent::CreateKey(type, key, this->GetAllocator(), result);
}

ErrorCode TSReplicatedStore::CreateBuffer(
    void const * value,
    size_t valueSize,
    __out KBuffer::SPtr & result)
{
    // TODO: This can be optimized if KBuffer can be initialized with just the pointer 
    //       and size without copying the bytes since the caller is just passing the
    //       pointer and size from a KBuffer anyway.
    //
    //       Alternatively, change all the APIs to take KBuffer.
    //
    auto status = KBuffer::CreateOrCopy(result, value, static_cast<ULONG>(valueSize), this->GetAllocator());

    return this->FromNtStatus("CreateBuffer", status);
}

StringLiteral const & TSReplicatedStore::GetTraceComponent() const
{
    return TraceComponent;
}

wstring const & TSReplicatedStore::GetTraceId() const
{
    return this->TraceId;
}
