//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Store;

StringLiteral const TraceComponent("KeyValueStoreMigrator");    

WStringLiteral const AzureBlobWinFabEtw(L"AzureBlobWinFabEtw");    
WStringLiteral const AzureBlobWinFabEtwInMemory(L"AzureBlobWinFabEtwInMemory");    
WStringLiteral const FileShareWinFabEtw(L"FileShareWinFabEtw");    
WStringLiteral const StoreConnectionString(L"StoreConnectionString");    
WStringLiteral const ReplicatedStoreSection(L"ReplicatedStore");    
WStringLiteral const MigrationBackupConnectionString(L"MigrationBackupConnectionString");    
WStringLiteral const FilePrefix(L"file:");    
WStringLiteral const XStorePrefix(L"xstore:");    

//
// TransactionContext
//

class KeyValueStoreMigrator::TransactionContext
{
public:
    TransactionContext(TransactionSPtr && txSPtr)
        : txSPtr_(move(txSPtr))
        , updatedKeys_()
        , deletedKeys_()
        , isCommitted_(false)
    {
    }

    __declspec(property(get=get_Transaction)) TransactionSPtr const & Transaction;
    __declspec(property(get=get_UpdatedKeys)) unordered_set<wstring> const & UpdatedKeys;
    __declspec(property(get=get_DeletedKeys)) unordered_set<wstring> const & DeletedKeys;
    __declspec(property(get=get_IsCommitted)) bool IsCommitted;

    TransactionSPtr const & get_Transaction() const { return txSPtr_; }
    unordered_set<wstring> const & get_UpdatedKeys() const { return updatedKeys_; }
    unordered_set<wstring> const & get_DeletedKeys() const { return deletedKeys_; }
    bool get_IsCommitted() const { return isCommitted_; }

    void OnUpdate(wstring const & type, wstring const & key)
    {
        auto fullKey = KeyValueStoreMigrator::GetFullKey(type, key);

        updatedKeys_.insert(fullKey);
        deletedKeys_.erase(fullKey);
    }

    void OnDelete(wstring const & type, wstring const & key)
    {
        auto fullKey = KeyValueStoreMigrator::GetFullKey(type, key);

        updatedKeys_.erase(fullKey);
        deletedKeys_.insert(fullKey);
    }

    void MarkCommitted()
    {
        isCommitted_ = true;
    }

private:
    TransactionSPtr txSPtr_;
    unordered_set<wstring> updatedKeys_;
    unordered_set<wstring> deletedKeys_;
    bool isCommitted_;
};

// 
// MigrationAsyncOperation
//

class KeyValueStoreMigrator::MigrationAsyncOperation : public AsyncOperation
{
public:
    MigrationAsyncOperation(
        __in KeyValueStoreMigrator & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , destTx_()
        , startType_()
        , startKey_()
        , isComplete_(false)
        , latestType_()
        , latestKey_()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<MigrationAsyncOperation>(operation)->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto phase = owner_.GetMigrationPhase();
        auto state = owner_.GetMigrationState();

        if (state == MigrationState::Completed)
        {
            WriteInfo(
                TraceComponent, 
                "{0} migration phase already completed: {1}",
                owner_.TraceId,
                phase);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);

            return;
        }

        WriteInfo(
            TraceComponent, 
            "{0} entering migration phase: {1}",
            owner_.TraceId,
            phase);

        switch (phase)
        {
        case MigrationPhase::Migration:
            this->PerformMigration(thisSPtr);
            break;

        case MigrationPhase::TargetDatabaseSwap:
            this->PerformSwap(thisSPtr);
            break;

        case MigrationPhase::TargetDatabaseCleanup:
            this->PerformTargetCleanup(thisSPtr);
            break;

        case MigrationPhase::SourceDatabaseCleanup:
            this->PerformSourceCleanup(thisSPtr);
            break;

        case MigrationPhase::RestoreSourceBackup:
            this->PerformRestoreSource(thisSPtr);
            break;

        default:
            this->PerformDefault(thisSPtr);
            break;
        }
    }

private:

    void PerformMigration(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.SetMigrationState(MigrationState::Processing);

        auto error = owner_.CreateCleanTStoreTarget();

        if (error.IsSuccess())
        {
            this->PerformNextMigrationBatch(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void PerformNextMigrationBatch(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.ProcessMigrationBatch(startType_, startKey_, destTx_, isComplete_, latestType_, latestKey_);

        if (error.IsSuccess())
        {
            auto operation = destTx_->BeginCommit(
                owner_.Settings.CommitTimeout,
                [this](AsyncOperationSPtr const & operation) { this->OnCommitMigrationBatchComplete(operation, false); },
                thisSPtr);
            this->OnCommitMigrationBatchComplete(operation, true);
        }
        else if (error.IsError(ErrorCodeValue::StoreWriteConflict))
        {
            Threadpool::Post(
                [this, thisSPtr]() { this->PerformNextMigrationBatch(thisSPtr); },
                owner_.Settings.ConflictRetryDelay);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void OnCommitMigrationBatchComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error =  destTx_->EndCommit(operation);
        
        if (error.IsSuccess())
        {
            if (isComplete_)
            {
                this->FinishMigration(thisSPtr);
            }
            else
            {
                startType_ = latestType_;
                startKey_ = latestKey_;

                this->PerformNextMigrationBatch(thisSPtr);
            }
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} failed to commit migration batch: {1}",
                owner_.TraceId,
                error);

            this->TryComplete(thisSPtr, error);
        }
    }

    void FinishMigration(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.FinishMigration();

        this->TryComplete(thisSPtr, error);
    }

    void PerformSwap(AsyncOperationSPtr const & thisSPtr)
    {
        // Nothing to do - source store provider type was 
        // already overridden in KeyValueStoreReplica::InitializeForSystemServices()
        //
        owner_.SetMigrationState(MigrationState::Completed);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    void PerformTargetCleanup(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.SetMigrationState(MigrationState::Processing);

        auto error = owner_.AttachTStoreTarget();

        if (error.IsSuccess())
        {
            this->PerformCleanup(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void PerformSourceCleanup(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.SetMigrationState(MigrationState::Processing);

        auto error = owner_.AttachExistingEseTarget();

        if (error.IsSuccess())
        {
            this->PerformCleanup(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void PerformCleanup(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.CleanupTarget();

        if (error.IsSuccess())
        {
            owner_.SetMigrationState(MigrationState::Completed);
        }

        this->TryComplete(thisSPtr, error);
    }

    void PerformRestoreSource(AsyncOperationSPtr const & thisSPtr)
    {
        // Nothing to do - the restore operation is triggered
        // only once when updating the initialization data
        //
        owner_.SetMigrationState(MigrationState::Completed);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    void PerformDefault(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.SetMigrationState(MigrationState::Inactive);

        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
    }


    KeyValueStoreMigrator & owner_;
    TransactionSPtr destTx_;
    wstring startType_;
    wstring startKey_;
    bool isComplete_;
    wstring latestType_;
    wstring latestKey_;
};

//
// KeyValueStoreMigrator
//

KeyValueStoreMigrator::KeyValueStoreMigrator(
    ComponentRoot const & root,
    Store::PartitionedReplicaId const & trace,
    __in IReplicatedStore & sourceStore,
    __in TSReplicatedStoreSettingsUPtr && tstoreSettings,
    unique_ptr<MigrationInitData> && initData)
    : RootedObject(root)
    , PartitionedReplicaTraceComponent<TraceTaskCodes::ReplicatedStore>(trace)
    , sourceReplicatedStore_(sourceStore)
    , tstoreSettings_(move(tstoreSettings))
    , targetLocalStore_()
    , initData_(move(initData))
    , phase_(initData_->GetPhase(trace.PartitionId))
    , settings_()
    , state_(MigrationState::Inactive)
    , stateLock_()
    , isCanceled_(false)
    , isFinished_(false)
    , migrationCount_(0)
    , uncommittedDeletes_()
    , deletedKeys_()
    , deletedKeysLock_()
    , txMap_()
    , txMapLock_()
    , txCount_(0)
    , txCountEvent_(false)
{
}

ErrorCode KeyValueStoreMigrator::CreateForTStoreMigration(
    ComponentRoot const & root,
    Store::PartitionedReplicaId const & trace,
    __in IReplicatedStore & sourceStore, 
    TSReplicatedStoreSettingsUPtr && tstoreSettings,
    unique_ptr<MigrationInitData> && initData,
    __out KeyValueStoreMigratorSPtr & result)
{
    result = unique_ptr<KeyValueStoreMigrator>(new KeyValueStoreMigrator(root, trace, sourceStore, move(tstoreSettings), move(initData)));

    return ErrorCodeValue::Success;
}

ErrorCode KeyValueStoreMigrator::CreateCleanTStoreTarget()
{
    auto error = this->CleanupTStoreTargetFiles();
    if (!error.IsSuccess()) { return error; }

    error = this->AttachTStoreTarget();
    if (!error.IsSuccess()) { return error; }

    sourceReplicatedStore_.SetTxEventHandler(shared_from_this());

    return error;
}

// The local store's existing cleanup mechanism (ChangeRole(None)) isn't suitable
// for migration since it can get stuck or fail trying to first open and recover 
// a previous failed migration - prefer deleting the shared and dedicated log files directly.
//
ErrorCode KeyValueStoreMigrator::CleanupTStoreTargetFiles()
{
    auto error = tstoreSettings_->DeleteDatabaseFiles(this->PartitionId, this->ReplicaId);
    if (!error.IsSuccess())
    {
        return TraceAndGetError(wformatString("CleanupTStoreTargetFiles failed: {0}", error.Message), error);
    }

    return error;
}

ErrorCode KeyValueStoreMigrator::AttachTStoreTarget()
{
    targetLocalStore_ = CreateTStoreTarget();

    return this->InitializeTarget(*targetLocalStore_);
}

shared_ptr<ILocalStore> KeyValueStoreMigrator::CreateTStoreTarget()
{
    // No KtlLoggerNode in this context, so use the default KTL system rather than node KTL system
    // for the migration target. The correct KTL system will get used after migration completes and
    // the database is swapped.
    //
    return shared_ptr<ILocalStore>(new TSLocalStore(make_unique<TSReplicatedStoreSettings>(*tstoreSettings_), nullptr)); 
}

ErrorCode KeyValueStoreMigrator::AttachExistingEseTarget()
{
    auto phase = this->GetMigrationPhase();

    if (phase != MigrationPhase::SourceDatabaseCleanup)
    {
        return TraceAndGetError(wformatString("invalid phase for TStoreTarget: {0}", phase), ErrorCodeValue::OperationFailed);
    }

    targetLocalStore_ = CreateEseTarget();

    // Do not initialize ESE store.
    // Attaching to the existing ESE store as target
    // is used for post-migration cleanp only.
    //
    return ErrorCodeValue::Success;
}

shared_ptr<ILocalStore> KeyValueStoreMigrator::CreateEseTarget()
{
    // Use mock EDB filename - only the database directory matters for cleanup
    //
    auto eseRoot = EseReplicatedStore::CreateLocalStoreRootDirectory(tstoreSettings_->WorkingDirectory, this->PartitionId);
    auto eseDir = EseReplicatedStore::CreateLocalStoreDatabaseDirectory(eseRoot, this->ReplicaId);
    EseLocalStoreSettings eseSettings(wformatString("{0}.edb", TraceComponent), eseRoot);
    auto eseFlags = EseLocalStore::LOCAL_STORE_FLAG_NONE;

    return shared_ptr<ILocalStore>(new EseLocalStore(eseDir, eseSettings, eseFlags));
}

bool KeyValueStoreMigrator::TryUpdateInitializationData(unique_ptr<MigrationInitData> && initData)
{
    auto currentPhase = this->GetMigrationPhase();
    auto targetPhase = initData->GetPhase(this->PartitionId);
    auto state = this->GetMigrationState();

    WriteInfo(TraceComponent, "{0} Migration phase init data updated: current={1} target={2} state={3}", this->TraceId, currentPhase, targetPhase, state);

    if (targetPhase == MigrationPhase::RestoreSourceBackup)
    {
        return this->TryRestoreSourceStore();
    }
    else
    {
        return (currentPhase != targetPhase || state == MigrationState::Failed);
    }
}

MigrationSettings const & KeyValueStoreMigrator::get_Settings() const
{
    if (initData_->Settings.get() != nullptr)
    {
        return *(initData_->Settings);
    }
    else
    {
        return settings_;
    }
}

AsyncOperationSPtr KeyValueStoreMigrator::BeginMigration(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<MigrationAsyncOperation>(*this, callback, parent);
}

ErrorCode KeyValueStoreMigrator::EndMigration(AsyncOperationSPtr const & operation)
{
    auto error = MigrationAsyncOperation::End(operation);

    if (error.IsSuccess())
    {
        this->OnMigrationSuccess();
    }
    else if (error.IsError(ErrorCodeValue::OperationCanceled))
    {
        this->OnMigrationCanceled();
    }
    else
    {
        this->OnMigrationFailure(error);
    }

    return error;
}

MigrationState::Enum KeyValueStoreMigrator::GetMigrationState() const
{
    AcquireReadLock lock(stateLock_);

    return state_;
}

MigrationPhase::Enum KeyValueStoreMigrator::GetMigrationPhase() const
{
    return phase_;
}

ErrorCode KeyValueStoreMigrator::InitializeTarget(__in ILocalStore & targetStore)
{
    FABRIC_EPOCH epoch = {0};
    auto error = sourceReplicatedStore_.GetCurrentEpoch(epoch);
    if (!error.IsSuccess()) 
    { 
        return TraceAndGetError(L"failed to get current epoch from source", error);
    }

    error = targetStore.Initialize(
        wformatString("{0}.target", TraceComponent),
        this->PartitionId,
        this->ReplicaId,
        epoch);

    if (!error.IsSuccess()) 
    { 
        return TraceAndGetError(L"failed to initialize target", error);
    }
    
    return error;
}

ErrorCode KeyValueStoreMigrator::ProcessMigrationBatch(
    wstring const & startType,
    wstring const & startKey,
    __out TransactionSPtr & destTx,
    __out bool & isComplete,
    __out wstring & currentType,
    __out wstring & currentKey)
{
    if (!sourceReplicatedStore_.IsMigrationSourceSupported())
    {
        return TraceAndGetError(L"source store does not support migration", ErrorCodeValue::NotImplemented);
    }

    destTx.reset();
    isComplete = false;

    WriteInfo(
        TraceComponent, 
        "{0} starting migration batch: type='{1}' key='{2}'", 
        this->TraceId,
        startType,
        startKey);

    auto error = targetLocalStore_->CreateTransaction(destTx);
    if (!error.IsSuccess()) 
    { 
        WriteInfo(
            TraceComponent, 
            "{0} failed to create destination tx: {1}",
            this->TraceId,
            error);

        return error; 
    }

    TransactionSPtr srcTx;
    IStoreBase::EnumerationSPtr srcEnum;
    error = sourceReplicatedStore_.CreateEnumerationByPrimaryIndex(
        nullptr, // ILocalStoreUPtr
        startType,
        startKey,
        srcTx,
        srcEnum);

    if (!error.IsSuccess()) 
    { 
        WriteInfo(
            TraceComponent, 
            "{0} failed to source enumeration: {1}",
            this->TraceId,
            error);

        return error; 
    }

    int count = 0;
    while (error.IsSuccess() && count <= this->Settings.MigrationBatchSize)
    {
        if (isCanceled_)
        {
            WriteInfo(
                TraceComponent, 
                "{0} cancelling migration",
                this->TraceId);

            return ErrorCodeValue::OperationCanceled;
        }

        ++count;
        error = srcEnum->MoveNext();

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            isComplete = true;

            error = ErrorCodeValue::Success;
            break;
        }
        else if (!error.IsSuccess()) 
        { 
            return error; 
        }

        error = srcEnum->CurrentType(currentType);
        if (!error.IsSuccess()) { return error; }

        error = srcEnum->CurrentKey(currentKey);
        if (!error.IsSuccess()) { return error; }

        if (!sourceReplicatedStore_.ShouldMigrateKey(currentType, currentKey))
        {
            WriteNoise(
                TraceComponent, 
                "{0} skipping migration of metadata: type={1} key={2}",
                this->TraceId,
                currentType,
                currentKey);

            continue;
        }

        if (this->DeletingKeyExists(currentType, currentKey))
        {
            WriteNoise(
                TraceComponent, 
                "{0} conflict on migration of uncommitted deleting key: type={1} key={2}",
                this->TraceId,
                currentType,
                currentKey);

            return ErrorCodeValue::StoreWriteConflict;
        }

        if (this->DeletedKeyExists(currentType, currentKey))
        {
            WriteNoise(
                TraceComponent, 
                "{0} skipping migration of deleted key: type={1} key={2}",
                this->TraceId,
                currentType,
                currentKey);

            continue;
        }

        vector<byte> currentValue;
        error = srcEnum->CurrentValue(currentValue);
        if (!error.IsSuccess()) { return error; }

        error = targetLocalStore_->Insert(
            destTx,
            currentType,
            currentKey,
            currentValue.data(),
            currentValue.size());

        if (error.IsSuccess())
        {
            WriteNoise(
                TraceComponent, 
                "{0} migrated: type={1} key={2}",
                this->TraceId,
                currentType,
                currentKey);
        }
        else if (error.IsError(ErrorCodeValue::StoreRecordAlreadyExists)) 
        { 
            WriteInfo(
                TraceComponent, 
                "{0} ignore existing entry: type='{1}' key='{2}'", 
                this->TraceId,
                currentType,
                currentKey);

            error = ErrorCodeValue::Success; 
        }
        else if (!error.IsSuccess()) 
        { 
            return TraceAndGetError(wformatString(
                "failed to migrate: type={0} key={1}",
                currentType,
                currentKey),
                error);
        }
    }

    migrationCount_ += count;

    auto msg = wformatString(GET_STORE_RC( MigrationBatchProgress ),
        currentType,
        currentKey,
        error,
        count,
        migrationCount_,
        isComplete);

    WriteInfo(TraceComponent, "{0} {1}", this->TraceId, msg);

    sourceReplicatedStore_.SetQueryStatusDetails(msg);

    return error;
}

ErrorCode KeyValueStoreMigrator::FinishMigration()
{
    WriteInfo(TraceComponent, "{0} flushing transactions", this->TraceId);

    // Once the destination store is closed below, any further attempts to create or
    // use destination transactions will fail. Prevent creation of any new 
    // destination transactions and wait for all existing destination transactions
    // to complete before closing the destination store. Otherwise, an outstanding
    // destination transaction can cause the migration to enter a failed state.
    //
    size_t outstandingTxCount = 0;
    {
        AcquireWriteLock lock(txMapLock_);

        isFinished_ = true;

        outstandingTxCount = txCount_;
    }

    // Cancel any remaining in-flight source transactions. Further transaction
    // creation will be blocked temporarily by the isFinished_ flag while 
    // draining these transactions.
    //
    sourceReplicatedStore_.AbortOutstandingTransactions();

    if (outstandingTxCount > 0)
    {
        WriteInfo(TraceComponent, "waiting for {0} outstanding destination transactions", this->TraceId);

        txCountEvent_.WaitOne();

        txCountEvent_.Reset();
    }

    // Disconnect the tx event handler while there are no outstanding transactions. 
    // The source store implementation may allow reads, but must block writes
    // for the remainder of the migration.
    //
    sourceReplicatedStore_.ClearTxEventHandlerAndBlockWrites();

    auto error = this->BackupSourceStore();

    if (error.IsSuccess())
    {
        error = this->CloseTarget();
    }

    return error;
}

void KeyValueStoreMigrator::OnMigrationSuccess()
{
    WriteInfo(TraceComponent, "{0} migration phase {1} complete", this->TraceId, this->GetMigrationPhase());

    this->SetMigrationState(MigrationState::Completed);
}

void KeyValueStoreMigrator::OnMigrationCanceled()
{
    WriteWarning(TraceComponent, "{0} migration phase {1} canceled", this->TraceId, this->GetMigrationPhase());

    this->SetMigrationState(MigrationState::Canceled);
}

void KeyValueStoreMigrator::OnMigrationFailure(ErrorCode const & error)
{
    auto msg = wformatString(GET_STORE_RC( MigrationPhaseFailed ), this->GetMigrationPhase(), error, error.Message);

    WriteWarning(TraceComponent, "{0} {1}", this->TraceId, msg);

    this->FailMigration(msg);
}

ErrorCode KeyValueStoreMigrator::CloseTarget()
{
    WriteInfo(TraceComponent, "{0} terminating destination store", this->TraceId);

    auto error = targetLocalStore_->Terminate();

    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} failed to terminate destination store: {1}", this->TraceId, error);
    }

    return error;
}

ErrorCode KeyValueStoreMigrator::CleanupTarget()
{
    auto error = targetLocalStore_->Cleanup();

    if (!error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} failed to cleanup destination store: {1}", this->TraceId, error);
    }

    return error;
}

void KeyValueStoreMigrator::Cancel()
{
    isCanceled_ = true;
}

void KeyValueStoreMigrator::FailMigration(wstring const & statusDetails)
{
    this->SetMigrationState(MigrationState::Failed);

    if (!statusDetails.empty())
    {
        sourceReplicatedStore_.SetQueryStatusDetails(statusDetails);
    }

    this->Cancel();
}

void KeyValueStoreMigrator::SetMigrationState(MigrationState::Enum state)
{
    AcquireWriteLock lock(stateLock_);

    if (state_ == MigrationState::Failed || state_ == state)
    {
        WriteInfo(
            TraceComponent, 
            "{0} ignoring state update: current={1} target={2}",
            this->TraceId,
            state_,
            state);

        return;
    }

    WriteInfo(TraceComponent, "{0} setting migration state: {1}->{2}", this->TraceId, state_, state);

    state_ = state;

    auto phase = this->GetMigrationPhase();

    sourceReplicatedStore_.SetMigrationQueryResult(make_unique<MigrationQueryResult>(
        phase,
        state,
        this->GetNextMigrationPhase(phase, state)));
}

//
// IReplicatedStoreTxEventHandler
//

void KeyValueStoreMigrator::OnCreateTransaction(ActivityId const & activityId, TransactionMapKey txMapKey)
{
    if (isCanceled_ || isFinished_) { return; }

    bool shouldFailMigration = false;
    wstring failureStatus;
    {
        AcquireWriteLock lock(txMapLock_);

        if (isCanceled_ || isFinished_) { return; }

        TransactionSPtr destTx;
        auto error = targetLocalStore_->CreateTransaction(destTx);
        if (error.IsSuccess())
        {
            ++txCount_;

            auto result = txMap_.insert(make_pair(txMapKey, make_shared<TransactionContext>(move(destTx)))); 
            if (!result.second)
            {
                shouldFailMigration = true;

                failureStatus = wformatString(GET_STORE_RC( MigrationSourceTxExists ), txMapKey);

                WriteWarning(TraceComponent, "{0} {1}", activityId, failureStatus);
            }
        }
        else
        {
            shouldFailMigration = true;

            failureStatus = wformatString(GET_STORE_RC( MigrationCreateTxFailed ), error);

            WriteWarning(TraceComponent, "{0} {1}", activityId, failureStatus);
        }
    }

    if (shouldFailMigration)
    {
        this->FailMigration(failureStatus);
    }
}

void KeyValueStoreMigrator::OnReleaseTransaction(ActivityId const & activityId, TransactionMapKey txMapKey)
{
    // Don't check isCanceled_ 
    // (allow release if isCanceled_ changes after tx creation)
    //
    //
    this->FinalizeRelease(activityId, txMapKey);

    auto count = --txCount_;

    if (count == 0 && isFinished_)
    {
        txCountEvent_.Set();
    }
}

ErrorCode KeyValueStoreMigrator::OnInsert(
    ActivityId const & activityId,
    TransactionMapKey txMapKey,
    std::wstring const & type,
    std::wstring const & key,
    void const * value,
    size_t const valueSizeInBytes)
{
    if (isCanceled_) { return ErrorCodeValue::NotPrimary; }
    if (isFinished_){ return ErrorCodeValue::DatabaseMigrationInProgress; }

    auto txContext = this->GetDestinationTxContext(activityId, txMapKey);

    if (txContext.get() == nullptr)
    {
        // Fail migration but not primary replica write operations
        //
        return ErrorCodeValue::Success;
    }

    auto error = targetLocalStore_->Insert(
        txContext->Transaction,
        type,
        key,
        value,
        valueSizeInBytes);

    if (error.IsSuccess())
    {
        this->UntrackDeletingKey(txContext, type, key);
    }
    else
    {
        return TraceAndGetError(wformatString("failed to mirror insert: type={0} key={1}", type, key), error);
    }

    return error;
}

ErrorCode KeyValueStoreMigrator::OnUpdate(
    ActivityId const & activityId,
    TransactionMapKey txMapKey,
    std::wstring const & type,
    std::wstring const & key,
    void const * newValue,
    size_t const valueSizeInBytes)
{
    if (isCanceled_) { return ErrorCodeValue::NotPrimary; }
    if (isFinished_){ return ErrorCodeValue::DatabaseMigrationInProgress; }

    auto txContext = this->GetDestinationTxContext(activityId, txMapKey);

    if (txContext.get() == nullptr)
    {
        // Fail migration but not primary replica write operations
        //
        return ErrorCodeValue::Success;
    }

    auto error = targetLocalStore_->Update(
        txContext->Transaction,
        type,
        key,
        ILocalStore::SequenceNumberIgnore,
        key,
        newValue,
        valueSizeInBytes);

    if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
    {
        WriteInfo(
            TraceComponent, 
            "{0} converting mirrored update to insert: type={1} key={2}",
            activityId,
            type,
            key);

        error = targetLocalStore_->Insert(
            txContext->Transaction,
            type,
            key,
            newValue,
            valueSizeInBytes);
    }

    if (error.IsSuccess())
    {
        this->UntrackDeletingKey(txContext, type, key);
    }
    else
    {
        return TraceAndGetError(wformatString("failed to mirror update: type={0} key={1}", type, key), error);
    }

    return error;
}

ErrorCode KeyValueStoreMigrator::OnDelete(
    ActivityId const & activityId,
    TransactionMapKey txMapKey,
    std::wstring const & type,
    std::wstring const & key)
{
    if (isCanceled_) { return ErrorCodeValue::NotPrimary; }
    if (isFinished_){ return ErrorCodeValue::DatabaseMigrationInProgress; }

    auto txContext = this->GetDestinationTxContext(activityId, txMapKey);

    if (txContext.get() == nullptr)
    {
        // Fail migration but not primary replica write operations
        //
        return ErrorCodeValue::Success;
    }

    auto error = targetLocalStore_->Delete(
        txContext->Transaction,
        type,
        key);

    if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
    {
        WriteInfo(
            TraceComponent, 
            "{0} no-op mirrored delete on missing key: type={1} key={2}",
            activityId,
            type,
            key);

        error = ErrorCodeValue::Success;
    }

    if (error.IsSuccess())
    {
        this->TrackDeletingKey(txContext, type, key);
    }
    else
    {
        return TraceAndGetError(wformatString("failed to mirror delete: type={0} key={1}", type, key), error);
    }

    return error;
}

ErrorCode KeyValueStoreMigrator::OnCommit(
    ActivityId const & activityId,
    TransactionMapKey txMapKey)
{
    if (isCanceled_) { return ErrorCodeValue::NotPrimary; }
    if (isFinished_){ return ErrorCodeValue::DatabaseMigrationInProgress; }

    auto txContext = this->GetDestinationTxContext(activityId, txMapKey);

    if (txContext.get() == nullptr)
    {
        // Fail migration but not primary replica write operations
        //
        return ErrorCodeValue::StoreUnexpectedError;
    }

    auto destTx = txContext->Transaction;

    auto operation = destTx->BeginCommit(
        this->Settings.CommitTimeout,
        [this, activityId, destTx](AsyncOperationSPtr const & operation) { this->OnTargetCommitComplete(activityId, destTx, operation, false); },
        this->Root.CreateAsyncOperationRoot());
    this->OnTargetCommitComplete(activityId, destTx, operation, true);

    return ErrorCodeValue::Success;
}

void KeyValueStoreMigrator::OnTargetCommitComplete(
    ActivityId const & activityId,
    TransactionSPtr const & destTx,
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto error = destTx->EndCommit(operation);

    if (!error.IsSuccess())
    {
        auto msg = GET_STORE_RC( MigrationMirrorCommitFailed );

        WriteWarning(TraceComponent, "{0} {1}", activityId, msg);

        if (!isFinished_)
        {
            this->FailMigration(msg);
        }
    }
}

KeyValueStoreMigrator::TransactionContextSPtr KeyValueStoreMigrator::GetDestinationTxContext(ActivityId const & activityId, TransactionMapKey txMapKey)
{
    AcquireReadLock lock(txMapLock_);

    auto findIt = txMap_.find(txMapKey);
    if (findIt == txMap_.end())
    {
        auto msg = wformatString(GET_STORE_RC( MigrationDestTxLookupFailed ), txMapKey);

        WriteWarning(TraceComponent, "{0} {1}", activityId, txMapKey);

        this->FailMigration(msg);

        return TransactionContextSPtr();
    }
    else
    {
        return findIt->second;
    }
}

void KeyValueStoreMigrator::TrackDeletingKey(TransactionContextSPtr const & txContext, wstring const & type, wstring const & key)
{
    AcquireWriteLock lock(deletedKeysLock_);

    uncommittedDeletes_.insert(GetFullKey(type, key));

    txContext->OnDelete(type, key);
}

void KeyValueStoreMigrator::UntrackDeletingKey(TransactionContextSPtr const & txContext, wstring const & type, wstring const & key)
{
    AcquireWriteLock lock(deletedKeysLock_);

    uncommittedDeletes_.erase(GetFullKey(type, key));

    txContext->OnUpdate(type, key);
}

bool KeyValueStoreMigrator::DeletingKeyExists(wstring const & type, wstring const & key)
{
    AcquireReadLock lock(deletedKeysLock_);

    return (uncommittedDeletes_.find(GetFullKey(type, key)) != uncommittedDeletes_.end());
}

bool KeyValueStoreMigrator::DeletedKeyExists(wstring const & type, wstring const & key)
{
    AcquireReadLock lock(deletedKeysLock_);

    return (deletedKeys_.find(GetFullKey(type, key)) != deletedKeys_.end());
}

KeyValueStoreMigrator::TransactionSPtr KeyValueStoreMigrator::FinalizeCommit(ActivityId const & activityId, TransactionMapKey txMapKey)
{
    auto txContext = this->GetDestinationTxContext(activityId, txMapKey);

    if (txContext.get() == nullptr)
    {
        return TransactionSPtr();
    }
    else
    {
        txContext->MarkCommitted();

        AcquireWriteLock lock(deletedKeysLock_);

        for (auto const & updated : txContext->UpdatedKeys)
        {
            deletedKeys_.erase(updated);
        }

        for (auto const & deleted : txContext->DeletedKeys)
        {
            uncommittedDeletes_.erase(deleted);
            deletedKeys_.insert(deleted);
        }

        return txContext->Transaction;
    }
}

void KeyValueStoreMigrator::FinalizeRelease(ActivityId const &, TransactionMapKey txMapKey)
{
    TransactionContextSPtr txContext;
    {
        AcquireWriteLock lock(txMapLock_);

        auto findIt = txMap_.find(txMapKey);
        if (findIt != txMap_.end())
        {
            txContext = findIt->second;

            txMap_.erase(findIt);
        }
    }

    if (txContext.get() != nullptr && !txContext->IsCommitted)
    {
        AcquireWriteLock lock(deletedKeysLock_);

        for (auto const & deleted : txContext->DeletedKeys)
        {
            uncommittedDeletes_.erase(deleted);
        }
    }
}

wstring KeyValueStoreMigrator::GetFullKey(wstring const & type, wstring const & key)
{
    return wformatString("{0}:{1}", type, key);
}

MigrationPhase::Enum KeyValueStoreMigrator::GetNextMigrationPhase(MigrationPhase::Enum phase, MigrationState::Enum state)
{
    switch (state)
    {
    case MigrationState::Failed:
        return (phase == MigrationPhase::Migration || phase == MigrationPhase::TargetDatabaseSwap) ? MigrationPhase::TargetDatabaseCleanup : phase;

    case MigrationState::Completed:
        break;
    
    default:
        return phase;
    }

    //
    // MigrationState::Completed
    //

    switch (phase)
    {
    case MigrationPhase::Migration:
        return MigrationPhase::TargetDatabaseSwap;

    case MigrationPhase::TargetDatabaseSwap:
        return MigrationPhase::SourceDatabaseCleanup;

    case MigrationPhase::TargetDatabaseCleanup:
        return MigrationPhase::Inactive;

    case MigrationPhase::SourceDatabaseCleanup:
        return MigrationPhase::TargetDatabaseActive;

    default:
        return phase;
    }
}

ErrorCode KeyValueStoreMigrator::BackupSourceStore()
{
    SecureString secureConnectionString;
    auto error = this->GetBackupConnectionString(secureConnectionString);
    if (!error.IsSuccess()) { return error; }

    auto const & connectionString = secureConnectionString.GetPlaintext();

    if (StringUtility::StartsWith(connectionString, FilePrefix))
    {
        return this->BackupToFileShare(connectionString);
    }
    else if (StringUtility::StartsWith(connectionString, XStorePrefix))
    {
        return this->BackupToAzureStorage(connectionString);
    }
    else
    {
        return TraceAndGetError(wformatString(L"StoreConnectionString must be prefixed by either {0} or {1}", FilePrefix, XStorePrefix), ErrorCodeValue::NotImplemented);
    }
}

ErrorCode KeyValueStoreMigrator::BackupToFileShare(wstring const & connectionString)
{
    wstring unusedArchiveDest;
    return this->BackupToFileShare(connectionString, unusedArchiveDest);
}

ErrorCode KeyValueStoreMigrator::BackupToFileShare(wstring const & connectionString, __out wstring & archiveDest)
{
    wstring backupDest;
    auto error = this->GetFileShareBackupPaths(connectionString, backupDest, archiveDest);
    if (!error.IsSuccess()) { return error; }

    {
        auto msg = wformatString(GET_STORE_RC( MigrationLocalBackup ), backupDest);
        WriteInfo(TraceComponent, "{0} {1}", this->TraceId, msg);
        sourceReplicatedStore_.SetQueryStatusDetails(msg);
    }

    error = sourceReplicatedStore_.BackupLocal(backupDest);
    if (!error.IsSuccess())
    {
        return TraceAndGetError(wformatString("local backup to '{0}' failed", backupDest), error);
    }

    if (!StringUtility::StartsWith(connectionString, FilePrefix))
    {
        return TraceAndGetError(wformatString(L"StoreConnectionString '{0}' is not a file share", connectionString), ErrorCodeValue::InvalidArgument);
    }

    if (File::Exists(archiveDest))
    {
        error = File::Delete2(archiveDest);
        if (!error.IsSuccess())
        {
            return TraceAndGetError(wformatString("Failed to delete '{0}'", archiveDest), error);
        }
    }

    auto archivePath = Path::GetDirectoryName(archiveDest);
    if (!Directory::Exists(archivePath))
    {
        error = Directory::Create2(archivePath);
        if (!error.IsSuccess())
        {
            return TraceAndGetError(wformatString("Failed to create directory '{0}'", archivePath), error);
        }
    }

    {
        auto msg = wformatString(GET_STORE_RC( MigrationArchiveBackup ), archiveDest);
        WriteInfo(TraceComponent, "{0} {1}", this->TraceId, msg);
        sourceReplicatedStore_.SetQueryStatusDetails(msg);
    }

    error = Directory::CreateArchive(backupDest, archiveDest);

    if (error.IsSuccess())
    {
        error = Directory::Delete(backupDest, true); // recursive

        if (!error.IsSuccess())
        {
            return TraceAndGetError(wformatString("Failed to cleanup backup directory '{0}'", backupDest), error);
        }
    }

    if (error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} archived backup to '{1}'", this->TraceId, archiveDest);
    }
    else
    {
        return TraceAndGetError(wformatString("Failed to archive to '{0}' ({1})", archiveDest, archivePath), error);
    }

    return error;
}

ErrorCode KeyValueStoreMigrator::BackupToAzureStorage(wstring const & connectionString)
{
    wstring archiveDest;

    auto mockConnectionString = wformatString("{0}{1}", FilePrefix, tstoreSettings_->WorkingDirectory);
    auto error = this->BackupToFileShare(mockConnectionString, archiveDest);
    if (!error.IsSuccess()) { return error; }

    auto azClient = AzureStorageClient::Create(this->PartitionedReplicaId, connectionString.substr(XStorePrefix.size()));

    auto const & srcZip = archiveDest;
    auto destZip = wformatString("{0}.zip", this->PartitionId);
    auto container = this->GetBackupRootDirectory();

    auto msg = wformatString(GET_STORE_RC( MigrationArchiveUpload ), Path::Combine(container, destZip));
    WriteInfo(TraceComponent, "{0} {1}", this->TraceId, msg);
    sourceReplicatedStore_.SetQueryStatusDetails(msg);

    error = azClient->Upload(srcZip, destZip, container);
    if (!error.IsSuccess()) 
    { 
        return TraceAndGetError(wformatString("Failed to upload '{0}' -> '{1}' (container={2})", srcZip, destZip, container), error);
    }

    error = File::Delete2(srcZip);
    if (!error.IsSuccess()) 
    { 
        return TraceAndGetError(wformatString("Failed to delete '{0}'", srcZip), error);
    }

    return error; 
}

bool KeyValueStoreMigrator::TryRestoreSourceStore()
{
    auto state = this->GetMigrationState();

    if (state == MigrationState::Processing)
    {
        this->FailMigration(GET_STORE_RC( MigrationRestoreInterruption ));

        return false;
    }
    else
    {
        phase_ = MigrationPhase::RestoreSourceBackup;

        this->SetMigrationState(MigrationState::Processing);

        auto error = this->RestoreSourceStore();

        if (!error.IsSuccess()) { this->OnMigrationFailure(error); }

        return true;
    }
}

ErrorCode KeyValueStoreMigrator::RestoreSourceStore()
{
    SecureString secureConnectionString;
    auto error = this->GetBackupConnectionString(secureConnectionString);
    if (!error.IsSuccess()) { return error; }

    auto const & connectionString = secureConnectionString.GetPlaintext();

    if (StringUtility::StartsWith(connectionString, FilePrefix))
    {
        return this->RestoreFromFileShare(connectionString);
    }
    else if (StringUtility::StartsWith(connectionString, XStorePrefix))
    {
        return this->RestoreFromAzureStorage(connectionString);
    }
    else
    {
        return TraceAndGetError(wformatString(L"StoreConnectionString must be prefixed by either {0} or {1}", FilePrefix, XStorePrefix), ErrorCodeValue::NotImplemented);
    }

}

ErrorCode KeyValueStoreMigrator::RestoreFromFileShare(wstring const & connectionString)
{
    wstring restoreSrc;
    wstring archiveSrc;

    auto error = this->GetFileShareBackupPaths(connectionString, restoreSrc, archiveSrc);
    if (!error.IsSuccess()) { return error; }

    return this->RestoreFromFileShare(archiveSrc, restoreSrc);
}

ErrorCode KeyValueStoreMigrator::RestoreFromFileShare(wstring const & archiveSrc, wstring const & restoreSrc)
{
    if (!File::Exists(archiveSrc))
    {
        return TraceAndGetError(wformatString("Archive not found at '{0}'", archiveSrc), ErrorCodeValue::NotFound);
    }

    if (Directory::Exists(restoreSrc))
    {
        auto error = Directory::Delete(restoreSrc, true); // recursive
        if (!error.IsSuccess())
        {
            return TraceAndGetError(wformatString("Failed to delete directory '{0}'", restoreSrc), error);
        }
    }
    else
    {
        auto error = Directory::Create2(restoreSrc);
        if (!error.IsSuccess())
        {
            return TraceAndGetError(wformatString("Failed to create directory '{0}'", restoreSrc), error);
        }
    }

    auto error = Directory::ExtractArchive(archiveSrc, restoreSrc);
    if (error.IsSuccess())
    {
        WriteInfo(TraceComponent, "{0} extracted archive to '{1}'", this->TraceId, restoreSrc);
    }
    else
    {
        return TraceAndGetError(wformatString("Failed to extract archive to '{0}'", restoreSrc), error);
    }

    WriteInfo(TraceComponent, "{0} restoring source store from '{1}'", this->TraceId, restoreSrc);

    error = sourceReplicatedStore_.RestoreLocal(restoreSrc);
    if (!error.IsSuccess())
    {
        return TraceAndGetError(wformatString("local restore from '{0}' failed", restoreSrc), error);
    }

    //
    // Replica will transient fault itself to complete the restore
    //

    return error;
}

ErrorCode KeyValueStoreMigrator::RestoreFromAzureStorage(wstring const & connectionString)
{
    wstring restoreSrc;
    wstring archiveSrc;

    auto mockConnectionString = wformatString("{0}{1}", FilePrefix, tstoreSettings_->WorkingDirectory);
    auto error = this->GetFileShareBackupPaths(mockConnectionString, restoreSrc, archiveSrc);
    if (!error.IsSuccess()) { return error; }

    auto srcZip = wformatString("{0}.zip", this->PartitionId);
    auto const & destZip = archiveSrc;
    auto container = this->GetBackupRootDirectory();

    if (File::Exists(destZip))
    {
        error = File::Delete2(destZip);
        if (!error.IsSuccess())
        {
            return TraceAndGetError(wformatString("Failed to delete '{0}'", destZip), error);
        }
    }

    auto destPath = Path::GetDirectoryName(destZip);
    if (!Directory::Exists(destPath))
    {
        error = Directory::Create2(destPath);
        if (!error.IsSuccess())
        {
            return TraceAndGetError(wformatString("Failed to create directory '{0}'", destPath), error);
        }
    }

    auto azClient = AzureStorageClient::Create(this->PartitionedReplicaId, connectionString.substr(XStorePrefix.size()));

    error = azClient->Download(srcZip, destZip, container);
    if (!error.IsSuccess()) 
    { 
        return TraceAndGetError(wformatString("Failed to download '{0}' -> '{1}' (container={2})", srcZip, destZip, container), error);
    }

    error = this->RestoreFromFileShare(archiveSrc, restoreSrc);
    if (!error.IsSuccess()) { return error; }

    error = File::Delete2(archiveSrc);
    if (!error.IsSuccess()) 
    { 
        return TraceAndGetError(wformatString("Failed to delete '{0}'", archiveSrc), error);
    }

    return error; 
}

ErrorCode KeyValueStoreMigrator::GetBackupConnectionString(__out SecureString & secureConnectionString)
{
    auto const & config = StoreConfig::GetConfig().FabricGetConfigStore();

    vector<wstring> sectionNames;
    config.GetSections(sectionNames);

    bool isEncrypted = false;

    auto rawConnectionString = this->Settings.BackupConnectionString;
        
    if (rawConnectionString.empty())
    {
        rawConnectionString = this->ReadBackupConnectionString(ReplicatedStoreSection, MigrationBackupConnectionString, isEncrypted);
    }

    if (rawConnectionString.empty())
    {
        rawConnectionString = this->ReadBackupConnectionString(AzureBlobWinFabEtw, StoreConnectionString, isEncrypted);
    }

    if (rawConnectionString.empty())
    {
        rawConnectionString = this->ReadBackupConnectionString(AzureBlobWinFabEtwInMemory, StoreConnectionString, isEncrypted);
    }

    if (rawConnectionString.empty())
    {
        rawConnectionString = this->ReadBackupConnectionString(FileShareWinFabEtw, StoreConnectionString, isEncrypted);
    }

    if (rawConnectionString.empty())
    {
        return TraceAndGetError(L"Failed to read backup StoreConnectionString from configuration", ErrorCodeValue::NotFound);
    }

    if (isEncrypted)
    {
        auto error = CryptoUtility::DecryptText(rawConnectionString, this->Settings.CertStoreLocation, secureConnectionString);
        if (!error.IsSuccess())
        {
            return TraceAndGetError(L"Failed to decrypt StoreConnectionString", error);
        }
    }
    else
    {
        secureConnectionString = SecureString(move(rawConnectionString));
    }

    return ErrorCodeValue::Success;
}

wstring KeyValueStoreMigrator::ReadBackupConnectionString(
    WStringLiteral const & targetSection, 
    WStringLiteral const & targetKey, 
    __out bool & isEncrypted)
{
    isEncrypted = false;

    auto const & config = StoreConfig::GetConfig().FabricGetConfigStore();

    vector<wstring> sectionNames;
    config.GetSections(sectionNames);

    for (auto const & section : sectionNames)
    {
        if (section != targetSection) { continue; }

        vector<wstring> keyNames;
        config.GetKeys(section, keyNames);

        for (auto const & key : keyNames)
        {
            if (key != targetKey) { continue; }

            return config.ReadString(section, key, isEncrypted);
        }

        break;
    }

    return L"";
}

ErrorCode KeyValueStoreMigrator::GetFileShareBackupPaths(
    wstring const & connectionString,
    __out wstring & backupDest,
    __out wstring & archiveDest)
{
    auto backupDirectoryName = Path::Combine(this->GetBackupRootDirectory(), this->PartitionId.ToString());
    backupDest = Path::Combine(tstoreSettings_->WorkingDirectory, backupDirectoryName);

    if (!StringUtility::StartsWith(connectionString, FilePrefix))
    {
        return TraceAndGetError(wformatString(L"GetFileShareBackupPaths: connection string must start with '{0}'", FilePrefix), ErrorCodeValue::NotImplemented);
    }

    archiveDest = Path::Combine(connectionString.substr(FilePrefix.size()), backupDirectoryName).append(L".zip");

    return ErrorCodeValue::Success;
}

wstring KeyValueStoreMigrator::GetBackupRootDirectory()
{
    auto const & overrideSetting = this->Settings.BackupDirectory;
    return (overrideSetting.empty() ? StoreConfig::GetConfig().MigrationBackupDirectory : overrideSetting);
}

ErrorCode KeyValueStoreMigrator::TraceAndGetError(wstring && msg, ErrorCode const & error)
{
    if (!error.Message.empty())
    {
        msg.append(L"; ");
        msg.append(error.Message);
    }

    WriteWarning(TraceComponent, "{0} {1}: {2}", this->TraceId, msg, error);

    return ErrorCode(error.ReadValue(), move(msg));
}
