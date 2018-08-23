// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Naming/MockOperationContext.h"
#include "Naming/MockReplicator.h"
#include "Naming/MockStatefulServicePartition.h"

using namespace Api;
using namespace Common;
using namespace Reliability::ReplicationComponent;
using namespace ServiceModel;
using namespace std;
using namespace TestHooks;

namespace Store
{
    StringLiteral const TraceComponent("LifeCycle");

    //
    // *** Repair async operations
    //
    
    class ReplicatedStore::OpenRepairAsyncOperation : public AsyncOperation
    {
    public:
        OpenRepairAsyncOperation(
            __in ReplicatedStore & owner,
            ErrorCode const & error,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent)
            , owner_(owner)
            , openError_(error)
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & operation)
        {
            return AsyncOperation::End<OpenRepairAsyncOperation>(operation)->Error;
        }

        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            this->GetRepairAction(thisSPtr);
        }

    private:

        void GetRepairAction(AsyncOperationSPtr const & thisSPtr)
        {
            if (!StoreConfig::GetConfig().EnableRepairPolicy)
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} repair policy disabled via cluster config: error={1}",
                    owner_.TraceId,
                    openError_);

                this->TryComplete(thisSPtr, openError_);

                return;
            }

            auto repairCondition = GetRepairCondition(openError_);

            if (repairCondition == RepairCondition::None)
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} unsupported error for repair on open: error={1}",
                    owner_.TraceId,
                    openError_);

                this->TryComplete(thisSPtr, openError_);

                return;
            }

            if (!owner_.repairPolicySPtr_)
            {
                owner_.WriteInfo(
                    TraceComponent,
                    "{0} repair policy not set on open: error={1}",
                    owner_.TraceId,
                    openError_);

                this->TryComplete(thisSPtr, openError_);

                return;
            }

            owner_.WriteInfo(
                TraceComponent,
                "{0} applying repair policy on open: error={1}",
                owner_.TraceId,
                openError_);

            RepairDescription repairDescription(
                owner_.ReplicaRole,
                repairCondition);

            auto operation = owner_.repairPolicySPtr_->BeginGetRepairActionOnOpen(
                repairDescription,
                StoreConfig::GetConfig().RepairPolicyTimeout,
                [this](AsyncOperationSPtr const & operation){ this->OnGetRepairActionComplete(operation, false); },
                thisSPtr);
            this->OnGetRepairActionComplete(operation, true);
        }

        RepairCondition::Enum GetRepairCondition(ErrorCode const & error)
        {
            switch (error.ReadValue())
            {
            case ErrorCodeValue::DatabaseFilesCorrupted:
                return RepairCondition::CorruptDatabaseLogs;

            case ErrorCodeValue::StoreInUse:
                return RepairCondition::DatabaseFilesAccessDenied;

            default:
                return RepairCondition::None;
            }
        }

        void OnGetRepairActionComplete(
            AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

            auto const & thisSPtr = operation->Parent;

            RepairAction::Enum repairAction;

            auto error = owner_.repairPolicySPtr_->EndGetRepairActionOnOpen(operation, repairAction);

            if (!error.IsSuccess())
            {
                owner_.WriteWarning(
                    TraceComponent,
                    "{0} failed to get repair action: {1}",
                    owner_.TraceId,
                    error);

                this->TryComplete(thisSPtr, error);

                return;
            }

            switch (repairAction)
            {
            case RepairAction::DropDatabase:
                this->DropDatabase(thisSPtr);
                break;

            case RepairAction::TerminateProcess:
                this->TerminateProcess(thisSPtr);
                break;

            default:
                this->TryComplete(thisSPtr, openError_);
                break;
            }
        }

        void DropDatabase(AsyncOperationSPtr const & thisSPtr)
        {
            owner_.ReleaseLocalStore();

            auto error = owner_.BackupAndDeleteDatabase();

            if (error.IsSuccess())
            {
                // This will fail open and RA will retry with FABRIC_REPLICA_OPEN_MODE_EXISTING.
                // That will mismatch the current state since the database is now gone, so RA
                // will continue retrying open until the permanent fault is processed.
                //
                error = ErrorCodeValue::DatabaseFilesCorrupted;
            }

            owner_.expectingPermanentFault_ = true;

            // Report fault to actually drop this replica and change the replica ID
            // since that is what we've effectively done internally.
            //
            // We can do this safely even if BackupAndDeleteDatabase() fails since
            // we've already determined that the drop database action is safe.
            //
            owner_.ReportPermanentFault(error);

            this->TryComplete(thisSPtr, error);
        }

        void TerminateProcess(AsyncOperationSPtr const & thisSPtr)
        {
            if (StoreConfig::GetConfig().AssertOnOpenSharingViolation)
            {
                Assert::CodingError(
                    "{0} configuration ReplicatedStore/AssertOnOpenSharingViolation=true: terminating process for self-repair",
                    owner_.TraceId);
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0} configuration ReplicatedStore/AssertOnOpenSharingViolation=false: skip terminating process for self-repair",
                    owner_.TraceId);

                this->TryComplete(thisSPtr, openError_);
            }
        }

        ReplicatedStore & owner_;
        ErrorCode openError_;
    };

    //
    // *** ReplicatedStore
    //

    ReplicatedStore::ReplicatedStore(
        Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        ReplicatorSettingsUPtr && replicatorSettings,
        ReplicatedStoreSettings const & settings,
        Api::IStoreEventHandlerPtr const & storeEventHandler,
        Api::ISecondaryEventHandlerPtr const& secondaryEventHandler,
        ComponentRoot const & root)
        : RootedObject(root) 
        , PartitionedReplicaTraceComponent(Store::PartitionedReplicaId(partitionId, replicaId))
    {
        this->InitializeCtor(settings, storeEventHandler, secondaryEventHandler);

        replicatorSettings_ = move(replicatorSettings);
    }
    
    ReplicatedStore::ReplicatedStore(
        Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        ComPointer<IFabricReplicator> && replicator,
        ReplicatedStoreSettings const & settings,
        Api::IStoreEventHandlerPtr const & storeEventHandler,
        Api::ISecondaryEventHandlerPtr const& secondaryEventHandler,
        ComponentRoot const & root)
        : RootedObject(root) 
        , PartitionedReplicaTraceComponent(Store::PartitionedReplicaId(partitionId, replicaId))
    {
        this->InitializeCtor(settings, storeEventHandler, secondaryEventHandler);

        replicatorCPtr_ = move(replicator);
    }
        
    void ReplicatedStore::InitializeCtor(
        ReplicatedStoreSettings const & settings,
        Api::IStoreEventHandlerPtr const & storeEventHandler,
        Api::ISecondaryEventHandlerPtr const& secondaryEventHandler)
    {
        //
        // Common member simple initialization
        //

        settings_ = settings;
        storeEventHandler_ = storeEventHandler;
        secondaryEventHandler_ = secondaryEventHandler;

        test_localApplyDelay_ = TimeSpan::Zero;

        isLocalStoreTerminated_.store(false);
        replicaRole_ = ::FABRIC_REPLICA_ROLE_UNKNOWN;
        allowIncrementalBackup_.store(false);

        tombstoneCount_.store(0);
        tombstoneCleanupActive_.store(false);
        lowWatermarkReaderCount_ = 0;

        targetCopyOperationSize_ = StoreConfig::GetConfig().TargetCopyOperationSize;

        isRoleSet_ = false;
        isActive_ = false;
        expectingPermanentFault_ = false;

        fatalError_ = ErrorCodeValue::Success;

        pendingTransactions_ = 0;

        test_SecondaryPumpEnabled_ = true;
        test_SlowCommitEnabled_ = StoreConfig::GetConfig().EnableSlowCommitTest;
        test_SecondaryApplyFaultInjectionEnabled_ = false;
        test_TombstoneCleanupEnabled_ = true;
        test_IsLogTruncationTestMode_ = false;
        test_LogTruncationTimerFireCount_ = 0;

        enableFabricTimer_ = (StoreConfig::GetConfig().FabricTimePersistInterval == TimeSpan::Zero ? false : true);
        test_StartFabricTimerWithCancelWait_ = false;

        isDatalossCallbackActive_.store(false);

        isReadOnlyForMigration_ = false;

        //
        // Common member object creation
        //

        WriteInfo(TraceComponent, "{0} ctor: EnableFlushOnDrain={1}", this->TraceId, settings_.EnableFlushOnDrain);  

        auto fabricTimePersistInterval = StoreConfig::GetConfig().FabricTimePersistInterval;
        auto maxAsyncCommitDelay = StoreConfig::GetConfig().MaxAsyncCommitDelayInMilliseconds;

        if (enableFabricTimer_ && fabricTimePersistInterval.TotalPositiveMilliseconds() <= maxAsyncCommitDelay)
        {
            WriteWarning(
                TraceComponent,
                "{0}: FabricTimePersistInterval ({1}) <= MaxAsyncCommitDelayInMilliseconds ({2}): Fabric timer may be inaccurate",
                this->TraceId,
                fabricTimePersistInterval,
                maxAsyncCommitDelay);
        }

        auto tempStateMachineUPtr = make_unique<class ReplicatedStore::StateMachine>(this->PartitionedReplicaId);

        auto tempNotificationManagerUPtr = make_unique<class ReplicatedStore::NotificationManager>(
            *this,
            secondaryEventHandler_,
            settings_.NotificationMode);

        auto tempFileStreamFullCopyManagerSPtr = make_shared<FileStreamFullCopyManager>(*this);

        auto tempPerfCounters = ReplicatedStorePerformanceCounters::CreateInstance(wformatString("{0}:{1}",
            this->TraceId,
            SequenceNumber::GetNext()));

        cachedCurrentEpoch_.DataLossNumber = 0;
        cachedCurrentEpoch_.ConfigurationNumber = 0;
        cachedCurrentEpoch_.Reserved = NULL;
        
        localStoreInstanceName_ = this->TraceId;

        stateMachineUPtr_.swap(tempStateMachineUPtr);

        notificationManagerUPtr_.swap(tempNotificationManagerUPtr);

        fileStreamFullCopyManagerSPtr_.swap(tempFileStreamFullCopyManagerSPtr);

        perfCounters_.swap(tempPerfCounters);
    }

    ReplicatedStore::~ReplicatedStore()
    {
        WriteNoise(
            TraceComponent, 
            "{0} ReplicatedStore::~dtor",
            this->TraceId);
    }

    // *******************
    // IStoreBase methods
    // *******************

    ::FABRIC_TRANSACTION_ISOLATION_LEVEL ReplicatedStore::GetDefaultIsolationLevel()
    {
        AcquireReadLock lock(dropLocalStoreLock_);

        if (this->IsLocalStoreClosed)
        {
            // KeyValueStoreReplica::CreateTransaction() calls GetDefaultIsolationLevel() first
            // before actually calling ReplicatedStore::CreateTransaction().
            //
            // Subsequent call to ReplicatedStore::CreateTransaction() will fail when the store
            // is closed, so just return a meaningless isolation level.
            //
            return FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT;
        }

        return this->LocalStore->GetDefaultIsolationLevel();
    }

    ErrorCode ReplicatedStore::SetDefaultIsolationLevel(::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel)
    {
        AcquireReadLock lock(dropLocalStoreLock_);

        if (this->IsLocalStoreClosed)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        return this->LocalStore->SetDefaultIsolationLevel(isolationLevel);
    }

    ErrorCode ReplicatedStore::CreateTransaction(
        __out TransactionSPtr & transactionSPtr)
    {
        return this->CreateTransaction(ActivityId(), transactionSPtr);
    }

    ErrorCode ReplicatedStore::CreateTransaction(
        ActivityId const & activityId,
        __out TransactionSPtr & transactionSPtr)
    {
        return this->CreateTransaction(activityId, TransactionSettings(), transactionSPtr);
    }

    ErrorCode ReplicatedStore::CreateTransaction(
        ActivityId const & activityId,
        TransactionSettings const & settings,
        __out TransactionSPtr & transactionSPtr)
    {
        transactionSPtr.reset();

        ReplicatedStoreState::Enum state;
        auto error = stateMachineUPtr_->StartTransactionEvent(state);

        if (error.IsSuccess())
        {
            ASSERT_IFNOT(
                state == Store::ReplicatedStoreState::PrimaryActive, 
                "{0} StateMachine::StartTransactionEvent returned invalid state = {1}", 
                this->TraceId, 
                state);

            TransactionSPtr innerTxSPtr;
            error = this->LocalStore->CreateTransaction(innerTxSPtr);

            if (error.IsSuccess())
            {
                auto baseSPtr = make_shared<Transaction>(
                    *this,
                    this->TryGetTxReplicator(),
                    innerTxSPtr,
                    txEventHandler_,
                    activityId,
                    settings);

                if (txTrackerUPtr_->TryAdd(baseSPtr))
                {
                    transactionSPtr = move(baseSPtr);
                }
                else
                {
                    // dtor will result in FinishTransaction()
                    //
                    baseSPtr.reset();
                    error = ErrorCodeValue::NotPrimary;
                }
            }
            else
            {
                this->FinishTransaction();

                if (error.IsError(ErrorCodeValue::StoreFatalError))
                {
                    this->ReportTransientFault(error);
                }
            }
        }

        return error;
    }

    void ReplicatedStore::SimpleTransactionGroupTimerCallback()
    {
        if (settings_.TransactionHighWatermark >= 0 && pendingTransactions_ >= settings_.TransactionHighWatermark)
        {
            WriteInfo(
                TraceComponent, 
                "{0} ReplicatedStore::SimpleTransactionGroupTimerCallback. Pending completion transactions {1}.  Batching period extended by {2} ms",
                this->TraceId,
                pendingTransactions_,
                settings_.CommitBatchingPeriodExtension);

            simpleTransactionGroupTimer_->Change(TimeSpan::FromMilliseconds(settings_.CommitBatchingPeriodExtension));
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} ReplicatedStore::SimpleTransactionGroupTimerCallback",
                this->TraceId);

            this->InnerCloseCurrentSimpleTransactionGroup();
        }
    }

    void ReplicatedStore::OnRollbackSimpleTransactionGroup(SimpleTransactionGroup * group)
    {
        AcquireExclusiveLock grab(transactionGroupLock_);

        if (group == simpleTransactionGroupSPtr_.get())
        {
            WriteInfo(
                TraceComponent, 
                "{0} ReplicatedStore::OnRollbackSimpleTransactionGroup: {1}",
                this->TraceId,
                simpleTransactionGroupSPtr_->TraceId);
            simpleTransactionGroupTimer_->Change(TimeSpan::MaxValue);
            simpleTransactionGroupSPtr_.reset();
        }
    }

    void ReplicatedStore::CloseCurrentSimpleTransactionGroup()
    {
        if (settings_.CommitBatchingPeriod > 0)
        {
            this->InnerCloseCurrentSimpleTransactionGroup();
        }
    }

    void ReplicatedStore::InnerCloseCurrentSimpleTransactionGroup()
    {
        SimpleTransactionGroupSPtr snap;
        {
            AcquireExclusiveLock grab(transactionGroupLock_);

            if (simpleTransactionGroupSPtr_)
            {
                simpleTransactionGroupTimer_->Change(TimeSpan::MaxValue);
                snap = move(simpleTransactionGroupSPtr_);
            }
        }

        if (snap)
        {
            snap->Close();
        }
    }

    ErrorCode ReplicatedStore::CreateSimpleTransaction(
        __out IStoreBase::TransactionSPtr & transactionSPtr)
    {
        return this->CreateSimpleTransaction(ActivityId(Guid::NewGuid()), transactionSPtr);
    }

    ErrorCode ReplicatedStore::CreateSimpleTransaction(
        ActivityId const & activityId,
        __out IStoreBase::TransactionSPtr & txSPtr)
    {
        if (settings_.CommitBatchingPeriod <= 0)
        {
            return CreateTransaction(activityId, txSPtr);
        }

        txSPtr.reset();

        ReplicatedStoreState::Enum state;
        auto error = stateMachineUPtr_->StartTransactionEvent(state);

        if (error.IsSuccess())
        {
            ASSERT_IFNOT(
                state == Store::ReplicatedStoreState::PrimaryActive, 
                "{0} StateMachine::StartTransactionEvent returned invalid state = {1}", 
                this->TraceId, 
                state);

            TransactionBaseSPtr baseSPtr;
            error = this->AttachOrCreateSimpleTransactionGroup(activityId, baseSPtr);

            if (error.IsSuccess())
            {
                if (txTrackerUPtr_->TryAdd(baseSPtr))
                {
                    txSPtr = move(baseSPtr);
                }
                else
                {
                    // dtor will result in FinishTransaction()
                    //
                    baseSPtr.reset();
                    error = ErrorCodeValue::NotPrimary;
                }
            }
            else
            {
                this->FinishTransaction();

                if (error.IsError(ErrorCodeValue::StoreFatalError))
                {
                    this->ReportTransientFault(error);
                }
            }
        }

        return error;
    }

    ErrorCode ReplicatedStore::AttachOrCreateSimpleTransactionGroup(
        ActivityId const & activityId,
        __out TransactionBaseSPtr & simpleTxSPtr)
    {
        simpleTxSPtr.reset();
        auto now = DateTime::Now();
        SimpleTransactionGroupSPtr groupToCloseSPtr;
        {
            AcquireExclusiveLock grab(transactionGroupLock_);

            lastSimpleTransactionTimestamp_ = now;

            if (pendingTransactions_ > settings_.TransactionLowWatermark)
            {
                if (simpleTransactionGroupSPtr_)
                {
                    simpleTxSPtr = simpleTransactionGroupSPtr_->CreateSimpleTransaction(activityId);
                }

                // If there was no existing group or the existing group has already
                // been closed, then start a new transaction group.
                //
                if (!simpleTxSPtr)
                {
                    TransactionSPtr innerTxSPtr;
                    auto error = this->LocalStore->CreateTransaction(innerTxSPtr);

                    if (!error.IsSuccess())
                    {
                        return error;
                    }

                    groupToCloseSPtr = move(simpleTransactionGroupSPtr_);

                    simpleTransactionGroupSPtr_ = make_shared<SimpleTransactionGroup>(
                        *this,
                        this->TryGetTxReplicator(),
                        move(innerTxSPtr),
                        txEventHandler_,
                        activityId,
                        settings_.CommitBatchingSizeLimit);
                    simpleTransactionGroupTimer_->Change(TimeSpan::FromMilliseconds(settings_.CommitBatchingPeriod));

                    simpleTxSPtr = simpleTransactionGroupSPtr_->CreateSimpleTransaction(activityId);
                }
            }
        }

        // Fallback to a normal transaction in all cases where a simple transaction
        // cannot be created.
        //
        if (!simpleTxSPtr)
        {
            TransactionSPtr innerTxSPtr;
            auto error = this->LocalStore->CreateTransaction(innerTxSPtr);

            if (!error.IsSuccess())
            {
                return error;
            }

            simpleTxSPtr = make_shared<Transaction>(
                *this, 
                this->TryGetTxReplicator(),
                innerTxSPtr, 
                txEventHandler_,
                activityId);
        }

        if (groupToCloseSPtr)
        {
            groupToCloseSPtr->Close();
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::CreateEnumerationByTypeAndKey(
        __in IStoreBase::TransactionSPtr const & txSPtr,
        __in std::wstring const & type,
        __in std::wstring const & keyStart,
        __out IStoreBase::EnumerationSPtr & enumerationSPtr)
    {
        auto error = this->CheckReadAccess(txSPtr, type);

        if (!error.IsSuccess()) { return error; }

        return InternalCreateEnumerationByTypeAndKey(txSPtr, type, keyStart, enumerationSPtr);
    }

    ErrorCode ReplicatedStore::InternalCreateEnumerationByTypeAndKey(
        __in IStoreBase::TransactionSPtr const & txSPtr,
        __in std::wstring const & type,
        __in std::wstring const & keyStart,
        __out IStoreBase::EnumerationSPtr & enumerationSPtr)
    {
        ErrorCode error;

        auto castedTx = CastTransactionSPtr<ReplicatedStore::TransactionBase>(txSPtr);

        if (!castedTx)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}: failed to cast transaction", this->TraceId);

            error = ErrorCodeValue::OperationFailed;
        }

        if (error.IsSuccess() && !castedTx->IsEnumerationSupported)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}: enumeration not supported", this->TraceId); 

            error = ErrorCodeValue::InvalidOperation;
        }
        
        if (error.IsSuccess())
        {
            TransactionSPtr innerTxSPtr;
            error = castedTx->TryGetInnerTransaction(innerTxSPtr);
            
            if (error.IsSuccess())
            {
                EnumerationSPtr innerStoreEnumSPtr;
                error = this->LocalStore->CreateEnumerationByTypeAndKey(innerTxSPtr, type, keyStart, innerStoreEnumSPtr);

                if (error.IsSuccess())
                {
                    enumerationSPtr = make_shared<ReplicatedStore::Enumeration>(
                        castedTx, 
                        std::move(innerStoreEnumSPtr), 
                        error.ReadValue(),
                        settings_.EnumerationPerfTraceThreshold);
                }
            }
        }

        WriteNoise(
            TraceComponent, 
            "{0} CreateEnumeration: {1}", 
            castedTx->TraceId,
            error);

        if (!error.IsSuccess())
        {
            txSPtr->Abort();
        }

        return error;
    }

    ErrorCode ReplicatedStore::InternalReadExact(
        __in TransactionSPtr const & tx,
        __in std::wstring const & type,
        __in std::wstring const & key,
        __out std::vector<byte> & value,
        __out __int64 & operationLsn)
    {
        EnumerationSPtr enumSPtr;
        auto error = this->InternalCreateEnumerationByTypeAndKey(tx, type, key, enumSPtr);
        if (!error.IsSuccess()) { return error; }

        error = enumSPtr->MoveNext();
        if (error.IsError(Common::ErrorCodeValue::EnumerationCompleted)) { return Common::ErrorCodeValue::NotFound; }
        if (!error.IsSuccess()) { return error; }

        std::wstring currentType;
        error = enumSPtr->CurrentType(currentType);
        if (!error.IsSuccess()) { return error; }
        
        std::wstring currentKey;
        error = enumSPtr->CurrentKey(currentKey);
        if (!error.IsSuccess()) { return error; }

        if (currentType == type && currentKey == key)
        {
            error = enumSPtr->CurrentValue(value);
            if (!error.IsSuccess()) { return error; }

            error = enumSPtr->CurrentOperationLSN(operationLsn);
            if (!error.IsSuccess()) { return error; }
        }
        else
        {
            error = Common::ErrorCodeValue::NotFound;
        }

        return error;
    }

    ErrorCode ReplicatedStore::Insert(
        IStoreBase::TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        __in void const * value,
        size_t valueSizeInBytes)
    {
        auto error = this->CheckWriteAccess(txSPtr, type);

        if (!error.IsSuccess()) { return error; }

        return InternalInsert(txSPtr,type,key,value,valueSizeInBytes);
    }
    
    ErrorCode ReplicatedStore::InternalInsert(
        IStoreBase::TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        __in void const * value,
        size_t valueSizeInBytes)
    {
        ReplicatedStore::TransactionBase & castedTx = CastTransaction<ReplicatedStore::TransactionBase>(txSPtr);

        ErrorCode error(ErrorCodeValue::Success);

        castedTx.AcquireLock();

        // We do not allow inserting an entry that was previously deleted in 
        // the same transaction. The secondary is unable to correctly order the 
        // insert operation with respect to the delete operation during copy
        // if the LSNs are the same. This can result in unexpected dataloss if
        // the secondary applies the delete after the insert.
        //
        // Update will fail if the entry does not exist, so this verification
        // is not needed for updates.
        //
        if (castedTx.ContainsDelete(type, key))
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to Write({1}, {2}): previous Delete operation exists",
                castedTx.TraceId,
                type,
                key);

            error = ErrorCodeValue::StoreWriteConflict;
        }

		FILETIME currStoreFiletime = this->LocalStore->GetStoreUtcFILETIME();
		
        TransactionSPtr innerTxSPtr;
        if (error.IsSuccess())
        {
            error = castedTx.TryGetInnerTransaction(innerTxSPtr);

            if (error.IsSuccess())
            {
                error = this->LocalStore->Insert(
                    innerTxSPtr,
                    type, 
                    key, 
                    value, 
                    valueSizeInBytes,
                    ILocalStore::OperationNumberUnspecified,
                    &currStoreFiletime);
            }
        }

        if (error.IsSuccess())
        {
            error = castedTx.AddReplicationOperation(ReplicationOperation(
                ReplicationOperationType::Insert,
                type, 
                key, 
                reinterpret_cast<byte const *>(value), 
                valueSizeInBytes,
                currStoreFiletime));
        }

        castedTx.ReleaseLock();
        
        if (error.IsSuccess())
        {
            auto txHandler = txEventHandler_.lock();
            if (txHandler.get() != nullptr)
            {
                error = txHandler->OnInsert(
                    castedTx.ActivityId,
                    castedTx.MigrationTxKey,
                    type,
                    key,
                    value,
                    valueSizeInBytes);
            }
        }

        if (!error.IsSuccess())
        {
            this->OnWriteError(castedTx, error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::Update(
        IStoreBase::TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        _int64 checkSequenceNumber,
        std::wstring const & newKey,
        __in_opt void const * newValue,
        size_t valueSizeInBytes)
    {
        auto error = this->CheckWriteAccess(txSPtr, type);

        if (!error.IsSuccess()) { return error; }

        return InternalUpdate(txSPtr,type,key,checkSequenceNumber,newKey,newValue,valueSizeInBytes);
    }

    ErrorCode ReplicatedStore::InternalUpdate(
        IStoreBase::TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        _int64 checkSequenceNumber,
        std::wstring const & newKey,
        __in_opt void const * newValue,
        size_t valueSizeInBytes)
    {
        ReplicatedStore::TransactionBase & castedTx = CastTransaction<ReplicatedStore::TransactionBase>(txSPtr);

        castedTx.AcquireLock();
        
        TransactionSPtr innerTxSPtr;
        auto error = castedTx.TryGetInnerTransaction(innerTxSPtr);
		
		FILETIME currStoreFiletime = this->LocalStore->GetStoreUtcFILETIME();

        if (error.IsSuccess())
        {
            error = this->LocalStore->Update(
                innerTxSPtr,
                type, 
                key, 
                checkSequenceNumber, 
                newKey, 
                reinterpret_cast<byte const *>(newValue), 
                valueSizeInBytes,
                ILocalStore::OperationNumberUnspecified,
                &currStoreFiletime);
        }

        if (error.IsSuccess())
        {
            error = castedTx.AddReplicationOperation(ReplicationOperation(
                ReplicationOperationType::Update,
                type, 
                key, 
                newKey,
                reinterpret_cast<byte const *>(newValue),
                valueSizeInBytes,
                currStoreFiletime));
        }

        castedTx.ReleaseLock();
        
        if (error.IsSuccess())
        {
            auto txHandler = txEventHandler_.lock();
            if (txHandler.get() != nullptr)
            {
                error = txHandler->OnUpdate(
                    castedTx.ActivityId,
                    castedTx.MigrationTxKey,
                    type,
                    key,
                    newValue,
                    valueSizeInBytes);
            }
        }


        if (!error.IsSuccess())
        {
            this->OnWriteError(castedTx, error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::Delete(
        IStoreBase::TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        _int64 checkSequenceNumber)
    {
        auto error = this->CheckWriteAccess(txSPtr, type);

        if (!error.IsSuccess()) { return error; }

        return InternalDelete(txSPtr,type,key,checkSequenceNumber);
    }

    ErrorCode ReplicatedStore::InternalDelete(
        IStoreBase::TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        _int64 checkSequenceNumber)
    {
        ReplicatedStore::TransactionBase & castedTx = CastTransaction<ReplicatedStore::TransactionBase>(txSPtr);

        ErrorCode error(ErrorCodeValue::Success);

        castedTx.AcquireLock();

        // We do not allow deleting an entry that was inserted/updated earlier in the same
        // transaction since the primary will be unable to update the LSN of the missing
        // entry without special-handling to detect this case. The primary currently
        // faults itself if it fails to update any LSNs.
        //
        if (castedTx.ContainsOperation(type, key))
        {
            WriteInfo(
                TraceComponent, 
                "{0} failed to Delete({1}, {2}): previous operation exists",
                castedTx.TraceId,
                type,
                key);

            error = ErrorCodeValue::StoreWriteConflict;
        }
        
        TransactionSPtr innerTxSPtr;

        if (error.IsSuccess())
        {
            error = castedTx.TryGetInnerTransaction(innerTxSPtr);

            if (error.IsSuccess())
            {
                error = this->LocalStore->Delete(
                    innerTxSPtr,
                    type, 
                    key, 
                    checkSequenceNumber); 
            }
        }

        if (error.IsSuccess() && !settings_.EnableTombstoneCleanup2)
        {
            // Lock the underlying ESE record for this tombstone before
            // replication so that the tombstone prune process cannot 
            // cause a conflict and fail the tombstone sequence number update 
            // that occurs after replication succeeds.
            //
            bool ignoredInserted = false;
            error = ReplicatedStore::UpdateTombstone1(
                innerTxSPtr,
                type,
                key,
                0,
                ignoredInserted);
        }

        if (error.IsSuccess())
        {
            error = castedTx.AddReplicationOperation(ReplicationOperation(
                ReplicationOperationType::Delete,
                type, 
                key)); 
        }

        castedTx.ReleaseLock();

        if (error.IsSuccess())
        {
            auto txHandler = txEventHandler_.lock();
            if (txHandler.get() != nullptr)
            {
                error = txHandler->OnDelete(
                    castedTx.ActivityId,
                    castedTx.MigrationTxKey,
                    type,
                    key);
            }
        }

        if (!error.IsSuccess())
        {
            this->OnWriteError(castedTx, error);
        }

        return error;
    }

    void ReplicatedStore::OnWriteError(__in ReplicatedStore::TransactionBase & castedTx, Common::ErrorCode const & error)
    {
        castedTx.OperationFailed(error);

        if (this->ShouldRestartOnWriteError(error))
        {
            this->ReportTransientFault(error);
        }
    }

    FILETIME ReplicatedStore::GetStoreUtcFILETIME()
    {
        //
        // The value returned from this should be monotonically increasing across causally ordered
        // requests.  In other words, if a single client does request 1 that accesses this value
        // and then after request 1 completes does request 2 that also accesses this value, then
        // request 2 must get a greater or equal value than request 1.
        //
        // Implementing this correctly requires an agreement protocol regarding current UTC time
        // among all nodes that may be primary.  This is not straightforward to accomplish and may
        // require a prohibitively expensive protocol.
        //
        // One option is just to return local clock time anyway.  However, this is currently only
        // used by Federation, which does not use local store, and which could potentially cause
        // data-loss or corruption if it gets the wrong result.
        //
        // So to be safe, this does FailFast to verify that we do not use this store in an unsafe
        // way.
        //
        Assert::CodingError("{0} ReplicatedStore::GetStoreUtcFILETIME not implemented.", this->TraceId);
    }

    bool ReplicatedStore::IsThrottleNeeded() const
    {
        auto txReplicator = this->TryGetTxReplicator();
        return (txReplicator ? txReplicator->IsThrottleNeeded() : false);
    }
    
    // ***************************************
    // Lifecycle-related methods
    // ***************************************

    AsyncOperationSPtr ReplicatedStore::BeginOpen(
        __in FABRIC_REPLICA_OPEN_MODE openMode,
        __in ComPointer<IFabricStatefulServicePartition> const & partition, 
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
    {
        ASSERT_IF(openMode == FABRIC_REPLICA_OPEN_MODE_INVALID, "Invalid openMode at {0}", this->TraceId);
        partitionCPtr_ = partition;

        ErrorCode openError(ErrorCodeValue::Success);

        if (replicatorCPtr_.GetRawPointer() == nullptr)
        {
            IStateProviderPtr stateProvider(this, this->Root.CreateComponentRoot());
            auto stateProviderCPtr = make_com<ComStateProvider>(stateProvider);

            ScopedHeap heap;
            FABRIC_REPLICATOR_SETTINGS publicReplicatorSettings = {0};
            replicatorSettings_->ToPublicApi(heap, publicReplicatorSettings);

            auto hr = partition->CreateReplicator(
                stateProviderCPtr.GetRawPointer(), 
                &publicReplicatorSettings,
                replicatorCPtr_.InitializationAddress(),
                stateReplicatorCPtr_.InitializationAddress());

            auto error = ErrorCode::FromHResult(hr);
            
            if (!error.IsSuccess())
            {
                auto msg = wformatString("CreateReplicator failed: {0} ({1})", error, hr);

                WriteWarning(TraceComponent, "{0} {1}", this->TraceId, msg);

                openError = ErrorCode(error.ReadValue(), move(msg));
            }
        }
        else
        {
            auto hr = replicatorCPtr_->QueryInterface(IID_IFabricStateReplicator, stateReplicatorCPtr_.VoidInitializationAddress());
            auto error = ErrorCode::FromHResult(hr);

            if (!error.IsSuccess())
            {
                auto msg = wformatString("QueryInterface(IFabricStateReplicator) failed: {0} ({1})", error, hr);

                WriteWarning(TraceComponent, "{0} {1}", this->TraceId, msg);

                openError = ErrorCode(error.ReadValue(), move(msg));
            }
        }

        if (openError.IsSuccess())
        {
            auto hr = stateReplicatorCPtr_->QueryInterface(IID_IFabricInternalStateReplicator, internalStateReplicatorCPtr_.VoidInitializationAddress());
            auto error = ErrorCode::FromHResult(hr);

            if (!error.IsSuccess())
            {
                auto msg = wformatString("QueryInterface(IFabricInternalStateReplicator): {0} ({1})", error, hr);

                WriteWarning(TraceComponent, "{0} {1}", this->TraceId, msg);

                openError = ErrorCode(error.ReadValue(), move(msg));
            }
        }

        AsyncOperationSPtr outerOperation;
        if (openError.IsSuccess())
        {
            outerOperation = AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, callback, parent);

            txReplicatorSPtr_ = make_shared<TransactionReplicator>(*this, stateReplicatorCPtr_, internalStateReplicatorCPtr_);
            txReplicatorWPtr_ = txReplicatorSPtr_;
            txTrackerUPtr_ = make_unique<TransactionTracker>(*this);

            bool databaseShouldExist = openMode == FABRIC_REPLICA_OPEN_MODE_EXISTING ? true : false;

            stateMachineUPtr_->PostOpenEvent([this, outerOperation, databaseShouldExist](ErrorCode const & error, ReplicatedStoreState::Enum state)
            {
                this->OpenEventCallback(outerOperation, error, state, databaseShouldExist);
            },
            this->Root.CreateComponentRoot());
        }
        else
        {
            outerOperation = AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, openError, callback, parent);
        }

        return outerOperation;
    }

    ErrorCode ReplicatedStore::EndOpen(
        __in AsyncOperationSPtr const & operation,
        __out ComPointer<IFabricReplicator> & replicator)
    {
        ErrorCode error = OpenAsyncOperation::End(operation);

        if (!error.IsSuccess())
        {
            // KeyValueStoreReplica open will attempt abort, but also abort here
            // in case ReplicatedStore is used directly without the KVS layer
            // (e.g. in unit tests).
            //
            this->Abort();

            replicatorCPtr_.Release();
            stateReplicatorCPtr_.Release();
            txReplicatorSPtr_.reset();
        }
        else
        {
            error = FabricComponent::Open();
            ASSERT_IFNOT(error.IsSuccess(), "FabricComponent::Open is not expected to fail in ReplicatedStore. ErrorCode={0}", error);

            replicator = move(replicatorCPtr_);
        }

        return error;
    }

    AsyncOperationSPtr ReplicatedStore::BeginChangeRole(
        __in ::FABRIC_REPLICA_ROLE newRole,
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
    {
        auto outerOperation = AsyncOperation::CreateAndStart<ChangeRoleAsyncOperation>(newRole, callback, parent);

        if (newRole == ::FABRIC_REPLICA_ROLE_NONE)
        {
            WriteInfo(TraceComponent, "{0} ChangeRole(None)", this->TraceId);

            // Replica is being dropped, just change the role. During close clean up will delete database directory
            outerOperation->TryComplete(outerOperation, ErrorCodeValue::Success);
            return outerOperation;
        }

        ASSERT_IF(expectingPermanentFault_, "{0} permanent fault expected after self-repair", this->TraceId);

        stateMachineUPtr_->PostChangeRoleEvent(newRole, [this, outerOperation, newRole](ErrorCode const & error, ReplicatedStoreState::Enum state)
        {
            this->ChangeRoleEventCallback(outerOperation, error, state, newRole);
        },
        this->Root.CreateComponentRoot());

        return outerOperation;
    }

    ErrorCode ReplicatedStore::EndChangeRole(
        __in AsyncOperationSPtr const & operation,
        __out wstring & serviceLocation)
    {
        ::FABRIC_REPLICA_ROLE newRole;
        ErrorCode error = ChangeRoleAsyncOperation::End(operation, newRole);

        if (error.IsSuccess())
        {
            replicaRole_ = newRole;
            isRoleSet_ = true;
            serviceLocation = this->TraceId;
        }

        return error;
    }

    AsyncOperationSPtr ReplicatedStore::BeginClose(
        __in AsyncCallback const & callback,
        __in AsyncOperationSPtr const & parent)
    {
        auto outerOperation = AsyncOperation::CreateAndStart<CloseAsyncOperation>(callback, parent);

        stateMachineUPtr_->PostCloseEvent([this, outerOperation](ErrorCode const & error, ReplicatedStoreState::Enum state)
        {
            this->CloseEventCallback(outerOperation, error, state);
        },
        this->Root.CreateComponentRoot());

        return outerOperation;
    }

    ErrorCode ReplicatedStore::EndClose(
        __in AsyncOperationSPtr const & operation)
    {
        auto error1 = CloseAsyncOperation::End(operation);

        // Ensure store is terminated even if close fails for whatever reason
        auto error2 = this->TerminateLocalStore();

        if (!error2.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} TerminateLocalStore failed: error={1}", this->TraceId, error2);
        }

        // Best effort - ignore any errors from component close.
        // This is just to put FabricComponent into the expected state.
        //
        FabricComponent::Close();

        return ErrorCode::FirstError(error1, error2);
    }

    void ReplicatedStore::Abort()
    {
        if (!this->FabricComponent::TryAbort())
        {
            // Ensure that a subsequent abort call takes 
            // effect and actually calls OnAbort() if
            // open is cancelled. FabricComponent::Abort() will
            // skip calling OnAbort() unless the component has
            // opened successfully.
            //
            // Alternatively, save the error in EndOpen and
            // call FabricComponent::Open() regardless of
            // success or failure and return the saved error
            // in OnOpen(). Then FabricComponent:Open()
            // will internally Abort(). 
            //
            // Prefer the first option since the latter is 
            // actually more convoluted.
            //
            this->OnAbort();
        }
    }

    // FM uses this to bypass the normal stateful service functions.
    //
    ErrorCode ReplicatedStore::SynchronousClose()
    {
        if (stateMachineUPtr_->IsClosed())
        {
            return ErrorCodeValue::Success;
        }

        ManualResetEvent waiter(false);

        auto operation = this->BeginClose(
            [&waiter](AsyncOperationSPtr const &) { waiter.Set(); },
            this->Root.CreateAsyncOperationRoot());

        waiter.WaitOne();

        return this->EndClose(operation);
    }
    
    //
    // FabricComponent
    //

    ErrorCode ReplicatedStore::OnOpen() 
    {
        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::OnClose() 
    {
        // TODO: In 'develop' branch, FM used to have its own wrapper implementing 
        // IReplicatedStore (ReplicatedStore.cpp/FailoverManagerReplicatedStore)
        // that forwarded all operations to the real replicated store. However,
        // this wrapper did not override FabricComponent::OnClose, which means
        // it never actually closed the underlying store.
        //
        // In 'feature_data' branch, this extra wrapper was removed so that
        // all calls on IReplicatedStore go directly to the real underlying store.
        // However, closing ReplicatedStore waits for replicator to pump a null
        // operation from the replication stream, which only happens when
        // replicator is closed. This will deadlock at the FM during demotion
        // from primary - see SeviceFactory::OnChangeRole, ServiceFactory.cpp.
        //
        // To keep the original (buggy) semantics, don't actually close the 
        // store when FM calls FabricComponent::Close.
        //
        // TSReplicatedStore doesn't have this issue since it doesn't control
        // the replication pump directly. 
        // 
        //return this->SynchronousClose();

        return ErrorCodeValue::Success;
    }

    void ReplicatedStore::OnAbort() 
    {
        WriteInfo(TraceComponent, "{0} aborting", this->TraceId);

        // RA can call Abort and attempt to re-open the same replica immediately, which
        // can cause conflicts if the previous instance hasn't completed closed yet.
        //
        // Block Abort on synchronous close here. Close lifecycle tracking will 
        // self-mitigate against close being stuck forever.
        //
        auto error = this->SynchronousClose();

        WriteTrace(
            error.ToLogLevel(), 
            TraceComponent, 
            "{0} abort completed: error={1}", 
            this->TraceId, 
            error);
    }
        
    //
    // IStateProvider methods
    //
    AsyncOperationSPtr ReplicatedStore::BeginUpdateEpoch( 
        FABRIC_EPOCH const & epoch,
        FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & state)
    {
        return AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(
            *this,
            epoch,
            previousEpochLastSequenceNumber,
            callback, 
            state);
    }

    ErrorCode ReplicatedStore::EndUpdateEpoch(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto error = UpdateEpochAsyncOperation::End(asyncOperation);

        if (error.IsSuccess())
        {
            error = this->Test_GetInjectedFault(FaultInjector::FabricTest_EndUpdateEpochTag);
        }

        return error;
    }

    ErrorCode ReplicatedStore::GetLastCommittedSequenceNumber( 
        __out FABRIC_SEQUENCE_NUMBER & sequenceNumber)
    {
        TransactionSPtr txSPtr;
        auto error = this->CreateLocalStoreTransaction(txSPtr);

        if (error.IsSuccess())
        {
            error = this->InnerGetCurrentProgress(txSPtr, sequenceNumber);
        }

        return error;
    }

    ErrorCode ReplicatedStore::InnerGetCurrentProgress(TransactionSPtr const & txSPtr, __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
    {
        ErrorCode error = this->LocalStore->GetLastChangeOperationLSN(
            txSPtr,
            operationLSN);

        if (error.IsSuccess())
        {
            // Sequence number 1 is used to store all "metadata" (i.e. progress vector data).
            // This metadata does not count towards the progress of user data, so return
            // sequence number 0 if no user data is found. 
            //
            if (operationLSN == Constants::StoreMetadataSequenceNumber)
            {
                bool userDataFound = false;

                EnumerationSPtr enumSPtr;
                error = this->LocalStore->CreateEnumerationByOperationLSN(txSPtr, 0, enumSPtr);

                if (error.IsSuccess())
                {
                    while ((error = enumSPtr->MoveNext()).IsSuccess())
                    {
                        wstring type;
                        if (!(error = enumSPtr->CurrentType(type)).IsSuccess())
                        {
                            return error;
                        }

                        if (!IsStoreMetadataType(type))
                        {
                            WriteNoise(
                                TraceComponent, 
                                "{0} InnerGetCurrentProgress: found user data of type '{1}'",
                                this->TraceId,
                                type);

                            userDataFound = true;
                            break;
                        }
                    }
                }

                if (error.IsError(ErrorCodeValue::EnumerationCompleted))
                {
                    if (!userDataFound)
                    {
                        WriteNoise(
                            TraceComponent, 
                            "{0} InnerGetCurrentProgress: no user data found",
                            this->TraceId);

                        operationLSN = 0;
                    }

                    error = ErrorCodeValue::Success;
                }
            }
        }

        WriteInfo(
            TraceComponent, 
            "{0} InnerGetCurrentProgress: operation LSN = {1} error = {2}",
            this->TraceId,
            operationLSN,
            error);

        return error;
    }

    AsyncOperationSPtr ReplicatedStore::BeginOnDataLoss(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & state)
    {
        WriteInfo(
            TraceComponent, 
            "{0} OnDataLoss()",
            this->TraceId);

        isDatalossCallbackActive_.store(true);

        return AsyncOperation::CreateAndStart<OnDataLossAsyncOperation>(
            this->storeEventHandler_,
            callback, 
            state);

    }

    ErrorCode ReplicatedStore::EndOnDataLoss(
        AsyncOperationSPtr const & asyncOperation,
        __out bool & isStateChanged)
    {
        auto error = OnDataLossAsyncOperation::End(asyncOperation, isStateChanged);
        if (error.IsSuccess() && (storeEventHandler_.get() != NULL))
        {
            storeEventHandler_->OnDataLoss();
        }

        isDatalossCallbackActive_.store(false);

        return error;
    }

    ErrorCode ReplicatedStore::GetCopyContext(
        __out ComPointer<IFabricOperationDataStream> & copyContextStream)
    {
        TransactionSPtr txSPtr;
        auto error = this->CreateLocalStoreTransaction(txSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        bool isEpochValid = true;
        ::FABRIC_EPOCH epoch = { 0 };
        ::FABRIC_SEQUENCE_NUMBER lastOperationLSN = 0;

        // The secondary only has its epoch updated as replicated
        // operations are acked. This means that a recovering secondary 
        // must recover its epoch from disk for the copy context.
        // A new secondary will have no such epoch to recover.
        //
        error = this->InnerGetCurrentEpoch(txSPtr, dataSerializerUPtr_, epoch);

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            isEpochValid = false;
            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            error = this->InnerGetCurrentProgress(txSPtr, lastOperationLSN);
        }
        else
        {
            return error;
        }

        auto enumerator = make_com<ComCopyContextEnumerator>(
            this->PartitionedReplicaId, 
            isEpochValid,
            epoch, 
            lastOperationLSN, 
            this->Root);

        HRESULT hr = enumerator->QueryInterface(
            IID_IFabricOperationDataStream, 
            copyContextStream.VoidInitializationAddress());

        if (SUCCEEDED(hr))
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} ComCopyContextEnumerator doesn't support IFabricOperationDataStream. hr = {1}", 
                this->TraceId, 
                hr);

            return ErrorCode::FromHResult(hr);
        }
    }

    ErrorCode ReplicatedStore::GetCopyState(
        FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
        ComPointer<IFabricOperationDataStream> const & copyContextStream, 
        ComPointer<IFabricOperationDataStream> & copyStateStream)
    {
        WriteNoise(TraceComponent, "{0} GetCopyState() up to {1}", this->TraceId, uptoSequenceNumber);

        auto error = this->Test_GetInjectedFault(FaultInjector::FabricTest_GetCopyStateTag);
        if (!error.IsSuccess()) { return error; }

        if (copyContextStream.GetRawPointer() == nullptr)
        {
            WriteError(
                TraceComponent,
                "{0} GetCopyState got a NULL copy context from the Replicator. It is likely that the Service was created with 'HasPersistedState=FALSE'. This is not a supported scenario",
                this->TraceId);

            return ErrorCodeValue::InvalidState;
        }

        vector<ProgressVectorEntry> progressVector;
        error = this->GetProgressVector(
            nullptr,
            nullptr,
            uptoSequenceNumber, 
            progressVector);

        if (error.IsSuccess())
        {
            ComPointer<ComCopyOperationEnumerator> enumerator;
            error = ComCopyOperationEnumerator::CreateForRemoteCopy(
                *this,
                std::move(progressVector),
                uptoSequenceNumber, 
                copyContextStream, 
                this->Root,
                enumerator);

            if (error.IsSuccess())
            {
                HRESULT hr = enumerator->QueryInterface(
                    IID_IFabricOperationDataStream, 
                    copyStateStream.VoidInitializationAddress());

                if (!SUCCEEDED(hr))
                {
                    WriteWarning(
                        TraceComponent, 
                        "{0} ComCopyOperationEnumerator doesn't support IFabricOperationDataStream. hr = {1}", 
                        this->TraceId, 
                        hr);

                    error = ErrorCode::FromHResult(hr);
                }
            }
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} could not read epoch history for copy state: error = {1}",
                this->TraceId, 
                error);
        }

        return error;
    }

    // localStore and dataSerializer will point to a separate partial copy database
    // when the secondary is replaying partial copy operations between two
    // databases.
    //
    ErrorCode ReplicatedStore::GetProgressVector(
        ILocalStoreUPtr const & localStoreOverride,
        DataSerializerUPtr const & dataSerializerOverride,
        FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
        __out vector<ProgressVectorEntry> & result)
    {
        auto const & localStoreToUse = localStoreOverride ? localStoreOverride : this->LocalStore;
        auto const & dataSerializerToUse = dataSerializerOverride ? dataSerializerOverride : dataSerializerUPtr_;
            
        TransactionSPtr txSPtr;
        auto error = this->CreateLocalStoreTransaction(localStoreToUse, txSPtr);
        
        if (!error.IsSuccess()) { return error; }

        FABRIC_EPOCH currentEpoch = { 0 };
        ProgressVectorData epochHistory;

        error = this->InnerGetCurrentEpoch(txSPtr, dataSerializerToUse, currentEpoch);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} could not read current epoch for copy state: error = {1}",
                this->TraceId, 
                error);

            // Must be able to read current epoch
            return error;
        }

        error = dataSerializerToUse->TryReadData(
            txSPtr,
            Constants::ProgressDataType,
            Constants::EpochHistoryDataName,
            epochHistory);

        if (error.IsError(ErrorCodeValue::NotFound) && uptoSequenceNumber == 0)
        {
            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            vector<ProgressVectorEntry> progressVector(std::move(epochHistory.ProgressVector));
            progressVector.push_back(ProgressVectorEntry(currentEpoch, uptoSequenceNumber));

            result = move(progressVector);
        }

        return error;
    }

    ErrorCode ReplicatedStore::GetCurrentEpoch(__out FABRIC_EPOCH & epoch) const
    {
        epoch = this->GetCachedCurrentEpoch();

        return ErrorCodeValue::Success;
    }

    FABRIC_EPOCH ReplicatedStore::GetCachedCurrentEpoch() const
    {
        AcquireReadLock lock(cachedCurrentEpochLock_);

        return cachedCurrentEpoch_;
    }

    void ReplicatedStore::UpdateCachedCurrentEpoch(FABRIC_EPOCH const & epoch)
    {
        AcquireWriteLock lock(cachedCurrentEpochLock_);

        cachedCurrentEpoch_ = epoch;
    }

    ErrorCode ReplicatedStore::LoadCachedCurrentEpoch()
    {
        TransactionSPtr txSPtr;
        auto error = this->CreateLocalStoreTransaction(txSPtr);

        if (error.IsSuccess())
        {
            error = this->InnerLoadCachedCurrentEpoch(move(txSPtr));
        }

        return error;
    }
    
    ErrorCode ReplicatedStore::RecoverEpochHistory()
    {
        TransactionSPtr txSPtr;
        auto error = this->LocalStore->CreateTransaction(txSPtr);

        if (!error.IsSuccess()) { return error; }

        FABRIC_SEQUENCE_NUMBER progressLsn = 0;
        error = this->InnerGetCurrentProgress(txSPtr, progressLsn);

        if (!error.IsSuccess()) { return error; }

        ProgressVectorData epochHistory;
        error = dataSerializerUPtr_->TryReadData(
            txSPtr,
            Constants::ProgressDataType,
            Constants::EpochHistoryDataName,
            epochHistory);

        if (error.IsError(ErrorCodeValue::NotFound) && progressLsn == 0)
        {
            return ErrorCodeValue::Success;
        }

        if (error.IsSuccess() && epochHistory.TryTruncate(progressLsn))
        {
            WriteInfo(
                TraceComponent, 
                "{0} truncated epoch history: {1} LSN={2}", 
                this->TraceId,
                epochHistory, 
                progressLsn);

            error = dataSerializerUPtr_->WriteData(
                txSPtr, 
                epochHistory, 
                Constants::ProgressDataType,
                Constants::EpochHistoryDataName);

            if (error.IsSuccess()) 
            {
                error = txSPtr->Commit();
            }
        }

        return error;
    }

    ErrorCode ReplicatedStore::InnerLoadCachedCurrentEpoch()
    {
        TransactionSPtr txSPtr;
        auto error = this->LocalStore->CreateTransaction(txSPtr);

        if (error.IsSuccess())
        {
            error = this->InnerLoadCachedCurrentEpoch(move(txSPtr));
        }
        
        return error;
    }

    ErrorCode ReplicatedStore::InnerLoadCachedCurrentEpoch(TransactionSPtr && txSPtr)
    {
        FABRIC_EPOCH epoch = {0};
        auto error = this->InnerGetCurrentEpoch(txSPtr, dataSerializerUPtr_, epoch);

        if (error.IsSuccess())
        {
            this->UpdateCachedCurrentEpoch(epoch);
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: failed to refresh cached current epoch: {1}",
                this->TraceId,
                error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::InnerGetCurrentEpoch(
        TransactionSPtr const & txSPtr, 
        unique_ptr<Store::DataSerializer> const & dataSerializer,
        __out FABRIC_EPOCH & epoch) const
    {
        CurrentEpochData epochData;
        auto error = dataSerializer->TryReadData(
            txSPtr,
            Constants::ProgressDataType,
            Constants::CurrentEpochDataName,
            epochData);

        if (error.IsSuccess())
        {
            epoch = epochData.Epoch;
        }

        return error;
    }

    ErrorCode ReplicatedStore::UpdateReplicatorSettings(FABRIC_REPLICATOR_SETTINGS const & replicatorSettings)
    {
        auto hr = stateReplicatorCPtr_->UpdateReplicatorSettings(&replicatorSettings);

        if (SUCCEEDED(hr))
        {
            return this->LoadCopyOperationSizeFromReplicator();
        }
        else
        {
            return ErrorCode::FromHResult(hr);
        }
    }

    ErrorCode ReplicatedStore::UpdateReplicatorSettings(ReplicatorSettingsUPtr const & replicatorSettings)
    {
        ScopedHeap heap;
        FABRIC_REPLICATOR_SETTINGS settings = {0};
        replicatorSettings->ToPublicApi(heap, settings);

        return this->UpdateReplicatorSettings(settings);
    }

    ErrorCode ReplicatedStore::SetThrottleCallback(ThrottleCallback const & throttleCallback)
    {
        auto txReplicator = this->TryGetTxReplicator();

        if (!txReplicator)
        {
            return ErrorCodeValue::InvalidState;
        }

        txReplicator->UpdateThrottleCallback(throttleCallback);
        return ErrorCodeValue::Success;
    }

    // **************
    // Helper methods
    // **************

    void ReplicatedStore::Test_SetTargetCopyOperationSize(size_t size)
    {
        WriteInfo(TraceComponent, "{0} setting target copy operation size to {1} bytes", this->TraceId, size);
        targetCopyOperationSize_ = size;
    }

    ErrorCode ReplicatedStore::Test_ValidateAndMergeBackupChain(wstring const & restoreDir, __out wstring & mergedRestoreDir)
    {
        return this->ValidateAndMergeBackupChain(restoreDir, mergedRestoreDir);
    }

    void ReplicatedStore::Test_IncrementLogTruncationTimerFireCount()
    {
        if (test_IsLogTruncationTestMode_) 
        { 
            test_LogTruncationTimerFireCount_++; 
        }
    }

    bool ReplicatedStore::Test_GetCopyStreamComplete()
    {
        if (secondaryPumpSPtr_)
        {
            return secondaryPumpSPtr_->Test_GetCopyStreamComplete();
        }

        return false;
    }

    void ReplicatedStore::Test_SetCopyStreamComplete(bool value)
    {
        if (secondaryPumpSPtr_)
        {
            secondaryPumpSPtr_->SetLogicalCopyStreamComplete(value);
        }
    }

#if defined(PLATFORM_UNIX)
    bool ReplicatedStore::Test_IsSecondaryPumpClosed()
    {
        if (secondaryPumpSPtr_)
        {
            return secondaryPumpSPtr_->Test_IsPumpStateClosed();
        }
        return true;
    }

#endif

    void ReplicatedStore::Test_SetTestHookContext(TestHookContext const & testHookContext)
    {
        testHookContext_ = testHookContext;
    }

    void ReplicatedStore::Test_WaitForSignalIfSet(wstring const & signalName)
    {
        ApiSignalHelper::WaitForSignalIfSet(testHookContext_, signalName);
    }

    void ReplicatedStore::Test_BlockUpdateEpochIfNeeded()
    {
        this->Test_WaitForSignalIfSet(StateProviderBeginUpdateEpochBlock);
    }

    void ReplicatedStore::Test_BlockSecondaryPumpIfNeeded()
    {
        this->Test_WaitForSignalIfSet(StateProviderSecondaryPumpBlock);
    }

    ErrorCode ReplicatedStore::Test_GetBeginUpdateEpochInjectedFault()
    {
        return this->Test_GetInjectedFault(FaultInjector::FabricTest_BeginUpdateEpochTag);
    }

    ErrorCode ReplicatedStore::Test_GetInjectedFault(wstring const & tag)
    {
        ErrorCodeValue::Enum injectedError;
        if (FaultInjector::GetGlobalFaultInjector().TryReadError(
            tag,
            testHookContext_,
            injectedError))
        {
            return injectedError;
        }

        return ErrorCodeValue::Success;
    }

    TimeSpan const & ReplicatedStore::Test_GetLocalApplyDelay()
    {
        return test_localApplyDelay_;
    }

    void ReplicatedStore::Test_SetLocalApplyDelay(TimeSpan const & delay)
    {
        test_localApplyDelay_ = delay;
    }

    ErrorCode ReplicatedStore::InitializeRepairPolicy(
        Api::IClientFactoryPtr const & clientFactory,
        wstring const & serviceName,
        bool allowRepairUpToQuorum)
    {
        if (clientFactory.get() == nullptr)
        {
            WriteWarning(
                TraceComponent,
                "{0} InitializeRepairPolicy: skipping initialization - no client factory",
                this->TraceId);

            return ErrorCodeValue::Success;
        }

        if (serviceName.empty())
        {
            WriteWarning(
                TraceComponent,
                "{0} InitializeRepairPolicy: skipping initialization - empty service name",
                this->TraceId);

            return ErrorCodeValue::Success;
        }

        NamingUri serviceUri;
        if (!NamingUri::TryParse(serviceName, serviceUri))
        {
            WriteWarning(
                TraceComponent,
                "{0} InitializeRepairPolicy: failed to parse as Fabric URI '{1}'",
                this->TraceId,
                serviceName);

            return ErrorCodeValue::InvalidNameUri;
        }

        auto error = EseReplicatedStoreRepairPolicy::Create(
            clientFactory, 
            serviceUri,
            allowRepairUpToQuorum,
            *this,
            repairPolicySPtr_);

        if (!error.IsSuccess())
        {
            TRACE_ERROR_AND_TESTASSERT(
                TraceComponent,
                "{0} EseReplicatedStoreRepairPolicy::Create({1}) failed: error={2}",
                this->TraceId,
                serviceUri,
                error);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} EseReplicatedStoreRepairPolicy::Create({1}) succeeded",
                this->TraceId,
                serviceUri);
        }

        return error;
    }

    void ReplicatedStore::Test_TryFaultStreamAndStopSecondaryPump(__in FABRIC_FAULT_TYPE faultType)
    {
        if (!this->EnableStreamFaultsAndEOSOperationAck)
        {
            return;
        }

        if (secondaryPumpSPtr_)
        {
            secondaryPumpSPtr_->Test_FaultStreamAndStopSecondaryPump(faultType);
        }
    }

    ErrorCode ReplicatedStore::CreateLocalStoreTransaction(__out TransactionSPtr & txSPtr)
    {
        AcquireReadLock lock(dropLocalStoreLock_);

        if (this->IsLocalStoreClosed)
        {
            WriteInfo(
                TraceComponent,
                "{0}: CreateLocalStoreTransaction(): local store closed",
                this->TraceId);

            return ErrorCodeValue::ObjectClosed;
        }

        return this->LocalStore->CreateTransaction(txSPtr);
    }

    ErrorCode ReplicatedStore::CreateLocalStoreTransaction(ILocalStoreUPtr const & localStore, __out TransactionSPtr & txSPtr)
    {
        AcquireReadLock lock(dropLocalStoreLock_);

        // Replica lifetime is still governed by canonical local store
        // even when reading from a separate temporary database
        //
        if (this->IsLocalStoreClosed)
        {
            WriteInfo(
                TraceComponent,
                "{0}: CreateLocalStoreTransaction(): local store closed",
                this->TraceId);

            return ErrorCodeValue::ObjectClosed;
        }

        return localStore->CreateTransaction(txSPtr);
    }

    ErrorCode ReplicatedStore::CleanupLocalStore()
    {
        WriteInfo(
            TraceComponent, 
            "{0} CleanupLocalStore",
            this->TraceId);

        ErrorCode error = ErrorCodeValue::Success;

        // Terminate under read lock to abort outstanding
        // local store activity (such as backup)
        //
        {
            AcquireReadLock lock(dropLocalStoreLock_);

            if (this->LocalStore)
            {
                error = this->LocalStore->Cleanup();
                if (!error.IsSuccess())
                {
                    // Cleanup failed, request drop
                    WriteError(
                        TraceComponent, 
                        "{0} CleanupLocalStore failed: {1}",
                        this->TraceId,
                        error);

                    this->ReportPermanentFault(error);
                }
            }
        }

        {
            AcquireWriteLock lock(dropLocalStoreLock_);

            if (this->LocalStore)
            {
                this->ReleaseLocalStore();

                this->BestEffortDeleteAllCopyDatabases();
            }
        }

        this->SetTombstoneCount(0);

        return error;
    }

    ErrorCode ReplicatedStore::TerminateLocalStore()
    {
        AcquireReadLock grab(dropLocalStoreLock_);

        isLocalStoreTerminated_.store(true);

        if (this->LocalStore)
        {
            WriteInfo(TraceComponent, "{0} terminating local store", this->TraceId);

            return this->LocalStore->Terminate();
        }

        return ErrorCodeValue::Success;
    }

    bool ReplicatedStore::get_IsLocalStoreClosed() const
    {
        return (!this->LocalStore || isLocalStoreTerminated_.load());
    }

    bool ReplicatedStore::get_IsLocalStoreLogTruncationRequired()
    {
        return (settings_.LogTruncationInterval > TimeSpan::Zero &&
                this->LocalStore &&
                this->LocalStore->IsLogTruncationRequired());
    }

    wstring ReplicatedStore::GetCopyInstanceName(wstring const & copyTag) const
    {
        return wformatString("{0}_{1}_copy", this->LocalStoreInstanceName, copyTag);
    }

    // This function serves two purposes:
    //
    // 1) Recovers the estimated tombstone entry count
    //
    // 2) Migrates tombstones between the version 1 and version 2 format as needed
    //    depending on which cleanup algorithm version is enabled
    //
    ErrorCode ReplicatedStore::RecoverTombstones()
    {
        TransactionSPtr txSPtr;
        EnumerationSPtr enumSPtr;

        auto error = this->LocalStore->CreateTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        error = this->LocalStore->CreateEnumerationByTypeAndKey(
            txSPtr, 
            Constants::TombstoneDataType, 
            L"", 
            enumSPtr);
        if (!error.IsSuccess()) { return error; }

        size_t tombstoneCount = 0;
        size_t migratedBatchCount = 0;
        size_t migratedTotalCount = 0;
        bool useVersion1Format = !(settings_.EnableTombstoneCleanup2);

#if defined(PLATFORM_UNIX)
        bool shouldRecountTombstone = false;
#endif

        unordered_map<FABRIC_SEQUENCE_NUMBER, size_t> migrationIndexByLsn;

        WriteInfo(
            TraceComponent, 
            "{0} starting tombstone recovery: useVersion1Format={1}",
            this->TraceId, 
            useVersion1Format);

        while ((error = enumSPtr->MoveNext()).IsSuccess()) 
        { 
            wstring currentKey;
            error = enumSPtr->CurrentKey(currentKey);
            if (!error.IsSuccess()) { return error; }

            FABRIC_SEQUENCE_NUMBER currentLsn;
            error = enumSPtr->CurrentOperationLSN(currentLsn);
            if (!error.IsSuccess()) { return error; }

            bool shouldCountTombstone = true;

            wstring dataType;
            wstring dataKey;
            bool parsedVersion1 = this->TryParseTombstoneKey1(currentKey, dataType, dataKey);

            if (useVersion1Format && !parsedVersion1)
            {
                vector<byte> currentData;
                error = enumSPtr->CurrentValue(currentData);
                if (!error.IsSuccess()) { return error; }

                error = this->LocalStore->Delete(txSPtr, Constants::TombstoneDataType, currentKey, 0);
                if (!error.IsSuccess()) { return error; }

                TombstoneData tombstone;
                error = FabricSerializer::Deserialize(tombstone, currentData); 
                if (!error.IsSuccess()) { return error; }

                bool unusedShouldCountTombstone;
                wstring migratedKey;
                error = this->UpdateTombstone1(
                    txSPtr, 
                    tombstone.LiveEntryType,
                    tombstone.LiveEntryKey,
                    currentLsn,
                    unusedShouldCountTombstone,
                    migratedKey);
                if (!error.IsSuccess()) { return error; }

                // If the post-migration key is ordered after the current key,
                // then it will be read again by this transaction. Avoid double-counting
                // the tombstone in this case.
                //
                // The alternative would be to use a separate write transaction for
                // the migrated keys, which would no longer be visible to the read-only
                // snapshot transaction. However, the Linux local store (Rocks DB)
                // does not provide snapshot transaction semantics, so we need to avoid
                // any dependencies on snapshot semantics.
                //
                shouldCountTombstone = (migratedKey <= currentKey);

                WriteInfo(
                    TraceComponent,
                    "{0} tombstone migrated: key='{1}' -> '{2}' lsn={3} count={4} inc={5}",
                    this->TraceId, 
                    currentKey,
                    migratedKey,
                    currentLsn,
                    tombstoneCount,
                    shouldCountTombstone);

                ++migratedBatchCount;
                ++migratedTotalCount;
            }
            else if (!useVersion1Format && parsedVersion1)
            {
                error = this->LocalStore->Delete(txSPtr, Constants::TombstoneDataType, currentKey, 0);
                if (!error.IsSuccess()) { return error; }

                auto tombstoneIndex = migrationIndexByLsn[currentLsn]++;

                wstring migratedKey;
                error = this->UpdateTombstone2(
                    txSPtr, 
                    dataType,
                    dataKey,
                    currentLsn,
                    tombstoneIndex,
                    migratedKey);
                if (!error.IsSuccess()) { return error; }

                shouldCountTombstone = (migratedKey <= currentKey);

                WriteInfo(
                    TraceComponent,
                    "{0} tombstone migrated: key='{1}' -> '{2}' lsn={3} index={4} count={5} inc={6}",
                    this->TraceId, 
                    currentKey,
                    migratedKey,
                    currentLsn,
                    tombstoneIndex,
                    tombstoneCount,
                    shouldCountTombstone);

                ++migratedBatchCount;
                ++migratedTotalCount;
            }
            else
            {
                WriteNoise(
                    TraceComponent,
                    "{0} tombstone recovered: key={1} lsn={2} count={3}", 
                    this->TraceId, 
                    currentKey,
                    currentLsn,
                    tombstoneCount);
            }

            // Avoid a single transaction becoming too large
            //
            if (migratedBatchCount > StoreConfig::GetConfig().TombstoneMigrationBatchSize)
            {
                error = txSPtr->Commit();
                if (!error.IsSuccess()) { return error; }

                error = this->LocalStore->CreateTransaction(txSPtr);
                if (!error.IsSuccess()) { return error; }

                error = this->LocalStore->CreateEnumerationByTypeAndKey(
                    txSPtr, 
                    Constants::TombstoneDataType, 
                    currentKey,
                    enumSPtr);
                if (!error.IsSuccess()) { return error; }

                // Do not need to call MoveNext() here since the
                // current key must have been migrated, which means it
                // must have been deleted. So the new key enumeration
                // will actually be pointing to the next relevant key
                // on the first MoveNext() call.

                migratedBatchCount = 0;
            }

            if (shouldCountTombstone)
            {
                ++tombstoneCount; 
            }
#if defined(PLATFORM_UNIX)
            else if (!shouldRecountTombstone)
            {
                shouldRecountTombstone = true;
            }
#endif
        } // while MoveNext

#if defined(PLATFORM_UNIX)
        if (shouldRecountTombstone)
        {
            tombstoneCount = 0;
            error = this->LocalStore->CreateEnumerationByTypeAndKey(
                    txSPtr,
                    Constants::TombstoneDataType,
                    L"",
                    enumSPtr);
            if (!error.IsSuccess()) { return error; }
            while ((error = enumSPtr->MoveNext()).IsSuccess())
            {
                tombstoneCount++;
            }
        }
#endif

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            if (migratedBatchCount > 0)
            {
                error = txSPtr->Commit();
                if (!error.IsSuccess()) { return error; }
            }
            else
            {
                error = ErrorCodeValue::Success;
            }

            this->SetTombstoneCount(tombstoneCount);

            WriteInfo(
                TraceComponent, 
                "{0} tombstone recovery complete: count={1} migrated={2}",
                this->TraceId, 
                tombstoneCount_.load(),
                migratedTotalCount);
        }
        else
        {
            WriteError(
                TraceComponent,
                "{0} tombstone recovery failed: error={1}", 
                this->TraceId, 
                error);
        }

        return error;
    }

    size_t ReplicatedStore::Test_GetTombstoneCount() const
    {
        return tombstoneCount_.load();
    }

    ErrorCode ReplicatedStore::Test_TerminateLocalStore()
    {
        return this->TerminateLocalStore();
    }

    void ReplicatedStore::SetTombstoneCount(size_t count)
    {
        tombstoneCount_.store(count);

        this->UpdateTombstonePerfCounter();
    }

    void ReplicatedStore::IncrementTombstoneCount(size_t count)
    {
        tombstoneCount_ += count;

        this->UpdateTombstonePerfCounter();
    }
    
    void ReplicatedStore::DecrementTombstoneCount(size_t count)
    {
        tombstoneCount_ -= count;

        this->UpdateTombstonePerfCounter();
    }
    
    void ReplicatedStore::UpdateTombstonePerfCounter()
    {
        this->PerfCounters.TombstoneCount.Value = tombstoneCount_.load();
    }

    ErrorCode ReplicatedStore::LoadCopyOperationSizeFromReplicator()
    {
        ComPointer<IFabricStateReplicator2> comPtr;
        auto hr = stateReplicatorCPtr_->QueryInterface(IID_IFabricStateReplicator2, comPtr.VoidInitializationAddress());

        if (hr == E_NOINTERFACE)
        {
            WriteWarning(
                TraceComponent,
                "{0} IFabricStateReplicator2 not supported - cannot load copy operation size. Target={1}",
                this->TraceId,
                targetCopyOperationSize_);

            // Maintain backwards compatibility and keep defaults
            //
            return ErrorCodeValue::Success;
        }
        else if (FAILED(hr))
        {
            WriteWarning(
                TraceComponent,
                "{0} IFabricStateReplicator2 not supported - cannot load copy operation size. Target={1}. HR={2}",
                this->TraceId,
                targetCopyOperationSize_,
                hr);

            return ErrorCode::FromHResult(hr);
        }
        
        ComPointer<IFabricReplicatorSettingsResult> settingsResult;
        hr = comPtr->GetReplicatorSettings(settingsResult.InitializationAddress());

        if (FAILED(hr))
        {
            return ErrorCode::FromHResult(hr);
        }
 
        auto settings = settingsResult->get_ReplicatorSettings();
        if (settings->Reserved != NULL)
        {
            auto settingsEx1 = reinterpret_cast<FABRIC_REPLICATOR_SETTINGS_EX1*>(settings->Reserved);

            auto maxReplSize = settingsEx1->MaxReplicationMessageSize;
            auto ratio = StoreConfig::GetConfig().CopyOperationSizeRatio;

            if (maxReplSize < targetCopyOperationSize_)
            {
                targetCopyOperationSize_ = static_cast<size_t>(maxReplSize * ratio);
            }

            WriteInfo(
                TraceComponent,
                "{0} setting TargetCopyOperationSize={1} bytes from MaxReplicationMessageSize={2} Ratio={3}",
                this->TraceId,
                targetCopyOperationSize_,
                maxReplSize,
                ratio);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} FABRIC_REPLICATOR_SETTINGS_EX1 does not exist. Target={1}",
                this->TraceId,
                targetCopyOperationSize_);
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::GetLocalStoreForFullCopy(__out ILocalStoreUPtr & copyStore)
    {
        return this->GetLocalStoreForCopy(
            copyStore,
            L"full",
            [this]{ return this->CreateFullCopyLocalStore(); });
    }

    ErrorCode ReplicatedStore::GetLocalStoreForPartialCopy(__out ILocalStoreUPtr & copyStore)
    {
        return this->GetLocalStoreForCopy(
            copyStore,
            L"partial",
            [this]{ return this->CreatePartialCopyLocalStore(); });
    }

    ErrorCode ReplicatedStore::GetLocalStoreForCopy(
        __out ILocalStoreUPtr & copyStore, 
        wstring const & copyTag,
        function<ILocalStoreUPtr(void)> const & createStoreFunc)
    {
        Stopwatch stopwatch;

        stopwatch.Start();

        // If this secondary is restarting a new full/partial copy for whatever reason,
        // then clear the previous copy database first (if any).
        //
        if (copyStore)
        {
            auto error = copyStore->Cleanup();
            if (!error.IsSuccess()) 
            { 
                WriteInfo(
                    TraceComponent,
                    "{0}: Failed to cleanup {1} copy store: {2}",
                    this->TraceId,
                    copyTag,
                    error);
                return error; 
            }
        }

        auto cleanupTime = stopwatch.Elapsed;

        copyStore = createStoreFunc();

        auto error = copyStore->Initialize(this->GetCopyInstanceName(copyTag));

        stopwatch.Stop();

        auto createTime = stopwatch.Elapsed;

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: created store for {1} copy. Time: cleanup={2} create={3}",
                this->TraceId,
                copyTag,
                cleanupTime,
                createTime);
        }

        return error;
    }

    ErrorCode ReplicatedStore::FinishFileStreamFullCopy(CopyType::Enum copyType, wstring const & srcRestoreDirectory)
    {
        AcquireWriteLock grab(dropLocalStoreLock_);

        if (this->IsLocalStoreClosed)
        {
            // Replicator will also pump a null copy operation when it closes
            return ErrorCodeValue::ObjectClosed;
        }

        WriteInfo(
            TraceComponent,
            "{0}: applying file stream full copy ({1}): {2}",
            this->TraceId,
            copyType,
            srcRestoreDirectory);

        if (copyType == CopyType::FileStreamRebuildCopy)
        {
#ifndef PLATFORM_UNIX
            return this->RebuildFileStreamToNewLocalStore(srcRestoreDirectory);
#endif
        }
        else
        {
            return this->RestoreFileStreamToMainLocalStore(srcRestoreDirectory);
        }
    }

    ErrorCode ReplicatedStore::RestoreFileStreamToMainLocalStore(wstring const & srcRestoreDirectory)
    {
        // Any pre-caching performed during open is now irrelevant since we are
        // about to swap databases
        //
        this->PostCopyNotification();

        Stopwatch stopwatch;

        stopwatch.Start();

        int maxRetryCount = StoreConfig::GetConfig().SwapDatabaseRetryCount;
        int retryCount = maxRetryCount;
        bool success = false;

        while (!success)
        {
            auto error = this->LocalStore->PrepareRestore(srcRestoreDirectory);

            if (error.IsSuccess())
            {
                success = true;
            }
            else if (error.IsError(ErrorCodeValue::AccessDenied) && --retryCount > 0)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} couldn't prepare restore from {1}: error={2} retry={3} max={4}",
                    this->TraceId,
                    srcRestoreDirectory,
                    error,
                    retryCount,
                    maxRetryCount);

                Sleep(StoreConfig::GetConfig().SwapDatabaseRetryDelayMilliseconds);
            }
            else
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: failed to prepare restore {1}: error={2}",
                    this->TraceId,
                    srcRestoreDirectory,
                    error);

                return error;
            }
        }

        this->AppendQueryStatusDetails(wformatString(GET_STORE_RC(Drain_Store), DateTime::Now()));

        auto error = this->LocalStore->Terminate();

        if (!error.IsSuccess()) { return error; }

        this->LocalStore->Drain();

        this->ReleaseLocalStore();

        this->AppendQueryStatusDetails(wformatString(GET_STORE_RC(FS_FullCopy_Restore), DateTime::Now()));

        auto elapsedPrepare = stopwatch.Elapsed;

        this->CreateLocalStore();

        error = this->InnerInitializeLocalStoreAndSerializer();

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: failed to re-initialize from restore {1}: error={2}",
                this->TraceId,
                srcRestoreDirectory,
                error);

            return error;
        }

        auto elapsedRestore = stopwatch.Elapsed;

        WriteInfo(
            TraceComponent,
            "{0}: restored full copy. Time: prepare={1} restore={2}",
            this->TraceId,
            elapsedPrepare,
            elapsedRestore);

        this->ClearQueryStatusDetails();

        return error;
    }

#ifndef PLATFORM_UNIX
    ErrorCode ReplicatedStore::RebuildFileStreamToNewLocalStore(wstring const & srcRestoreDirectory)
    {
        this->BestEffortDeleteAllCopyDatabases();

        auto srcStore = this->CreateRebuildLocalStore();

        auto error = srcStore->PrepareRestore(srcRestoreDirectory);
        if (!error.IsSuccess()) { return error; }

        error = srcStore->Initialize(this->GetCopyInstanceName(L"rebuild_src"));
        if (!error.IsSuccess()) { return error; }
        
        auto destStore = this->CreateFullCopyLocalStore();

        error = destStore->Initialize(this->GetCopyInstanceName(L"rebuild_dest"));
        if (!error.IsSuccess()) { return error; }
        
        TransactionSPtr srcTx;
        EnumerationSPtr srcEnum;
        error = this->CreateEnumerationByPrimaryIndex(srcStore, L"", L"", srcTx, srcEnum);
        if (!error.IsSuccess()) { return error; }

        TransactionSPtr destTx;
        error = destStore->CreateTransaction(destTx);
        if (!error.IsSuccess()) { return error; }

        size_t sizeEstimate = 0;
        while ((error = srcEnum->MoveNext()).IsSuccess())
        {
            wstring type;
            wstring key;
            vector<byte> data;
            FABRIC_SEQUENCE_NUMBER lsn;
            FILETIME lastModifiedOnPrimary;

            if (!(error = srcEnum->CurrentType(type)).IsSuccess()) { return error; }
            if (!(error = srcEnum->CurrentKey(key)).IsSuccess()) { return error; }
            if (!(error = srcEnum->CurrentValue(data)).IsSuccess()) { return error; }
            if (!(error = srcEnum->CurrentOperationLSN(lsn)).IsSuccess()) { return error; }
			if (!(error = srcEnum->CurrentLastModifiedOnPrimaryFILETIME(lastModifiedOnPrimary)).IsSuccess()) { return error; }

            error = destStore->Insert(
                destTx,
                type,
                key,
                data.data(),
                data.size(),
                lsn,
                &lastModifiedOnPrimary);
            if (!error.IsSuccess()) { return error; }

            sizeEstimate += (type.size() * sizeof(wchar_t));
            sizeEstimate += (key.size() * sizeof(wchar_t));
            sizeEstimate += data.size();
            sizeEstimate += sizeof(FABRIC_SEQUENCE_NUMBER);
            sizeEstimate += sizeof(FILETIME);

            if (sizeEstimate > static_cast<size_t>(StoreConfig::GetConfig().DatabaseRebuildBatchSizeInBytes))
            {
                error = destTx->Commit();
                if (!error.IsSuccess()) { return error; }

                error = destStore->CreateTransaction(destTx);
                if (!error.IsSuccess()) { return error; }

                sizeEstimate = 0;
            }
        }

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            if (sizeEstimate > 0)
            {
                error = destTx->Commit();
            }
            else
            {
                error = ErrorCodeValue::Success;
            }
        }

        if (!error.IsSuccess()) { return error; }

        srcTx.reset();
        srcEnum.reset();
        destTx.reset();

        return this->FinishFullCopy_CallerHoldsLock(move(destStore));
    }
#endif

    ErrorCode ReplicatedStore::FinishFullCopy(ILocalStoreUPtr && copyStore)
    {
        AcquireWriteLock grab(dropLocalStoreLock_);

        if (this->IsLocalStoreClosed)
        {
            // Replicator will also pump a null copy operation when it closes
            return ErrorCodeValue::ObjectClosed;
        }

        return this->FinishFullCopy_CallerHoldsLock(move(copyStore));
    }

    ErrorCode ReplicatedStore::FinishFullCopy_CallerHoldsLock(ILocalStoreUPtr && copyStore)
    {
        if (!copyStore)
        {
            WriteInfo(
                TraceComponent,
                "{0}: no full copy store to swap",
                this->TraceId);

            return this->InnerLoadCachedCurrentEpoch();
        }

        this->SetQueryStatusDetails(wformatString(GET_STORE_RC(Drain_Store), DateTime::Now()));

        // Any pre-caching performed during open is now irrelevant since we are
        // about to swap databases
        //
        this->PostCopyNotification();

        Stopwatch stopwatch;

        stopwatch.Start();

        auto error = copyStore->Terminate();
        if (!error.IsSuccess()) { return error; }

        copyStore->Drain();

        copyStore.reset();

        auto copyCleanupTime = stopwatch.Elapsed;

        error = this->LocalStore->Terminate();
        if (!error.IsSuccess()) { return error; }

        this->LocalStore->Drain();

        this->ReleaseLocalStore();

        auto localCleanupTime = stopwatch.Elapsed;

        this->AppendQueryStatusDetails(wformatString(GET_STORE_RC(FullCopy_Start), DateTime::Now()));

        error = this->CreateLocalStoreFromFullCopyData();
        if (!error.IsSuccess()) { return error; }

        error = this->InnerInitializeLocalStoreAndSerializer();

        auto swapTime = stopwatch.Elapsed;

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: swapped local store on copy completion. Time: copyCleanup={1} localCleanup={2} swap={3}",
                this->TraceId,
                copyCleanupTime,
                localCleanupTime,
                swapTime);

            error = this->InnerLoadCachedCurrentEpoch();
        }

        this->ClearQueryStatusDetails();

        return error;
    }

    ErrorCode ReplicatedStore::RecoverPartialCopy()
    {
        ILocalStoreUPtr partialCopyStore;

        auto error = this->TryRecoverPartialCopyLocalStore(partialCopyStore);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: TryRecoverPartialCopyLocalStore() failed: error={1}",
                this->TraceId,
                error);

            return error;
        }

        if (!partialCopyStore)
        {
            WriteInfo(
                TraceComponent,
                "{0}: no completed partial copy store to recover",
                this->TraceId);

            return ErrorCodeValue::Success;
        }

        WriteInfo(
            TraceComponent,
            "{0}: recovering partial copy database",
            this->TraceId);

        error = partialCopyStore->Initialize(this->GetCopyInstanceName(L"recover"));

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0}: RecoverPartialCopy() failed to initialize local store: error={1}",
                this->TraceId,
                error);

            return error;
        }

        auto tempSecondaryPump = make_shared<SecondaryPump>(*this, stateReplicatorCPtr_);

        error = this->FinishPartialCopy(
            *tempSecondaryPump,
            move(partialCopyStore));

        WriteInfo(
            TraceComponent,
            "{0}: partial copy database recovery complete: error={1}",
            this->TraceId,
            error);

        return error;
    }

    ErrorCode ReplicatedStore::FinishPartialCopy(
        __in SecondaryPump & secondaryPump,
        ILocalStoreUPtr && copyStore)
    {
        if (!copyStore)
        {
            WriteInfo(
                TraceComponent,
                "{0}: no partial copy store to apply",
                this->TraceId);

            return ErrorCodeValue::Success;
        }

        // Must move into local variable since this causes SecondaryPump::GetCurrentLocalStore()
        // to return the replicated store's ESE store, which is the correct target for applying
        // local copy operations.
        //
        auto partialCopyStore = move(copyStore);

        auto error = this->MarkPartialCopyLocalStoreComplete();

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} MarkPartialCopyLocalStoreComplete() failed: error={1}",
                this->TraceId,
                error);

            this->CleanupPartialCopy(move(partialCopyStore)).ReadValue();

            return error;
        }

        FABRIC_SEQUENCE_NUMBER lastCommittedLsn;
        error = this->GetLastCommittedSequenceNumber(lastCommittedLsn);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} GetLastCommittedSequenceNumber() failed: error={1}",
                this->TraceId,
                error);

            this->CleanupPartialCopy(move(partialCopyStore)).ReadValue();

            return error;
        }

        // Start at next LSN past what is already committed in the current active database
        //
        ++lastCommittedLsn;

        WriteInfo(
            TraceComponent,
            "{0}: applying partial copy store to active store: starting LSN={1}",
            this->TraceId,
            lastCommittedLsn);

        error = this->MarkPartialCopyReplayInProgress(lastCommittedLsn);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} MarkPartialCopyReplayInProgress() failed: error={1}",
                this->TraceId,
                error);

            this->CleanupPartialCopy(move(partialCopyStore)).ReadValue();

            return error;
        }

        ComPointer<ComCopyOperationEnumerator> copyOperationEnumerator;
        error = ComCopyOperationEnumerator::CreateForLocalCopy(
            *this,
            partialCopyStore,
            lastCommittedLsn,
            this->Root,
            copyOperationEnumerator);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} ComCopyOperationEnumerator::CreateForLocalCopy() failed: error={1}",
                this->TraceId,
                error);

            this->CleanupPartialCopy(move(partialCopyStore)).ReadValue();

            return error;
        }

        Stopwatch stopwatch;
        FABRIC_SEQUENCE_NUMBER mockLsn = 0;
        CopyOperation copyOperation;

        this->SetQueryStatusDetails(wformatString(GET_STORE_RC(PartialCopy_Start), DateTime::Now()));

        stopwatch.Start();

        // Locally replay all operations sent by the primary from the partial copy database
        // to the currently attached database. The database is not transactionally consistent
        // until all operations have been replayed. If this replica finds a *completed* partial 
        // copy database during open, then it will try to continue replaying the partial copy before
        // returning from open. If the partial copy database is *incomplete*, then it will
        // be deleted.
        //
        for (bool done = false; !done; ++mockLsn)
        {
            error = copyOperationEnumerator->GetNextCopyOperationForLocalApply(copyOperation);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0} GetNextCopyOperationForLocalApply() failed: error={1}",
                    this->TraceId,
                    error);

                copyOperationEnumerator.SetNoAddRef(nullptr);

                this->CleanupPartialCopy(move(partialCopyStore)).ReadValue();

                return error;
            }

            done = copyOperation.IsEmpty;

            if (!done)
            {
                error = secondaryPump.ApplyLocalOperations(
                    move(copyOperation),
                    mockLsn);

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} ApplyLocalOperations() failed: error={1}",
                        this->TraceId,
                        error);

                    copyOperationEnumerator.SetNoAddRef(nullptr);

                    this->CleanupPartialCopy(move(partialCopyStore)).ReadValue();

                    return error;
                }
            }
        }

        copyOperationEnumerator.SetNoAddRef(nullptr);

        error = this->MarkPartialCopyReplayCompleted();

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} MarkPartialCopyReplayCompleted() failed: error={1}",
                this->TraceId,
                error);

            this->CleanupPartialCopy(move(partialCopyStore));

            return error;
        }

        auto applyTime = stopwatch.Elapsed;

        error = this->CleanupPartialCopy(move(partialCopyStore));

        stopwatch.Stop();

        auto cleanupTime = stopwatch.Elapsed;

        WriteInfo(
            TraceComponent,
            "{0} FinishPartialCopy() complete: apply={1} cleanup={2} error={3}",
            this->TraceId,
            applyTime,
            cleanupTime,
            error);

        this->ClearQueryStatusDetails();

        return error;
    }

    ErrorCode ReplicatedStore::MarkPartialCopyReplayInProgress(FABRIC_SEQUENCE_NUMBER lsn)
    {
        TransactionSPtr txSPtr;
        auto error = this->LocalStore->CreateTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        auto serializer = make_unique<Store::DataSerializer>(
            this->PartitionedReplicaId, 
            *this->LocalStore);

        auto data = make_unique<PartialCopyProgressData>(lsn);
        error = serializer->WriteData(txSPtr, *data, data->Type, data->Key);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to update partial copy replay data {1}: {2}",
                this->TraceId,
                *data,
                error);

            return error;
        }

        return error;
    }

    ErrorCode ReplicatedStore::MarkPartialCopyReplayCompleted()
    {
        TransactionSPtr txSPtr;
        auto error = this->LocalStore->CreateTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        auto temp = make_unique<PartialCopyProgressData>();
        error = this->LocalStore->Delete(txSPtr, temp->Type, temp->Key);

        if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
        {
            error = ErrorCodeValue::Success;
        }

        return error;
    }

    ErrorCode ReplicatedStore::CheckPartialCopyReplayCompleted()
    {
        TransactionSPtr txSPtr;
        auto error = this->LocalStore->CreateTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        if (!dataSerializerUPtr_)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} data serializer not initialized", this->TraceId);

            return ErrorCodeValue::InvalidState;
        }

        auto data = make_unique<PartialCopyProgressData>();
        error = dataSerializerUPtr_->TryReadData(txSPtr, data->Type, data->Key, *data);

        if (error.IsSuccess())
        {
            // Partial copy entry should have been cleaned up after a successful local
            // replay. If not, then the data may be transactionally inconsistent and
            // we should attempt to repair with a full build if possible.
            //
            // Consider:
            //
            // Tx1: LSN=1 write(key A) write(key B)
            // Tx2: LSN=2 write(key B')
            //
            // The database rows are not transactionally consistent if we've only replayed
            // up to LSN 1 since only key A exists.
            //
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} partial copy replay incomplete: {1}", this->TraceId, *data);

            error = ErrorCodeValue::DatabaseFilesCorrupted;
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::Success;
        }

        return error;
    }

    ErrorCode ReplicatedStore::CleanupPartialCopy(ILocalStoreUPtr && partialCopyStore)
    {
        auto error = partialCopyStore->Terminate();

        if (error.IsSuccess()) 
        {
            partialCopyStore->Drain();

            error = this->DeletePartialCopyDatabases();
        }

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} CleanupPartialCopy() failed: error={1}",
                this->TraceId,
                error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::InitializeLocalStoreForUnittests(bool databaseShouldExist)
    {
        {
            ManualResetEvent waiter(false);

            auto operation = BeginOpen(
                databaseShouldExist ? FABRIC_REPLICA_OPEN_MODE_EXISTING : FABRIC_REPLICA_OPEN_MODE_NEW,
                make_com<Naming::MockStatefulServicePartition, IFabricStatefulServicePartition>(this->PartitionId),
                [&](AsyncOperationSPtr const &) { waiter.Set(); },
                Root.CreateAsyncOperationRoot());

            waiter.WaitOne();

            ComPointer<IFabricReplicator> unused;
            auto error = EndOpen(operation, unused);

            if (!error.IsSuccess()) { return error; }
        }

        {
            ManualResetEvent waiter(false);

            ::FABRIC_REPLICA_ROLE newRole = ::FABRIC_REPLICA_ROLE_NONE;

            auto operation = BeginChangeRole(
                ::FABRIC_REPLICA_ROLE_PRIMARY,
                [&](AsyncOperationSPtr const &) { waiter.Set(); },
                Root.CreateAsyncOperationRoot());

            waiter.WaitOne();

            auto error = ChangeRoleAsyncOperation::End(operation, newRole);

            if (!error.IsSuccess()) { return error; }

            replicaRole_ = newRole;
            isRoleSet_ = true;
        }
        
        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::InitializeLocalStoreForJobQueue(bool databaseShouldExist)
    {
        return this->InitializeLocalStore(databaseShouldExist);
    }

    ErrorCode ReplicatedStore::InitializeLocalStore(bool shouldExist)
    {
        ASSERT_IF(this->LocalStore, "ReplicatedStore::InitializeLocalStore: Store already exists");

        this->CreateLocalStore();

        if (!StoreConfig::GetConfig().IgnoreOpenLocalStoreFlag)
        {
            bool storeExists = this->LocalStore->StoreExists();

            if (shouldExist && !storeExists)
            {
                auto msg = wformatString("{0} Open called with FABRIC_REPLICA_OPEN_MODE_EXISTING and database does not exist", this->TraceId);
                WriteWarning(TraceComponent, "{0}", msg);

                auto error = ErrorCodeValue::DatabaseFilesCorrupted;
                this->ReportPermanentFault(error);
                return error;
            }
            else if (!shouldExist && storeExists)
            {
                auto msg = wformatString("{0} Open called with FABRIC_REPLICA_OPEN_MODE_NEW and database does exist", this->TraceId);
                WriteWarning(TraceComponent, "{0}", msg);

                auto error = ErrorCodeValue::DatabaseFilesCorrupted;
                this->ReportPermanentFault(error);
                return error;
            }

            if (shouldExist)
            {
                auto injectedError = this->Test_GetInjectedFault(FaultInjector::ReplicatedStoreOpenTag);
                if (!injectedError.IsSuccess()) { return injectedError; }
            }
        }

        return this->InnerInitializeLocalStoreAndSerializer();
    }

    ErrorCode ReplicatedStore::InnerInitializeLocalStoreAndSerializer()
    {
        bool reportedHealth = false;
        auto error = this->CheckPreOpenLocalStoreHealth(reportedHealth); // out
        if (!error.IsSuccess()) { return error; }

        error = this->LocalStore->Initialize(this->LocalStoreInstanceName);
        if (!error.IsSuccess()) { return error; }
        
        error = this->CheckPostOpenLocalStoreHealth(reportedHealth); // cleanupHealth
        if (!error.IsSuccess()) { return error; }

        dataSerializerUPtr_ = make_unique<Store::DataSerializer>(
            this->PartitionedReplicaId, 
            *this->LocalStore);

        return error;
    }

    bool ReplicatedStore::TryAcquireActiveBackupState(
        __in StoreBackupRequester::Enum backupRequester,
        __out ScopedActiveBackupStateSPtr & scopedBackupState)
    {
        return ScopedActiveBackupState::TryAcquire(storeBackupState_, backupRequester, scopedBackupState);
    }

    ErrorCode ReplicatedStore::BackupLocal(std::wstring const & dir)
    {
        ManualResetEvent waiter;

        auto operation = BeginBackupLocal(
            dir, 
            StoreBackupOption::Full, 
            Api::IStorePostBackupHandlerPtr(),
            StoreBackupRequester::User,
            [&](AsyncOperationSPtr const &) { waiter.Set(); },
            nullptr);

        waiter.WaitOne();

        auto error = EndBackupLocal(operation);
        
        return error;
    }

    /// <summary>
    /// Backs up the replicated store to the specified directory.
    /// </summary>
    /// <param name="dir">The destination directory where the store should be backed up to.</param>
    /// <param name="backupOption">The backup option. The default option is StoreBackupOption::Full.</param>
    /// <returns>ErrorCode::Success() if the backup was successfully done. An appropriate ErrorCode otherwise.</returns>
    ErrorCode ReplicatedStore::BackupLocalImpl(
        wstring const & dir, 
        StoreBackupOption::Enum backupOption,
        __out Guid & backupChainFullBackupGuid,
        __out ULONG & backupIndex)
    {
        AcquireReadLock lock(dropLocalStoreLock_);

        if (this->IsLocalStoreClosed)
        {
            return ErrorCodeValue::ObjectClosed;
        }

		if (this->LocalStore->IsIncrementalBackupEnabled() && !(allowIncrementalBackup_.load()))
		{
			TransactionSPtr txSPtr;
			auto error = this->LocalStore->CreateTransaction(txSPtr);
			if (!error.IsSuccess())
			{
				WriteWarning(
					TraceComponent,
					"{0} ReplicatedStore::BackupLocalImpl: failed to create transaction. Error = {1}",
					this->TraceId,
					error);

				return error;
			}

			error = this->DisableIncrementalBackup(txSPtr);
			if (!error.IsSuccess())
			{
				WriteWarning(
					TraceComponent,
					"{0} ReplicatedStore::BackupLocalImpl: failed to disable incremental backup. Error = {1}",
					this->TraceId,
					error);

				return error;
			}

			allowIncrementalBackup_.store(true);
		}

		// Set values to start of a new backup chain.
        backupChainFullBackupGuid = Guid::NewGuid();
        backupIndex = 0;

        ErrorCode error;
        if (backupOption == StoreBackupOption::Incremental)
        {
            bool allowed = false;

            TransactionSPtr txSPtr;
            error = this->LocalStore->CreateTransaction(txSPtr);
            if (!error.IsSuccess()) 
            { 
                WriteWarning(
                    TraceComponent,
                    "{0} ReplicatedStore::BackupLocalImpl: failed to create transaction. Error = {1}",
                    this->TraceId,
                    error);

                return error; 
            }

            error = GetIncrementalBackupState(txSPtr, allowed, backupChainFullBackupGuid, backupIndex);
            if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound))
            {
                return error;
            }

            if (!allowed)
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: ReplicatedStore::BackupLocalImpl: Incremental backup is not allowed. The allowed backup options are Full or TruncateLogsOnly.",
                    this->TraceId);

                return ErrorCodeValue::MissingFullBackup;
            }
        }

        error = this->LocalStore->Backup(dir, backupOption);

        if (!error.IsSuccess()) 
        { 
            if (error.IsError(ErrorCodeValue::StoreFatalError))
            {
                this->ReportTransientFault(error);
            }

            return error; 
        }

        // With truncate logs, there is nothing useful that can be done with
        // the destination backup folder. So, skip writing this metadata.
        if (backupOption == StoreBackupOption::TruncateLogsOnly)
        {
            return error;
        }

        if (backupOption == StoreBackupOption::Full)
        {
            error = this->WriteRestoreMetadata(dir, StoreBackupType::Full, backupChainFullBackupGuid, backupIndex);
        }
        else if (backupOption == StoreBackupOption::Incremental)
        {
            backupIndex++;
            error = this->WriteRestoreMetadata(dir, StoreBackupType::Incremental, backupChainFullBackupGuid, backupIndex);
        }
                
        return error;
    }
    
    ErrorCode ReplicatedStore::RestoreLocal(std::wstring const & dir, RestoreSettings const & restoreSettings)
    {
        ErrorCode error;

        if (dir.empty())
        {
            error = ErrorCodeValue::InvalidArgument;
            WriteWarning(
                TraceComponent,
                "{0}: Empty restore dir path provided: {1}",
                this->TraceId,
                error);

            return error;
        }

        if (!Directory::Exists(dir))
        {
            error = ErrorCodeValue::DirectoryNotFound;
            WriteWarning(
                TraceComponent,
                "{0}: The restore dir {1} does not exist: {2}",
                this->TraceId,
                dir,
                error);

            return error;
        }
        
        WriteInfo(
            TraceComponent,
            "{0}: Beginning local restore from dir: {1}",
            this->TraceId,
            dir);

        AcquireReadLock grab(dropLocalStoreLock_);
        
        if (this->IsLocalStoreClosed)
        {
            error = ErrorCodeValue::ObjectClosed;

            WriteWarning(
                TraceComponent,
                "{0}: Not proceeding with restore due to inner store error: {1}",
                this->TraceId,
                error);
            
            return error;
        }

        // Check: Partition information is consistent with the backup.
        // The partition ID does not need to match, but the partition
        // type and range must match.
        //
        wstring mergedRestoreDir;
        error = this->ValidateAndMergeBackupChain(dir, mergedRestoreDir);
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent,
                "{0}: Not proceeding with restore since metadata validation failed: {1}",
                this->TraceId,
                error);

            return error; 
        }

        // Check: Perform a test restore to a temporary local
        // ESE instance to ensure that the backup works. 
        //
        // If specified in restore settings, perform validation of the restored 
        // data's progress against the existing data's progress to protect against 
        // accidental dataloss.
        //
        // Any manipulation of the metadata (e.g. progress vectors)
        // before restoring should also occur at this point (future
        // work item).
        //
        error = this->ValidateRestoreServiceData(mergedRestoreDir, restoreSettings.EnableLsnCheck);
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent,
                "{0}: Not proceeding with restore since service's data validation failed: {1}",
                this->TraceId,
                error);

            return error; 
        }

        // All checks have passed. Move from the temporary validation restore
        // directory to the actual restore directory and restart the replica.
        //
        error = this->LocalStore->PrepareRestoreFromValidation();
        if (!error.IsSuccess())
        {
            return error;
        }        

        if (restoreSettings.InlineReopen)
        {
            error = this->InlineReopen();
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: Reporting transient fault to restart replica since restore was successfully done from dir: {1}",
                this->TraceId,
                dir);

            this->ReportTransientFault(ErrorCodeValue::ObjectClosed);
        }

        return error;
    }

    ErrorCode ReplicatedStore::InlineReopen()
    {
        if (isDatalossCallbackActive_.load())
        {
            WriteInfo(
                TraceComponent,
                "{0}: Performing inline re-initialize",
                this->TraceId);

            auto error = this->LocalStore->Terminate();
            if (!error.IsSuccess()) 
            { 
                WriteWarning(
                    TraceComponent,
                    "{0}: Failed to terminate for inline re-initialize: {1}",
                    this->TraceId,
                    error);
                return error; 
            }

            // Abort all outstanding application transactions
            //
            txTrackerUPtr_->StartDrainTimer();

            // Wait for all ESE transactions to complete since
            // we are about to invalidate the old ESE local instance
            //
            this->LocalStore->Drain();

            txTrackerUPtr_->CancelDrainTimer();

            this->ReleaseLocalStore();

            this->CreateLocalStore();

            error = this->InnerInitializeLocalStoreAndSerializer();
            if (!error.IsSuccess()) 
            { 
                WriteWarning(
                    TraceComponent,
                    "{0}: Failed to re-initialize: {1}",
                    this->TraceId,
                    error);
                return error; 
            }
            
            WriteInfo(
                TraceComponent,
                "{0}: Inline re-initialize complete",
                this->TraceId);

            return error;
        }
        else
        {
            auto msg = wformatString("{0}: Inline reopen only supported within OnDataLoss callback", this->TraceId);

            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}", msg);

            return ErrorCode(ErrorCodeValue::InvalidOperation, move(msg));
        }
    }

    ErrorCode ReplicatedStore::GetQueryStatus(
        __out Api::IStatefulServiceReplicaStatusResultPtr & result)
    {
        size_t estimatedRowCount = 0;
        size_t estimatedDbSize = 0;
        {
            AcquireReadLock lock(dropLocalStoreLock_);

            if (this->IsLocalStoreClosed)
            {
                return ErrorCodeValue::ObjectClosed;
            }

            auto error = this->LocalStore->EstimateRowCount(estimatedRowCount);
            if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotImplemented))
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: GetQueryStatus: EstimateRowCount failed: {1}",
                    this->TraceId,
                    error);
                return error;
            }

            error = this->LocalStore->EstimateDbSizeBytes(estimatedDbSize);
            if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotImplemented))
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: GetQueryStatus: EstimateDbSizeBytes failed: {1}",
                    this->TraceId,
                    error);
                return error;
            }
        } // dropLocalStoreLock_

        shared_ptr<wstring> copyNotificationPrefix;
        size_t copyNotificationProgress = 0;
        if (notificationManagerUPtr_)
        {
            notificationManagerUPtr_->GetQueryStatus(
                copyNotificationPrefix, 
                copyNotificationProgress);
        }

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

        auto resultSPtr = make_shared<KeyValueStoreQueryResult>(
            estimatedRowCount,
            estimatedDbSize,
            copyNotificationPrefix,
            copyNotificationProgress,
            queryStatusDetails,
            ProviderKind::ESE,
            move(migrationDetails));

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

    void ReplicatedStore::SetQueryStatusDetails(std::wstring const & details)
    {
        AcquireWriteLock lock(queryStatusDetailsLock_);

        queryStatusDetails_ = details;
    }

    void ReplicatedStore::AppendQueryStatusDetails(std::wstring const & details)
    {
        AcquireWriteLock lock(queryStatusDetailsLock_);

        queryStatusDetails_.append(L"; ");
        queryStatusDetails_.append(details);
    }

    void ReplicatedStore::ClearQueryStatusDetails()
    {
        AcquireWriteLock lock(queryStatusDetailsLock_);

        queryStatusDetails_.clear();
    }

    void ReplicatedStore::SetMigrationQueryResult(unique_ptr<MigrationQueryResult> && details)
    {
        AcquireWriteLock lock(queryStatusDetailsLock_);

        migrationDetails_ = move(details);
    }

    void ReplicatedStore::SetTxEventHandler(IReplicatedStoreTxEventHandlerWPtr const & txEventHandler)
    {
        txEventHandler_ = txEventHandler;
    }

    void ReplicatedStore::AbortOutstandingTransactions()
    {
        txTrackerUPtr_->AbortOutstandingTransactions();
    }

    void ReplicatedStore::ClearTxEventHandlerAndBlockWrites()
    {
        txEventHandler_.reset();
        isReadOnlyForMigration_ = true;
    }

    void ReplicatedStore::OnSlowCommit()
    {
        if (healthTrackerSPtr_)
        {
            healthTrackerSPtr_->OnSlowCommit();
        }
    }

    AsyncOperationSPtr ReplicatedStore::BeginBackupLocal(
        std::wstring const & backupDirectory,
        StoreBackupOption::Enum backupOption,
        Api::IStorePostBackupHandlerPtr const & postBackupHandler,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent,
        ErrorCode createError)
    {
        return this->BeginBackupLocal(
            backupDirectory,
            backupOption,
            postBackupHandler,
            StoreBackupRequester::User,
            callback,
            parent,
            createError);
    }

    AsyncOperationSPtr ReplicatedStore::BeginBackupLocal(
        std::wstring const & backupDirectory,
        StoreBackupOption::Enum backupOption,
        Api::IStorePostBackupHandlerPtr const & postBackupHandler,
        StoreBackupRequester::Enum backupRequester,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent,
        ErrorCode createError)
    {
        WriteInfo(
            TraceComponent,
            "{0}: BeginBackupLocal: dir={1} option={2} error={3}",
            this->TraceId,            
            backupDirectory,
            backupOption,
            createError);

        return AsyncOperation::CreateAndStart<BackupAsyncOperation>(
            *this,                 
            createError,
            backupDirectory, 
            backupOption,
            postBackupHandler,
            backupRequester,
            callback, 
            parent);
    }

    ErrorCode ReplicatedStore::EndBackupLocal(
        __in AsyncOperationSPtr const & operation)
    {
        auto error = BackupAsyncOperation::End(operation);

        auto msg = wformatString("{0} EndBackupLocal: error={1}", this->TraceId, error);

        switch (error.ReadValue())
        {
        case ErrorCodeValue::Success:
        case ErrorCodeValue::ObjectClosed:
        case ErrorCodeValue::BackupInProgress:
            WriteInfo(TraceComponent, "{0}", msg);
            break;

        default:
            WriteWarning(TraceComponent, "{0}", msg);
            break;
        }

        return error;
    }

    void ReplicatedStore::LogTruncationTimerCallback()
    {
        {
            AcquireReadLock lock(dropLocalStoreLock_);

            if (this->IsLocalStoreClosed || !this->IsLocalStoreLogTruncationRequired)
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: LogTruncationBackupTimer: LocalStore is either closed or don't require log truncation. Returning.",
                    this->TraceId);

                return;
            }
        }

        this->Test_IncrementLogTruncationTimerFireCount();

        WriteInfo(TraceComponent, "{0}: LogTruncationBackupTimer: Starting TruncateLogsOnly backup.", this->TraceId);

        this->BeginBackupLocal(
            L"",
            StoreBackupOption::TruncateLogsOnly,
            Api::IStorePostBackupHandlerPtr(),
            StoreBackupRequester::LogTruncation,
            [this] (AsyncOperationSPtr const & asyncOp) { this->LogTruncationCallback(asyncOp); },
            this->Root.CreateAsyncOperationRoot());
    }
    
    void ReplicatedStore::LogTruncationCallback(AsyncOperationSPtr const & asyncOp)
    {
        auto error = this->EndBackupLocal(asyncOp);

        WriteInfo(
            TraceComponent,
            "{0}: LogTruncationBackupTimer: Finished TruncateLogsOnly backup with error=[{1}].",
            this->TraceId,
            error);
    }

    // This is called by BackupAsyncOperation, after it has finished, to re-arm the timer to fire next.
    void ReplicatedStore::ArmLogTruncationTimer()
    {
        {
            AcquireReadLock lock(dropLocalStoreLock_);

            if (this->IsLocalStoreClosed || !this->IsLocalStoreLogTruncationRequired)
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: ArmLogTruncationBackupTimer: LocalStore is either closed or don't require log truncation. Returning.",
                    this->TraceId);

                return;
            }
        }
        
        AcquireExclusiveLock lock(logTruncationTimerLock_);

        if (logTruncationTimer_)
        {
            WriteInfo(
                TraceComponent, 
                "{0}: ArmLogTruncationBackupTimer: Rearming timer. Replica Role is [{1}].",
                this->TraceId, 
                replicaRole_);
            
            logTruncationTimer_->Change(settings_.LogTruncationInterval);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: ArmLogTruncationBackupTimer: logTruncationTimer_ is not set. Replica Role is [{1}].",
                this->TraceId,
                replicaRole_);
        }
    }

    // This is called by ChangeRoleEventCallback.
    void ReplicatedStore::TryStartLogTruncationTimer(::FABRIC_REPLICA_ROLE newRole)
    {
        {
            AcquireReadLock lock(dropLocalStoreLock_);

            if (!this->IsLocalStoreLogTruncationRequired)
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: TryStartLogTruncationTimer: LocalStore don't require log truncation. Returning.",
                    this->TraceId);

                return;
            }
        }

        AcquireExclusiveLock lock(logTruncationTimerLock_);

        if (newRole == ::FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY ||
            newRole == ::FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            WriteInfo(
                TraceComponent,
                "{0}: TryStartLogTruncationTimer: Setting and arming timer. Replica Role is [{1}].",
                this->TraceId,
                replicaRole_);

            if (!logTruncationTimer_)
            {
                logTruncationTimer_ = Timer::Create(
                    TimerTagDefault,
                    [this](TimerSPtr const&) { this->LogTruncationTimerCallback(); },
                    false);
            }

            logTruncationTimer_->Change(settings_.LogTruncationInterval);
        }
    }

    // This is called by CloseEventCallback
    void ReplicatedStore::TryStopLogTruncationTimer()
    {
        WriteInfo(
            TraceComponent,
            "{0}: TryStopLogTruncationTimer: Resetting timer. Replica Role is [{1}].",
            this->TraceId,
            replicaRole_);

        AcquireExclusiveLock lock(logTruncationTimerLock_);

        if (logTruncationTimer_)
        {
            logTruncationTimer_->Cancel();
            logTruncationTimer_.reset();
        }
    }

    AsyncOperationSPtr ReplicatedStore::BeginRestoreLocal(
        std::wstring const & backupDirectory,
        RestoreSettings const & restoreSettings,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        WriteInfo(
            TraceComponent,
            "{0}: BeginRestoreLocal: dir={1}",
            this->TraceId,
            backupDirectory);

        return AsyncOperation::CreateAndStart<RestoreAsyncOperation>(
            *this,            
            backupDirectory,
            restoreSettings,
            callback,
            parent);
    }

    ErrorCode ReplicatedStore::EndRestoreLocal(
        __in AsyncOperationSPtr const & operation)
    {
        return RestoreAsyncOperation::End(operation);
    }

    ErrorCode ReplicatedStore::DisableIncrementalBackup()
    {
        return this->SetIncrementalBackupState(false, Guid::Empty(), 0);
    }

	ErrorCode ReplicatedStore::DisableIncrementalBackup(__in TransactionSPtr & txSPtr)
	{
		return this->SetIncrementalBackupState(txSPtr, false, Guid::Empty(), 0);
	}

    ErrorCode ReplicatedStore::SetIncrementalBackupState(Guid const & backupChainGuid, ULONG const & prevBackupIndex)
    {
        ASSERT_IFNOT(
            backupChainGuid != Guid::Empty(),
            "{0} ReplicatedStore::SetIncrementalBackupState: Unexpected values prevBackupIndex = {1}, backupChainfullBackupGuid = {2}.",
            this->TraceId,
            prevBackupIndex,
            backupChainGuid);

        return this->SetIncrementalBackupState(true, backupChainGuid, prevBackupIndex);
    }
    
	ErrorCode ReplicatedStore::SetIncrementalBackupState(bool allow, Guid const & backupChainGuid, ULONG const & prevBackupIndex)
	{
		TransactionSPtr txSPtr;
		auto error = this->CreateLocalStoreTransaction(txSPtr);

		if (!error.IsSuccess()) { return error; }

		return this->SetIncrementalBackupState(txSPtr, allow, backupChainGuid, prevBackupIndex);
	}

    ErrorCode ReplicatedStore::SetIncrementalBackupState(
		__in TransactionSPtr & txSPtr, 
		bool allow, 
		Guid const & backupChainGuid, 
		ULONG const & prevBackupIndex)
    {
        bool currAllowState;
        ULONG currPrevBackupIndex;
        Guid currBackupChainGuid;
        
		auto error = GetIncrementalBackupState(txSPtr, currAllowState, currBackupChainGuid, currPrevBackupIndex);
        
        if (error.IsSuccess() &&
            currAllowState == allow &&
            currBackupChainGuid == backupChainGuid &&
            currPrevBackupIndex == prevBackupIndex)
        {
            // Nothing to do. the database already contains the status we want.
            // The transaction is rolled back automatically
            return error;
        }

        bool doMoreWork = error.IsSuccess() || error.IsError(ErrorCodeValue::NotFound);
        if (!doMoreWork)
        {
            // The transaction is rolled back automatically
            return error;
        }

        // we come here when either the key is not found or when the alreadyAllowed flag is false (i.e. when the store
        // currently allows only full backups)

        if (error.IsSuccess())
        {
            // the key already exists, delete it
            error = this->LocalStore->Delete(txSPtr, Constants::LocalStoreIncrementalBackupDataType, Constants::AllowIncrementalBackupDataName, 0);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0} ReplicatedStore::SetIncrementalBackupState: failed to allow the next incremental backup. Error = {1}",
                    this->TraceId,
                    error);

                txSPtr->Rollback();
                return error;
            }
        }

        error = dataSerializerUPtr_->WriteData(
            txSPtr,
            LocalStoreIncrementalBackupData(allow, backupChainGuid, prevBackupIndex),
            Constants::LocalStoreIncrementalBackupDataType,
            Constants::AllowIncrementalBackupDataName);

        if (error.IsSuccess())
        {
            error = txSPtr->Commit();
            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0} ReplicatedStore::SetIncrementalBackupState: Next incremental backup: {1}, BackupChainGuid: {2} PrevBackupIndex: {3}",
                    this->TraceId,
                    allow ? L"allowed" : L"disallowed",
                    backupChainGuid,
                    prevBackupIndex);
            }
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0} ReplicatedStore::SetIncrementalBackupState: failed to update incremental backup data. Allow: {1}, BackupChainGuid: {2} PrevBackupIndex: {3}. Error = {4}",                 
                this->TraceId, 
                allow ? L"allow" : L"disallow",
                backupChainGuid,
                prevBackupIndex,
                error);
            
            txSPtr->Rollback();
        }

        return error;
    }

    ErrorCode ReplicatedStore::GetIncrementalBackupState(
        __in TransactionSPtr & txSPtr, 
        __out bool & allowed,
        __out Guid & backupChainFullBackupGuid,
        __out ULONG & prevBackupIndex)
    {
        allowed = false;
        backupChainFullBackupGuid = Guid::Empty();
        prevBackupIndex = 0;
        
        LocalStoreIncrementalBackupData incrementalBackupData;

        auto error = dataSerializerUPtr_->TryReadData(
            txSPtr,
            Constants::LocalStoreIncrementalBackupDataType,
            Constants::AllowIncrementalBackupDataName,
            incrementalBackupData);

        if (error.IsSuccess())
        {
            allowed = incrementalBackupData.IsIncrementalBackupAllowed;
            backupChainFullBackupGuid = incrementalBackupData.BackupChainGuid;
            prevBackupIndex = incrementalBackupData.PreviousBackupIndex;
        }
        else if (!error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceComponent,
                "{0} ReplicatedStore::GetIncrementalBackupStateFromStore: failed to read data from local store. Error = {1}",
                this->TraceId,
                error);
        }

        return error;
    }

    shared_ptr<ReplicatedStore::TransactionReplicator> ReplicatedStore::TryGetTxReplicator() const
    {
        return txReplicatorWPtr_.lock();
    }
    
    ErrorCode ReplicatedStore::CheckAccess(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS status)
    {
        ErrorCode error;
        switch (status)
        {
            case ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED:
                error = ErrorCodeValue::Success;
                break;

            case ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING:
                error = ErrorCodeValue::ReconfigurationPending;
                break;

            case ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY:
                error = ErrorCodeValue::NotPrimary;
                break;

            case ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM:
                error = ErrorCodeValue::NoWriteQuorum;
                break;

            default:
                Assert::CodingError("Unrecognized access status: {0}", static_cast<int>(status));
        }

        return error;
    }

    ErrorCode ReplicatedStore::CheckWriteAccess(
        TransactionSPtr const & txSPtr,
        wstring const & type)
    {
        if (isReadOnlyForMigration_) { return ErrorCodeValue::DatabaseMigrationInProgress; }

        auto error = txSPtr ? txSPtr->CheckAborted() : ErrorCodeValue::Success;
        if (!error.IsSuccess()) { return error; }

        error = this->CheckTypeAccess(type);
        
        if (error.IsSuccess())
        {
            ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS status;
            HRESULT hr = partitionCPtr_->GetWriteStatus(&status);
            if (SUCCEEDED(hr))
            {
                error = this->CheckAccess(status);
            }
            else
            {
                error = ErrorCode::FromHResult(hr);
            }
        }

        if (!error.IsSuccess() && txSPtr)
        {
            txSPtr->Abort();
        }

        return error;
    }

    ErrorCode ReplicatedStore::CheckReadAccess(
        TransactionSPtr const & txSPtr, 
        wstring const & type)
    {
        auto error = txSPtr ? txSPtr->CheckAborted() : ErrorCodeValue::Success;

        if (!error.IsSuccess()) { return error; }

        error = this->CheckTypeAccess(type);

        if (error.IsSuccess())
        {
            ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS status;
            HRESULT hr = partitionCPtr_->GetReadStatus(&status);
            if (SUCCEEDED(hr))
            {
                error = this->CheckAccess(status);
            }
            else
            {
                error = ErrorCode::FromHResult(hr);
            }
        }

        if (!error.IsSuccess() && txSPtr)
        {
            txSPtr->Abort();
        }

        return error;
    }

    ErrorCode ReplicatedStore::CheckTypeAccess(wstring const & type)
    {
        if (!type.empty() && IsStoreMetadataType(type))
        { 
            WriteError(TraceComponent, "data type = '{0}' is reserved", type);

            return ErrorCodeValue::InvalidArgument;
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }

    ErrorCode ReplicatedStore::FinalizeTombstone(
        TransactionSPtr const & txSPtr,
        wstring const & type,
        wstring const & key,
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        size_t tombstoneIndex,
        __out bool & shouldCountTombstone)
    {
        if (!settings_.EnableTombstoneCleanup2)
        {
            return this->UpdateTombstone1(
                txSPtr,
                type,
                key,
                operationLsn,
                shouldCountTombstone);
        }
        else
        {
            shouldCountTombstone = true;

            return this->UpdateTombstone2(
                txSPtr,
                type,
                key,
                operationLsn,
                tombstoneIndex);
        }
    }

    // Tombstone creation corresponding to version 1 tombstone cleanup algorithm.
    //
    ErrorCode ReplicatedStore::UpdateTombstone1(
        TransactionSPtr const & txSPtr,
        wstring const & type, 
        wstring const & key, 
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        __out bool & countTombstone)
    {
        wstring unusedKey;
        return UpdateTombstone1(txSPtr, type, key, operationLsn, countTombstone, unusedKey);
    }

    ErrorCode ReplicatedStore::UpdateTombstone1(
        TransactionSPtr const & txSPtr,
        wstring const & type, 
        wstring const & key, 
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        __out bool & countTombstone,
        __out wstring & tombstoneKey)
    {
        countTombstone = false;

        tombstoneKey = CreateTombstoneKey1(type, key);

        // The tombstone may already exist from a previous deletion
        // (e.g. insert, delete, insert). Since we do not remove old
        // tombstones on every insert (for performance), we need to check for
        // it here. Furthermore, this implies that secondaries may get both a
        // tombstone and live entry for the same data item during copy and must resolve the
        // conflict by comparing sequence numbers.
        //
        ::FABRIC_SEQUENCE_NUMBER existingLsn = -1;
        auto error = this->DataExists(txSPtr, Constants::TombstoneDataType, tombstoneKey, existingLsn);

        if (error.IsSuccess())
        {
            WriteNoise(
                TraceComponent, 
                "{0} UpdateTombstone1({1}, {2}, {3}, {4}): count={5}",
                this->TraceId,
                type,
                key,
                existingLsn,
                operationLsn,
                tombstoneCount_.load());

            if (operationLsn > 0 && operationLsn <= existingLsn)
            {
                // operationLsn is already uptodate
                WriteInfo(
                    TraceComponent, 
                    "{0} UpdateTombstone1 - current lsn {1} is up to date: ({2}, {3}, {4})",
                    this->TraceId,
                    existingLsn,
                    type,
                    key,
                    operationLsn);
                return error;
            }

            if (existingLsn == 0)
            {
                countTombstone = true;
            }

            error = this->LocalStore->UpdateOperationLSN(
                txSPtr,
                Constants::TombstoneDataType,
                tombstoneKey,
                (operationLsn > 0 ? operationLsn : existingLsn));
        }
#if defined(PLATFORM_UNIX)
        if (error.IsError(ErrorCodeValue::NotFound) || error.IsError(ErrorCodeValue::StoreRecordNotFound))
#else
        else if (error.IsError(ErrorCodeValue::NotFound))
#endif
        {
            WriteNoise(
                TraceComponent, 
                "{0} InsertTombstone1({1}, {2}, {3})",
                this->TraceId,
                type,
                key,
                operationLsn);

            if (operationLsn > 0)
            {
                countTombstone = true;
            }

            // EseLocalStore does not support NULL pointers
            // for the value or empty byte arrays, 
            // so insert a single '0' byte for NULL
            //
            vector<byte> bytes;
            bytes.push_back(0);
            error = this->LocalStore->Insert(
                txSPtr,
                Constants::TombstoneDataType,
                tombstoneKey,
                bytes.data(),
                bytes.size(),
                operationLsn);

            //
            // If we persist the tombstone count, then we cannot have
            // concurrent transactions containing deletes since they
            // would conflict when attempting to update the count.
            // Keep the count in memory and recover during open instead.
            //
            // We can update the count here, but it's possible for the
            // prune process to read the count before the tombstone is
            // actually committed. While this is functionally okay, it
            // can result in many small prunes when there are many
            // concurrent delete operations. We can optimize this
            // scenario by updating the count only after commit.
            //
            // A newly inserted tombstone always starts at LSN 0 on the primary
            // and we account for the tombstone only after successfully
            // updating its LSN. This is to keep the tombstone count
            // estimate in sync with the actual number of tombstones.
            // On the secondary, a tombstone will be inserted with its actual
            // LSN.
            //
        }

        return error;
    }

    // Tombstone creation corresponding to version 2 tombstone cleanup.
    //
    ErrorCode ReplicatedStore::UpdateTombstone2(
        TransactionSPtr const & txSPtr,
        wstring const & type, 
        wstring const & key, 
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        size_t index)
    {
        wstring unusedKey;
        return UpdateTombstone2(txSPtr, type, key, operationLsn, index, unusedKey);
    }

    ErrorCode ReplicatedStore::UpdateTombstone2(
        TransactionSPtr const & txSPtr,
        wstring const & type, 
        wstring const & key, 
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        size_t index,
        __out wstring & tombstoneKey)
    {
        // LNK2001: unresolved external symbol for get_Type() override without pointer
        //
        auto tombstone = make_unique<TombstoneData>(type, key, operationLsn, index);

        tombstoneKey = tombstone->Key;

        vector<byte> buffer;
        auto error = FabricSerializer::Serialize(tombstone.get(), buffer);
        if (!error.IsSuccess()) { return error; }

        EnumerationSPtr enumSPtr;
        error = this->LocalStore->CreateEnumerationByTypeAndKey(
            txSPtr, 
            tombstone->Type, 
            tombstone->Key, 
            enumSPtr);
        if (!error.IsSuccess()) { return error; }

        wstring existingKey;
        error = enumSPtr->MoveNext();
        if (error.IsSuccess()) 
        {
            error = enumSPtr->CurrentKey(existingKey);
            if (!error.IsSuccess()) { return error; }

            if (existingKey != tombstone->Key)
            {
                error = ErrorCodeValue::StoreRecordNotFound;
            }
        }
        else if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            error = ErrorCodeValue::StoreRecordNotFound;
        }

        if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
        {
            WriteNoise(
                TraceComponent, 
                "{0} InsertTombstone2: {1}",
                this->TraceId,
                *tombstone);

            error = this->LocalStore->Insert(
                txSPtr,
                tombstone->Type, 
                tombstone->Key, 
                buffer.data(), 
                buffer.size(),
                operationLsn);
        }
        else if (error.IsSuccess())
        {
            // Typically, we will be inserting a brand new tombstone. The uncommon scenario
            // where the tombstone already exists is if it was transferred via a copy operation
            // before showing up in a replication operation - though this also typically should
            // not be the case.
            //
            // Validate the existing tombstone and overwrite if we ever find one that
            // already exists.
            //
            vector<byte> existingBuffer;
            error = enumSPtr->CurrentValue(existingBuffer);
            if (!error.IsSuccess()) { return error; }

            FABRIC_SEQUENCE_NUMBER existingLsn;
            error = enumSPtr->CurrentOperationLSN(existingLsn);
            if (!error.IsSuccess()) { return error; }

            TombstoneData existingTombstone;
            error = FabricSerializer::Deserialize(existingTombstone, existingBuffer);
            if (!error.IsSuccess()) { return error; }

            existingTombstone.SetSequenceNumber(existingLsn);

            if (*tombstone != existingTombstone || operationLsn != existingLsn)
            {
                TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} tombstones do not match: new={1} ({2}) existing={3} ({4})",
                    this->TraceId,
                    *tombstone,
                    operationLsn,
                    existingTombstone,
                    existingLsn);
            }

            WriteInfo(
                TraceComponent, 
                "{0} UpdateTombstone2: new={1} existing={2}",
                this->TraceId,
                *tombstone,
                existingTombstone);

            error = this->LocalStore->Update(
                txSPtr,
                tombstone->Type,
                tombstone->Key,
                existingLsn,
                tombstone->Key,
                buffer.data(),
                buffer.size(),
                operationLsn);
        }

        return error;
    }

    void ReplicatedStore::ScheduleTombstoneCleanupIfNeeded(size_t addedTombstonesCount)
    {
        this->IncrementTombstoneCount(addedTombstonesCount);

        // Optimization: When there are many delete operations, only
        // allow one active tombstone cleanup thread. Otherwise, they
        // may pile up waiting for dropLocalStoreLock_.
        //
        shared_ptr<ScopedActiveFlag> scopedActiveFlag;
        if (ScopedActiveFlag::TryAcquire(tombstoneCleanupActive_, scopedActiveFlag))
        {
            auto root = this->Root.CreateComponentRoot();
            Threadpool::Post([this, root, scopedActiveFlag, addedTombstonesCount]
            {
                this->CleanupTombstones(addedTombstonesCount);
            });
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} tombstone cleanup already scheduled or active: new={1}",
                this->TraceId,
                addedTombstonesCount);
        }
    }

    void ReplicatedStore::CleanupTombstones(size_t addedTombstonesCount)
    {
        if (!test_TombstoneCleanupEnabled_)
        {
            WriteInfo(
                TraceComponent, 
                "{0} tombstone cleanup disabled",
                this->TraceId);

            return;
        }

        // Optimization: If we are performing any copies, then don't
        // bother enumerating tombstones for removal. Defer tombstone
        // cleanup until after the copies complete.
        // Note that even if this pre-emptive check passes, we still
        // need to perform the check later before actually committing
        // any tombstone removal changes.
        //
        {
            AcquireExclusiveLock lock(lowWatermarkLock_);

            if (lowWatermarkReaderCount_ > 0)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} skipping tombstone cleanup: {1} active copies",
                    this->TraceId,
                    lowWatermarkReaderCount_);

                return;
            }
        }

        auto tombstoneCount = tombstoneCount_.load();
        auto cleanupLimit = StoreConfig::GetConfig().TombstoneCleanupLimit;

        if (tombstoneCount <= cleanupLimit)
        {
            WriteNoise(
                TraceComponent, 
                "{0} skip tombstone cleanup: estimated={1} <= limit={2}: new={3}", 
                this->TraceId, 
                tombstoneCount, 
                cleanupLimit,
                addedTombstonesCount);

            return;
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} starting tombstone cleanup: estimated={1} > limit={2}: new={3}", 
                this->TraceId, 
                tombstoneCount, 
                cleanupLimit, 
                addedTombstonesCount);
        }

        TransactionSPtr txSPtr;
        auto error = this->CreateLocalStoreTransaction(txSPtr);
        if (!error.IsSuccess()) 
        { 
            WriteInfo(TraceComponent, "{0} failed to create tombstone cleanup transaction: error = {1}", this->TraceId, error);
            return; 
        }

        EnumerationSPtr enumSPtr;
        error = this->LocalStore->CreateEnumerationByTypeAndKey(txSPtr, Constants::TombstoneDataType, L"", enumSPtr);
        if (!error.IsSuccess()) 
        { 
            WriteInfo(TraceComponent, "{0} failed to create tombstone cleanup enumeration: error = {1}", this->TraceId, error);
            return; 
        }

        Stopwatch stopwatch;
        stopwatch.Start();

        ::FABRIC_SEQUENCE_NUMBER lastRemovedLsn = 0;
        size_t removedCount = 0;

        if (!settings_.EnableTombstoneCleanup2)
        {
            error = CleanupTombstones1(txSPtr, enumSPtr, removedCount, lastRemovedLsn);
        }
        else
        {
            error = CleanupTombstones2(txSPtr, enumSPtr, removedCount, lastRemovedLsn);
        }

        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} tombstone cleanup failed: error={1}", this->TraceId, error); 
            return;
        }

        if (removedCount == 0)
        {
            WriteInfo(
                TraceComponent, 
                "{0} no tombstones were removed",
                this->TraceId); 
            return;
        }

        error = dataSerializerUPtr_->WriteData(
            txSPtr, 
            TombstoneLowWatermarkData(lastRemovedLsn),
            Constants::ProgressDataType, 
            Constants::TombstoneLowWatermarkDataName);

        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} failed to update tombstone low watermark: error={1}", this->TraceId, error); 
            return;
        }

        // Cannot persist tombstone pruning changes to disk until
        // all copy operations that may need to read the pruned tombstones
        // are complete since we will be changing the low watermark.
        //
        AcquireExclusiveLock lock(lowWatermarkLock_);

        if (0 == lowWatermarkReaderCount_)
        {
            error = txSPtr->Commit();

            if (!error.IsSuccess())
            {
                WriteInfo(TraceComponent, "{0} failed to commit tombstone cleanup: error = {1}", this->TraceId, error);
                return;
            }

            stopwatch.Stop();

            WriteInfo(
                TraceComponent, 
                "{0} tombstone cleanup complete: removed={1} low watermark={2} elapsed={3}",
                this->TraceId, 
                removedCount,
                lastRemovedLsn,
                stopwatch.Elapsed);

            this->DecrementTombstoneCount(removedCount);
        }
        else
        {
            txSPtr->Rollback();

            stopwatch.Stop();

            WriteInfo(
                TraceComponent, 
                "{0} rollback tombstone cleanup due to {1} pending partial copies: elapsed={2}",
                this->TraceId, 
                lowWatermarkReaderCount_,
                stopwatch.Elapsed);
        }

        this->PerfCounters.AvgLatencyOfTombstoneCleanupBase.Increment();
        this->PerfCounters.AvgLatencyOfTombstoneCleanup.IncrementBy(stopwatch.ElapsedMilliseconds);
    }

    typedef std::pair<wstring, ::FABRIC_SEQUENCE_NUMBER> TombstoneTuple;
    bool TombstoneSequenceNumberLessThan(TombstoneTuple const & t1, TombstoneTuple const & t2)
    {
        return (t1.second < t2.second);
    }

    // Version1 tombstone cleanup - legacy comment:
    //
    // Since the prune process restricts the number of tombstones kept in the store,
    // it will generally be more efficient to read all tombstones into memory and sort them
    // rather than reading the entire database in sequence number order looking for tombstones.
    // (This assumes that there is much more user data than tombstones).
    //
    // The one issue is that if any partial copy takes a long time and during that time, lots of
    // new tombstones are created, then the next prune will have to load a large number of
    // tombstones into memory. Ideally, if the local store supported multiple tables per local store 
    // instance, then we could create a table just for the tombstones and enumerate only tombstones in
    // sequence number order.
    //
    ErrorCode ReplicatedStore::CleanupTombstones1(
        TransactionSPtr const & txSPtr, 
        EnumerationSPtr const & enumSPtr,
        __out size_t & removedCount,
        __out FABRIC_SEQUENCE_NUMBER & lastRemovedLsn)
    {
        ErrorCode error;
        vector<TombstoneTuple> tombstones;

        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            wstring key;
            error = enumSPtr->CurrentKey(key);
            if (!error.IsSuccess()) { break; }

            ::FABRIC_SEQUENCE_NUMBER operationLSN = 0;
            error = enumSPtr->CurrentOperationLSN(operationLSN);
            if (!error.IsSuccess()) { break; }

            tombstones.push_back(TombstoneTuple(key, operationLSN));
        }

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            error = ErrorCodeValue::Success;
        }

        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0} failed to read tombstones: error = {1}", this->TraceId, error);
            return error;
        }

        if (tombstones.empty())
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} tombstones empty: skipping tombstone cleanup", this->TraceId);

            return ErrorCodeValue::OperationFailed;
        }

        auto cleanupTarget = tombstones.size() / 2;
        auto maxTarget = StoreConfig::GetConfig().MaxTombstonesPerCleanup;
        auto cleanupLimit = StoreConfig::GetConfig().TombstoneCleanupLimit;

        if (cleanupTarget > maxTarget)
        {
            cleanupTarget = maxTarget;
        }

        if (tombstones.size() <= cleanupLimit)
        {
            WriteInfo(TraceComponent, "{0} skipping tombstone cleanup: actual={1} limit={2}", this->TraceId, tombstones.size(), cleanupLimit);

            this->SetTombstoneCount(tombstones.size());

            return ErrorCodeValue::Success;
        }
        else
        {
            WriteInfo(
                TraceComponent, 
                "{0} running tombstone cleanup version 1: actual={1} target={2} max={3}", 
                this->TraceId, 
                tombstones.size(), 
                cleanupTarget,
                maxTarget);
        }

        std::sort(tombstones.begin(), tombstones.end(), TombstoneSequenceNumberLessThan);

        removedCount = 0;
        lastRemovedLsn = 0;

        for (size_t ix = 0; ix < tombstones.size(); ++ix)
        {
            wstring const & key = std::get<0>(tombstones[ix]);
            ::FABRIC_SEQUENCE_NUMBER tombstoneLsn = std::get<1>(tombstones[ix]);

            // The last tombstone may be the data item marking the current progress (highest LSN).
            // Conservatively, don't remove the last tombstone since the replicator assumes that 
            // GetLastCommittedSequenceNumber() must return the last quorum-acked LSN.
            //
            if (ix == tombstones.size() - 1)
            {
                TRACE_ERROR_AND_TESTASSERT(
                    TraceComponent,
                    "{0} stop tombstone cleanup at current progress: key = '{1}' lsn = {2} found = {3}",
                    this->TraceId, 
                    key,
                    tombstoneLsn,
                    tombstones.size());
                break;
            }

            error = this->LocalStore->Delete(txSPtr, Constants::TombstoneDataType, key, 0);

            if (!error.IsSuccess())
            {
                // This is non-fatal since we can try again on the next prune event
                WriteInfo(
                    TraceComponent, 
                    "{0} cancel tombstone cleanup on error: key = '{1}' error = {2}",
                    this->TraceId, 
                    key,
                    error);
                return error;
            }

            // Once we've deleted at least one tombstone with sequence number N,
            // we must full copy to any secondaries who are not already caught up to N.
            // (i.e. if the secondary's progress is less than N)
            //
            lastRemovedLsn = tombstoneLsn;

            if (++removedCount >= cleanupTarget)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} stopping tombstone cleanup at target: key={1} lsn={2}", 
                    this->TraceId, 
                    key,
                    tombstoneLsn);
                break;
            }

        } // end for sorted tombstones

        return error;
    }

    // Version 2 tombstone cleanup:
    //
    // Tombstone entries are named using the zero padded string representation of their LSNs.
    // This allows us to enumerate tombstones in non-decreasing LSN order, which effectively
    // only scans the tombstones we intend to cleanup.
    //
    // Each tombstone entry additionally has an index appended to its name that's incremented
    // for each delete operation occurring within the same transaction since all such
    // tombstones will have the same LSN. The index is only a mechanism for creating unique
    // key names for deletes with the same LSN and is not used in any other way.
    //
    ErrorCode ReplicatedStore::CleanupTombstones2(
        TransactionSPtr const & txSPtr, 
        EnumerationSPtr const & enumSPtr,
        __out size_t & removedCount,
        __out FABRIC_SEQUENCE_NUMBER & lastRemovedLsn)
    {
        ErrorCode error;
        auto cleanupTarget = tombstoneCount_.load() / 2;
        auto maxTarget = StoreConfig::GetConfig().MaxTombstonesPerCleanup;

        if (cleanupTarget > maxTarget)
        {
            cleanupTarget = maxTarget;
        }

        WriteInfo(
            TraceComponent, 
            "{0} running tombstone cleanup: target={1} max={2}", 
            this->TraceId, 
            cleanupTarget,
            maxTarget);
            
        removedCount = 0;
        lastRemovedLsn = 0;

        wstring previousTombstoneKey;
        FABRIC_SEQUENCE_NUMBER previousTombstoneLsn = 0;

        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            wstring tombstoneKey;
            error = enumSPtr->CurrentKey(tombstoneKey);
            if (!error.IsSuccess()) { break; }

            ::FABRIC_SEQUENCE_NUMBER tombstoneLsn = 0;
            error = enumSPtr->CurrentOperationLSN(tombstoneLsn);
            if (!error.IsSuccess()) { break; }

            // Do not delete the last remaining tombstone since it may be the
            // highest LSN entry tracking the progress of this replica.
            // We could also be more strict and check if the last tombstone's
            // LSN actually matches the progress LSN, but there's not much
            // to gain from such an optimization.
            //
            if (!previousTombstoneKey.empty())
            {
                error = this->LocalStore->Delete(txSPtr, Constants::TombstoneDataType, previousTombstoneKey, 0);
                if (!error.IsSuccess()) { break; }

                lastRemovedLsn = previousTombstoneLsn;

                if (++removedCount >= cleanupTarget)
                {
                    WriteInfo(
                        TraceComponent, 
                        "{0} stopping tombstone cleanup (inclusive): key={1} lsn={2}", 
                        this->TraceId, 
                        previousTombstoneKey,
                        previousTombstoneLsn);
                    break;
                }
            }

            previousTombstoneKey = tombstoneKey;
            previousTombstoneLsn = tombstoneLsn;
        }

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            error = ErrorCodeValue::Success;
        }

        return error;
    }

    ErrorCode ReplicatedStore::GetTombstoneLowWatermark(__out TombstoneLowWatermarkData & result)
    {
        return this->GetTombstoneLowWatermark(nullptr, result);
    }

    ErrorCode ReplicatedStore::GetTombstoneLowWatermark(TransactionSPtr const & inTxSPtr, __out TombstoneLowWatermarkData & result)
    {
        TransactionSPtr txSPtr = inTxSPtr;

        if (!txSPtr)
        {
            auto error = this->LocalStore->CreateTransaction(txSPtr);
            if (!error.IsSuccess()) { return error; }
        }

        TombstoneLowWatermarkData lowWatermark;
        auto error = dataSerializerUPtr_->TryReadData(
            txSPtr, 
            Constants::ProgressDataType, 
            Constants::TombstoneLowWatermarkDataName, 
            lowWatermark);

        if (error.IsSuccess())
        {
            result = lowWatermark;
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::Success;
        }

        return error;
    }

    ErrorCode ReplicatedStore::DataExists(
        TransactionSPtr const & txSPtr, 
        wstring const & type, 
        wstring const & key,
        __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
    {
        auto error = this->LocalStore->GetOperationLSN(txSPtr, type, key, operationLSN);

        if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
        {
            error = ErrorCodeValue::NotFound;
        }

        return error;
    }

    bool ReplicatedStore::TryCompletePendingChangeRoleOperation(ErrorCode const & error)
    {
        // Only called with state machine lock held, 
        // so this function does not need to be threadsafe
        //
        if (pendingChangeRoleOperation_)
        {
            auto snap = pendingChangeRoleOperation_;
            Threadpool::Post([snap, error]() { snap->TryComplete(snap, error); });
            pendingChangeRoleOperation_.reset();

            return true;
        }

        return false;
    }

    bool ReplicatedStore::TryCompletePendingCloseOperation(ErrorCode const & error)
    {
        // Only called with state machine lock held, 
        // so this function does not need to be threadsafe
        //
        if (pendingCloseOperation_)
        {
            auto snap = pendingCloseOperation_;
            Threadpool::Post([snap, error]() { snap->TryComplete(snap, error); });
            pendingCloseOperation_.reset();

            return true;
        }

        return false;
    }

    bool ReplicatedStore::TryStartSecondaryPump()
    {
        if (!secondaryPumpSPtr_)
        {
            secondaryPumpSPtr_ = make_shared<SecondaryPump>(*this, stateReplicatorCPtr_);
            secondaryPumpSPtr_->Start();

            return true;
        }

        return false;
    }

    bool ReplicatedStore::TryCancelSecondaryPump()
    {
        if (secondaryPumpSPtr_)
        {
            secondaryPumpSPtr_->Cancel();
            secondaryPumpSPtr_.reset();

            return true;
        }

        return false;
    }

    ErrorCode ReplicatedStore::InitializePrimaryProcessing()
    {
        ::FABRIC_SEQUENCE_NUMBER lastCommittedLsn = 0;
        auto error = this->GetLastCommittedSequenceNumber(lastCommittedLsn);

        if (error.IsSuccess())
        {
            // TxReplicator protected by state machine
            //
            txReplicatorSPtr_->Initialize(lastCommittedLsn);
            TryStartFabricTimer();
        }

        return error;
    }

    void ReplicatedStore::TryUninitializePrimaryProcessing()
    {
        auto txReplicator = this->TryGetTxReplicator();

        if (txReplicator)
        {
            txReplicator->TryUnInitialize();
        }
        TryCancelFabricTimer();
    }

    void ReplicatedStore::StartPrimaryPendingTimer()
    {
        txTrackerUPtr_->StartDrainTimer();
    }

    void ReplicatedStore::CancelPrimaryPendingTimer()
    {
        txTrackerUPtr_->CancelDrainTimer();
    }

    void ReplicatedStore::Cleanup()
    {
        WriteInfo(
            TraceComponent, 
            "{0} Cleaning up store",
            this->TraceId);

        if (isRoleSet_ && replicaRole_ == ::FABRIC_REPLICA_ROLE_NONE)
        {
            this->CleanupLocalStore();
        }

        // release potential StatefulServiceBase circular reference
        //
        storeEventHandler_ = Api::IStoreEventHandlerPtr();
        secondaryEventHandler_ = Api::ISecondaryEventHandlerPtr();

        // Replicator maintains references to this store via 
        // the state provider interface. Release replicator
        // to guarantee breaking the circular reference regardless
        // of whether or not the replicator releases its state provider
        // references on close.
        //
        internalStateReplicatorCPtr_.SetNoAddRef(NULL);
        stateReplicatorCPtr_.SetNoAddRef(NULL);
        
        // There can be outstanding transactions after close if 
        // TransactionDrainTimeout > 0, which enables forcefully aborting 
        // transactions on primary demotion. Outstanding transactions held
        // by the application will keep TransactionReplicator references.
        //
        auto txReplicator = this->TryGetTxReplicator();

        if (txReplicator)
        {
            txReplicator->Cleanup();
        }

        txReplicatorSPtr_.reset();
    }
    
    // ******************** 
    // StateMachine support
    // ******************** 

    void ReplicatedStore::FinishSecondaryPump()
    {
        stateMachineUPtr_->SecondaryPumpClosedEvent(
            [this](ErrorCode const & error, ReplicatedStoreState::Enum state)
            {
                this->SecondaryPumpClosedEventCallback(error, state);
            });
    }

    void ReplicatedStore::SecondaryPumpClosedEventCallback(ErrorCode const & stateMachineError, ReplicatedStoreState::Enum state)
    {
        ErrorCode error = stateMachineError;

        WriteInfo(
            TraceComponent, 
            "{0} SecondaryPumpClosedEventCallback(): state = {1} error = {2}", 
            this->TraceId,
            state,
            error);

        if (error.IsSuccess())
        {
            switch (state)
            {
                case Store::ReplicatedStoreState::Closed:
                    Cleanup();
                    // intentional fall-through

                case Store::ReplicatedStoreState::SecondaryPassive:
                case Store::ReplicatedStoreState::PrimaryPassive:
                    // release reference
                    TryCancelSecondaryPump();

                    if (state == Store::ReplicatedStoreState::PrimaryPassive)
                    { 
                        error = InitializePrimaryProcessing();
                    }

                    TryCompletePendingChangeRoleOperation(error);
                    TryCompletePendingCloseOperation(error);
                    break;

                default:
                    Assert::CodingError(
                        "{0} StateMachine::SecondaryPumpClosedEvent returned invalid state = {1}", 
                        this->TraceId, 
                        state);
            }
        }
    }

    void ReplicatedStore::FinishTransaction()
    {
        stateMachineUPtr_->FinishTransactionEvent(
            [this](ErrorCode const & error, ReplicatedStoreState::Enum state)
            {
                this->FinishTransactionEventCallback(error, state);
            });
    }
    
    void ReplicatedStore::UntrackAndFinishTransaction(TransactionBase const & tx)
    {
        txTrackerUPtr_->Remove(tx);

        this->FinishTransaction();
    }

    void ReplicatedStore::FinishTransactionEventCallback(ErrorCode const & error, ReplicatedStoreState::Enum state)
    {
        ASSERT_IFNOT(
            error.IsSuccess(),
            "{0} Statemachine failed to finish transaction: error = {1}", 
            this->TraceId, 
            error);
        
        switch (state)
        {
            case Store::ReplicatedStoreState::PrimaryActive:
            case Store::ReplicatedStoreState::PrimaryChangePending:
            case Store::ReplicatedStoreState::PrimaryClosePending:
                // intentional no-op (not last transaction)
                break;

            case Store::ReplicatedStoreState::SecondaryActive:
                CancelPrimaryPendingTimer();
                TryStartSecondaryPump();
                // intentional fall-through to complete pending change role operation
                
            case Store::ReplicatedStoreState::PrimaryPassive:
                TryCompletePendingChangeRoleOperation(error);
                break;

            case Store::ReplicatedStoreState::Closed:
                Cleanup();
                CancelPrimaryPendingTimer();
                TryCompletePendingChangeRoleOperation(error);
                TryCompletePendingCloseOperation(error);
                TryUninitializePrimaryProcessing();
                break;

            default:
                Assert::CodingError(
                    "{0} StateMachine::FinishTransactionEvent returned invalid state = {1}", 
                    this->TraceId, 
                    state);
        }
    }

    // The following state machine callback functions occur under the state machine lock, so
    // avoid calling out to any user code that could re-enter the state machine (e.g. calling 
    // CreateTransaction() again)
    //
    void ReplicatedStore::OpenEventCallback(
        AsyncOperationSPtr const & outerOperation,
        ErrorCode const & stateMachineError,
        ReplicatedStoreState::Enum state,
        bool databaseShouldExist)
    {
        auto error = stateMachineError;

        WriteInfo(
            TraceComponent, 
            "{0} OpenEventCallback(): state = {1} error = {2}", 
            this->TraceId,
            state,
            error);

        if (error.IsSuccess())
        {
            if (StoreConfig::GetConfig().OpenLocalStoreThrottle > 0)
            {
                auto operation = OpenLocalStoreJobQueue::GetGlobalQueue().BeginEnqueueOpenLocalStore(
                    *this,
                    databaseShouldExist,
                    [this](AsyncOperationSPtr const & operation) { this->OnOpenLocalStoreAsyncComplete(operation, false); },
                    outerOperation);
                this->OnOpenLocalStoreAsyncComplete(operation, true); 
            }
            else
            {
                error = this->InitializeLocalStore(databaseShouldExist);

                this->OnOpenLocalStoreSyncComplete(outerOperation, error);
            }
        }
        else
        {
            this->PostRepairOpen(outerOperation, error);
        }
    }

    void ReplicatedStore::OnOpenLocalStoreAsyncComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = OpenLocalStoreJobQueue::GetGlobalQueue().EndEnqueueOpenLocalStore(operation);

        this->OnOpenLocalStoreSyncComplete(operation->Parent, error);
    }

    void ReplicatedStore::OnOpenLocalStoreSyncComplete(
        AsyncOperationSPtr const & outerOperation,
        ErrorCode const & openLocalStoreError)
    {
        ErrorCode error = openLocalStoreError;

        if (error.IsSuccess())
        {
            error = this->PreRepairOpen();
        }

        if (error.IsSuccess())
        {
            this->PostRepairOpen(outerOperation, error);
        }
        else
        {
            auto operation = AsyncOperation::CreateAndStart<OpenRepairAsyncOperation>(
                *this,
                error,
                [this](AsyncOperationSPtr const & operation) { this->OnOpenRepairCompleted(operation, false); },
                outerOperation);
            this->OnOpenRepairCompleted(operation, true);
        }
    }

    void ReplicatedStore::OnOpenRepairCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & outerOperation = operation->Parent;

        auto error = OpenRepairAsyncOperation::End(operation);

        if (!error.IsSuccess() && healthTrackerSPtr_)
        {
            if (error.IsError(ErrorCodeValue::PathTooLong))
            {
                healthTrackerSPtr_->ReportErrorDetails(error);
            }
            else
            {
                auto details = wformatString(GET_STORE_RC(Open_Failed), error.Message.empty() ? error.ErrorCodeValueToString() : error.Message);
                healthTrackerSPtr_->ReportErrorDetails(ErrorCode(error.ReadValue(), move(details)));
            }
        }

        this->PostRepairOpen(outerOperation, error);
    }

    // Open tasks to perform that may require repair attempts on failure.
    //
    ErrorCode ReplicatedStore::PreRepairOpen()
    {
        auto healthError = HealthTracker::Create(*this, partitionCPtr_, healthTrackerSPtr_);

        if (!healthError.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} HealthTracker::Create failed: error={1}",
                this->TraceId,
                healthError);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} HealthTracker::Create succeeded",
                this->TraceId);
        }

        auto error = this->RecoverPartialCopy();

        if (error.IsSuccess())
        {
            error = this->CheckPartialCopyReplayCompleted();
        }

        if (error.IsSuccess())
        {
            auto retryCount = StoreConfig::GetConfig().DeleteDatabaseRetryCount;
            do
            {
                error = this->DeleteFullCopyDatabases();

                if (!error.IsSuccess() && retryCount > 1)
                {
                    WriteInfo(TraceComponent, "{0} retrying DeleteFullCopyDatabases: error={1}", this->TraceId, error);
                    Sleep(StoreConfig::GetConfig().DeleteDatabaseRetryDelayMilliseconds);
                }
            } while (!error.IsSuccess() && --retryCount > 0);

            if (error.IsWin32Error(ERROR_SHARING_VIOLATION) || error.IsError(ErrorCodeValue::SharingAccessLockViolation))
            {
                // A long-running file stream full copy for the same replica may have 
                // temporary database files locked for backup or archival. Since reporting 
                // fault will not help, unblock the open by taking down the process via
                // self-repair.
                //
                error = ErrorCodeValue::StoreInUse;
            }
        }

        return error;
    }

    // Open tasks to perform after repair attempts (if any)
    // have already occurred.
    //
    void ReplicatedStore::PostRepairOpen(
        AsyncOperationSPtr const & outerOperation,
        ErrorCode const & previousError)
    {
        ErrorCode error = previousError;

        if (error.IsSuccess())
        {
            error = this->LoadCachedCurrentEpoch();
        }

        if (error.IsSuccess())
        {
            error = this->LoadCopyOperationSizeFromReplicator();
        }

        if (error.IsSuccess() && this->Settings.EnableCopyNotificationPrefetch)
        {
            error = this->PreCopyNotification(this->Settings.CopyNotificationPrefetchTypes);
        }

        if (error.IsSuccess())
        {
            // A primary will be ready after open, but a secondary will need to
            // perform tombstone recovery again after build.
            //
            error = this->RecoverTombstones();
        }

        if (error.IsSuccess())
        {
            auto root = this->Root.CreateComponentRoot();

            if (settings_.CommitBatchingPeriod > 0)
            {
                simpleTransactionGroupTimer_ = Timer::Create(TimerTagDefault, [this, root](TimerSPtr const &)
                {
                    this->SimpleTransactionGroupTimerCallback();
                },
                true); // allowConcurrency
            }

            isActive_ = true;
        }
        else
        {
            WriteInfo(TraceComponent, "{0} InitializeLocalStore failed with error {1}", this->TraceId, error);

            // Safe to release here since the initialization failed,
            // which means that we cannot have started any operations
            //
            this->ReleaseLocalStore();

            this->Cleanup();
        }

        auto * casted = dynamic_cast<OpenAsyncOperation*>(outerOperation.get());

        if (casted == nullptr)
        {
            TRACE_ERROR_AND_TESTASSERT(
                TraceComponent,
                "{0} failed to cast OpenAsyncOperation",
                this->TraceId);
        }
        else if (error.IsSuccess() && casted->IsCancelRequested)
        {
            WriteInfo(TraceComponent, "{0} open was cancelled", this->TraceId);

            this->Abort();

            error = ErrorCodeValue::OperationCanceled;
        }

        this->PostCompletion(L"Open", outerOperation, error);
    }

    void ReplicatedStore::ChangeRoleEventCallback(
        AsyncOperationSPtr const & outerOperation,
        ErrorCode const & stateMachineError,
        ReplicatedStoreState::Enum state,
        ::FABRIC_REPLICA_ROLE newRole)
    {
        ErrorCode error = stateMachineError;

        WriteInfo(
            TraceComponent, 
            "{0} ChangeRoleEventCallback(): state = {1} error = {2}", 
            this->TraceId,
            state,
            error);

        bool doComplete = true;

        if (error.IsSuccess())
        {
			if (newRole == ::FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY)
			{
				allowIncrementalBackup_.store(false);
			}

            // Try starting the log truncation timer.
            this->TryStartLogTruncationTimer(newRole);

            switch (state)
            {
                case Store::ReplicatedStoreState::PrimaryPassive:
                    TryCancelSecondaryPump();                    
                    error = InitializePrimaryProcessing();
                    break;

                case Store::ReplicatedStoreState::SecondaryActive:
                    TryUninitializePrimaryProcessing();
                    TryStartSecondaryPump();
                    break;

                case Store::ReplicatedStoreState::PrimaryChangePending:
                    StartPrimaryPendingTimer();
                    TryUninitializePrimaryProcessing();
                    // Intentional Fall through

                case Store::ReplicatedStoreState::SecondaryChangePending:
                    ASSERT_IF(pendingChangeRoleOperation_, "{0} pending change role operation already exists", this->TraceId);
                    doComplete = false;
                    pendingChangeRoleOperation_ = outerOperation;
                    break;

                default:
                    Assert::CodingError(
                        "{0} StateMaching::ChangeRoleEvent returned invalid state = {1}",
                        this->TraceId, 
                        state); 
            }
        }

        if (doComplete || !error.IsSuccess())
        {
            this->PostCompletion(L"ChangeRole", outerOperation, error);
        }
    }

    void ReplicatedStore::CloseEventCallback(
        AsyncOperationSPtr const & outerOperation,
        ErrorCode const & error,
        ReplicatedStoreState::Enum state)
    {
        WriteInfo(
            TraceComponent, 
            "{0} CloseEventCallback(): state = {1} error = {2}", 
            this->TraceId,
            state,
            error);

        SimpleTransactionGroupSPtr groupToCloseSPtr;
        {
            AcquireExclusiveLock grab(transactionGroupLock_);
            if (simpleTransactionGroupTimer_)
            {
                simpleTransactionGroupTimer_->Cancel();
                simpleTransactionGroupTimer_.reset();
            }
            groupToCloseSPtr = std::move(simpleTransactionGroupSPtr_);
        }

        if (groupToCloseSPtr)
        {
            // Closing the transaction group may finish an underlying transaction.
            // Finishing transactions will call back into the state machine, so this
            // must be posted to another thread.
            //
            Threadpool::Post([groupToCloseSPtr](){ groupToCloseSPtr->Close(); });
        }

        // Stop log truncation timer.
        this->TryStopLogTruncationTimer();

        bool doComplete = true;

        if (error.IsSuccess())
        {
            switch (state)
            {
                case Store::ReplicatedStoreState::Closed:
                    TryUninitializePrimaryProcessing();
                    Cleanup();
                    // release reference
                    TryCancelSecondaryPump();
                    break;

                case Store::ReplicatedStoreState::PrimaryClosePending:
                    StartPrimaryPendingTimer();
                    // intentional fall-through

                case Store::ReplicatedStoreState::SecondaryClosePending:
                    TryUninitializePrimaryProcessing();
                    ASSERT_IF(pendingCloseOperation_, "{0} pending close operation already exists", this->TraceId);
                    doComplete = false;
                    pendingCloseOperation_ = outerOperation;
                    break;

                default:
                    Assert::CodingError(
                        "{0} StateMaching::CloseEvent returned invalid state = {1}",
                        this->TraceId, 
                        state); 
            }
        }

        isActive_ = false;

        if (outerOperation && (doComplete || !error.IsSuccess()))
        {
            this->PostCompletion(L"Close", outerOperation, error);
        }
    }

    void ReplicatedStore::PostCompletion(wstring const & tag, AsyncOperationSPtr const & operation, ErrorCode const & error)
    {
        WriteInfo(
            TraceComponent, 
            "{0} posting {1} completion",
            this->TraceId,
            tag);

        auto root = this->Root.CreateComponentRoot();
        Threadpool::Post([this, root, tag, operation, error]
        { 
            WriteInfo(
                TraceComponent, 
                "{0} running {1} completion",
                this->TraceId,
                tag);

            operation->TryComplete(operation, error); 
        });
    }

    void ReplicatedStore::IncrementReaderCounterLocked()
    {
        AcquireExclusiveLock lock(lowWatermarkLock_);
        ++lowWatermarkReaderCount_;
    }

    void ReplicatedStore::DecrementReaderCounterLocked()
    {
        AcquireExclusiveLock lock(lowWatermarkLock_);
        --lowWatermarkReaderCount_;
    }

    bool ReplicatedStore::IsStoreMetadataType(wstring const & type)
    {
        return (type == Constants::ProgressDataType ||
                type == Constants::TombstoneDataType ||
                type == Constants::FabricTimeDataType ||
                type == Constants::LocalStoreIncrementalBackupDataType ||
                type == Constants::PartialCopyProgressDataType);
    }

    bool ReplicatedStore::ShouldMigrateKey(std::wstring const & type, std::wstring const & key)
    {
        UNREFERENCED_PARAMETER(key);

        return !IsStoreMetadataType(type);
    }

    wstring ReplicatedStore::CreateTombstoneKey1(wstring const & type, wstring const & key)
    {
        wstring escapedType = type;
        StringUtility::Replace<std::wstring>(
            escapedType,
            Constants::TombstoneKeyDelimiter, 
            Constants::TombstoneKeyEscapedDelimiter);
        wstring tombstoneKey(escapedType);

        tombstoneKey.append(Constants::TombstoneKeyDoubleDelimiter);

        // Don't need to escape the key since we will find the first instance
        // of the delimiter when parsing
        tombstoneKey.append(key);

        return tombstoneKey;
    }

    bool ReplicatedStore::TryParseTombstoneKey1(wstring const & tombstone, __out wstring & dataType, __out wstring & dataKey)
    {
        auto ix = tombstone.find(*Constants::TombstoneKeyDoubleDelimiter, 0);

        if (wstring::npos == ix)
        {
            WriteNoise(TraceComponent, "{0} cannot parse as version 1 tombstone key = '{1}'", this->TraceId, tombstone);

            return false;
        }

        dataType = tombstone.substr(0, ix);
        StringUtility::Replace<std::wstring>(
            dataType,
            Constants::TombstoneKeyEscapedDelimiter, 
            Constants::TombstoneKeyDelimiter);

        dataKey = tombstone.substr(
            ix + (*Constants::TombstoneKeyDoubleDelimiter).size(), 
            tombstone.size() - ix - (*Constants::TombstoneKeyDoubleDelimiter).size());

        return true;
    } 

    void ReplicatedStore::ReportTransientFault(ErrorCode const & fatalError)
    {
        this->ReportFault(fatalError, FABRIC_FAULT_TYPE_TRANSIENT);
    }

    void ReplicatedStore::ReportPermanentFault(ErrorCode const & fatalError)
    {
        this->ReportFault(fatalError, FABRIC_FAULT_TYPE_PERMANENT);
    }

    void ReplicatedStore::ReportFault(ErrorCode const & fatalError, FABRIC_FAULT_TYPE faultType)
    {
        AcquireWriteLock grab(fatalErrorLock_);
        if (fatalError_.IsSuccess())
        {
            if (faultType != FABRIC_FAULT_TYPE_PERMANENT && faultType != FABRIC_FAULT_TYPE_TRANSIENT)
            {
                TRACE_ERROR_AND_TESTASSERT(
                    TraceComponent,
                    "{0} invalid fault type {1} - overriding with transient fault",
                    this->TraceId,
                    static_cast<int>(faultType));

                faultType = FABRIC_FAULT_TYPE_TRANSIENT;
            }

            WriteWarning(
                TraceComponent, 
                "{0} reporting {1} fault: error={2}",
                this->TraceId,
                faultType == FABRIC_FAULT_TYPE_TRANSIENT ? "transient" : "permanent",
                fatalError);

            fatalError_ = fatalError;
            partitionCPtr_->ReportFault(faultType);
        }
    }

    void ReplicatedStore::ReportHealth(
        SystemHealthReportCode::Enum healthCode, 
        wstring const & description, 
        TimeSpan const ttl)
    {
        if (healthTrackerSPtr_)
        {
            healthTrackerSPtr_->ReportHealth(healthCode, description, ttl);
        }
    }

    void ReplicatedStore::TryFaultStreamAndStopSecondaryPump(ErrorCode const &, __in FABRIC_FAULT_TYPE faultType)
    {
        shared_ptr<SecondaryPump> snap;
        {
            AcquireReadLock lock(stateMachineUPtr_->StateLock);

            snap = secondaryPumpSPtr_;
        }

        if (snap)
        {
            // Will call into ReplicatedStore::ReportFault.
            // Do not hold fatalErrorLock_
            // 
            snap->FaultReplica(
                faultType,
                ErrorCodeValue::OperationFailed,
                L"TryFaultStreamAndStopSecondaryPump",
                false);
        }
    }

    ErrorCode ReplicatedStore::ValidateRestoreServiceData(wstring const & dir, bool performLsnCheck)
    {
        wstring restoreInstanceName = wformatString("{0}_restore", this->LocalStoreInstanceName);
        ILocalStoreUPtr restoreLocalStore = this->CreateTemporaryLocalStore();

        // Copy from the specified backup location to a temporary location in order to
        // attach the database and perform validation on its contents.
        //
        // This runs an actual restore so that issues are surfaced during the API call
        // rather than later when opening the replica. The latter is more difficult
        // to debug and handle since the replica open simply fails and gets retried.
        //
        auto error = restoreLocalStore->PrepareRestoreForValidation(dir, restoreInstanceName);

        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent, 
                "{0} Failed to initialize instance '{1}' at '{2}' for restore validation: {3}",
                this->TraceId,
                restoreInstanceName,
                dir,
                error);
            return error; 
        }

        // Simple sanity check of the restored data by reading the LSN.
        //
        if (error.IsSuccess()) 
        {
            error = this->ValidateRestoreLSN(restoreLocalStore, performLsnCheck);
        }

        // Delete all residual progress vector to ensure secondary replicas are
        // restored using the full sync replication.
        if (error.IsSuccess()) 
        {
            error = this->DeleteRestoreProgressVectorData(restoreLocalStore);
        }

        // Close local restore store
        if (error.IsSuccess())
        {
            error = restoreLocalStore->Terminate();
        }
        else if (!error.IsSuccess()) 
        {
            // This is a best-effort cleanup on failure that
            // ensures the return of the orginal error
            auto innerError = restoreLocalStore->Cleanup();
            if (!innerError.IsSuccess())
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} Failed to cleanup restore validation: {1}. Continuing restore since this failure is non-blocking.",
                    this->TraceId,
                    innerError);
            }
        }

        return error;
    }
    
    // Simple sanity check of the restored data by reading the LSN.
    //    
    ErrorCode ReplicatedStore::ValidateRestoreLSN(
        ILocalStoreUPtr const & restoreLocalStore, 
        bool performLsnCheck)
    {
        ErrorCode error(ErrorCodeValue::Success);
        
        FABRIC_SEQUENCE_NUMBER currentSequenceNumber = 0;
        {
            TransactionSPtr currentTxSPtr;
            error = this->LocalStore->CreateTransaction(currentTxSPtr);
            if (!error.IsSuccess()) { return error; }

            error = this->InnerGetCurrentProgress(currentTxSPtr, currentSequenceNumber);
            if (!error.IsSuccess()) { return error; }
        }
        
        TransactionSPtr restoreTxSPtr;
        FABRIC_SEQUENCE_NUMBER restoreSequenceNumber = 0;
        if (error.IsSuccess())
        {
            error = restoreLocalStore->CreateTransaction(restoreTxSPtr);
        }

        if (error.IsSuccess())
        {
            error = restoreLocalStore->GetLastChangeOperationLSN(
                restoreTxSPtr,
                restoreSequenceNumber);
        }

        if (error.IsSuccess()) 
        {
            if (restoreSequenceNumber > currentSequenceNumber)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} restoring data with LSNs: restore={1} current={2}",
                    this->TraceId,
                    restoreSequenceNumber,
                    currentSequenceNumber);
            }
            else
            {
                WriteWarning(
                    TraceComponent, 
                    "{0} restoring data with LSNs (overwrite): restore={1} current={2}",
                    this->TraceId,
                    restoreSequenceNumber,
                    currentSequenceNumber);

                if (performLsnCheck)
                {
                    auto msg = wformatString(
                        GET_STORE_RC(RestoreLsnCheckFailed),
                        restoreSequenceNumber,
                        currentSequenceNumber);

                    return ErrorCode(ErrorCodeValue::RestoreSafeCheckFailed, std::move(msg));
                }
            }
        }
        else
        { 
            WriteWarning(
                TraceComponent, 
                "{0} Failed to read restore LSN: {1}",
                this->TraceId,
                error);
        }

        return error;
    }

    // Delete all residual progress vector data (History and Current).
    //    
    ErrorCode ReplicatedStore::DeleteRestoreProgressVectorData(
            ILocalStoreUPtr const & restoreLocalStore) 
    {
        ErrorCode error(ErrorCodeValue::Success);
        TransactionSPtr restoreTxSPtr;

        //Delete current epoch and history data
        {
            if(error.IsSuccess())
            {
                error = restoreLocalStore->CreateTransaction(restoreTxSPtr);
            }

            if (error.IsSuccess())
            {
                error = restoreLocalStore->Delete(
                    restoreTxSPtr, 
                    Constants::ProgressDataType,
                    Constants::CurrentEpochDataName,
                    0);    
            }

            if (error.IsSuccess())
            {
                error = restoreLocalStore->Delete(
                    restoreTxSPtr, 
                    Constants::ProgressDataType,
                    Constants::EpochHistoryDataName,
                    0);
            }
            
            if (error.IsSuccess())
            {
                error = restoreTxSPtr->Commit();
                if (error.IsSuccess())
                {
                    WriteInfo(
                        TraceComponent, 
                        "{0} Deleted restore Progress Vector - Epoch history and current data",
                        this->TraceId);
                }
            }

            if (!error.IsSuccess())
            {                
                WriteWarning(
                        TraceComponent, 
                        "{0} Failed to delete restore Progress Vector - Epoch history and current data: {1}",
                        this->TraceId,
                        error);
                restoreTxSPtr->Rollback();
            }
        }

        return error;
    }

    ErrorCode ReplicatedStore::WriteRestoreMetadata(
        wstring const & dir, 
        StoreBackupType::Enum const & backupType, 
        Guid const & backupChainGuid,
        ULONG & backupIndex)
    {
        ServicePartitionInformation partitionInfo;
        auto error = this->GetCurrentPartitionInfo(partitionInfo);
        if (!error.IsSuccess()) { return error; }

		unordered_set<wstring> backupFiles;
		this->GetBackupFiles(dir, backupFiles);

		BackupRestoreData restoreData(
            partitionInfo,
            this->ReplicaId,
            backupType,
            backupChainGuid,
            backupIndex,
			std::move(backupFiles));

        KBuffer::SPtr bufferSPtr;
        error = FabricSerializer::Serialize(&restoreData, bufferSPtr);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to serialize restore data {1}: {2}",
                this->TraceId,
                restoreData,
                error);

            return error;
        }

        auto dataFileName = Path::Combine(dir, *BackupRestoreData::FileName);

        File file;
        error = file.TryOpen(
            dataFileName,
            FileMode::OpenOrCreate,
            FileAccess::Write,
            FileShare::None,
            FileAttributes::None);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed open to '{1}': {2}",
                this->TraceId,
                dataFileName,
                error);

            return error;
        }

        file.Seek(0, SeekOrigin::Begin);
        file.Write(bufferSPtr->GetBuffer(), bufferSPtr->QuerySize());

        error = file.Close2();

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed write to '{1}': {2}",
                this->TraceId,
                dataFileName,
                error);

            return error;
        }

        return error;
    }

	void ReplicatedStore::GetBackupFiles(__in wstring const & dir, __out unordered_set<wstring> & backupFiles)
	{
		// Number of backup files is not expected to be large so this should be fine for now.
		// For large number of backup files it would be preferable to have Directory::.GetFiles()
		// to take in __out param by reference and provide files as different container types or 
		// even better a Directory::EnumerateFiles() function.
		auto files = Directory::GetFiles(dir, L"*", true, false);

		// Make paths relative
		for (auto file : files)
		{
			backupFiles.insert(file.substr(dir.size() + 1));
		}
	}

    ErrorCode ReplicatedStore::ValidateAndMergeBackupChain(wstring const & restoreDir, __out wstring & mergedRestoreDir)
    {
        mergedRestoreDir = restoreDir;

        auto dataFileName = Path::Combine(restoreDir, *BackupRestoreData::FileName);
        if (File::Exists(dataFileName)) 
        {
            // Legacy single full backup or merged full+incremental backups, where all the 
            // backup files and restore metadata is present directly under the restore
            // directory specified by the user.
            BackupRestoreData restoreData;
            return this->ReadAndValidateRestoreData(dataFileName, false, restoreData);
        }
        
        // If restore metadata file (restore.dat) is not present directly under the restore directory 
        // specified by the user, we assume presence of backup-chain. This means the user specified 
        // restore directory should contain a bunch of sub-directories with one of the directory 
        // containing full backup and rest of directories containing continuous incremental backups.
        map<ULONG, wstring> backupChainOrderedDirList;
        auto error = this->ValidateBackupChain(restoreDir, backupChainOrderedDirList);
        
        if (error.IsSuccess())
        {
            error = this->LocalStore->MergeBackupChain(backupChainOrderedDirList, mergedRestoreDir);
        }
        
        return error;
    }

    ErrorCode ReplicatedStore::ValidateBackupChain(
        __in wstring const & restoreDir,
        __out map<ULONG, wstring> & backupChainOrderedDirList)
    {
        auto backupChainGuid = Guid::Empty();
        auto subdirs = Directory::GetSubDirectories(restoreDir, L"*", true, true);

        if (subdirs.size() == 0)
        {
            auto msg = wformatString(GET_STORE_RC(Restore_Dir_Empty), restoreDir);
            WriteWarning(TraceComponent, "{0}: {1}.", this->TraceId, msg);

            return ErrorCode(ErrorCodeValue::FileNotFound, std::move(msg));
        }

        wstring presentBackupIndexes = L"("; // Needed during error reporting.
        
        for (auto & dir : subdirs)
        {
            auto dataFileName = Path::Combine(dir, *BackupRestoreData::FileName);

            BackupRestoreData restoreData;
            auto error = this->ReadAndValidateRestoreData(dataFileName, true, restoreData);
            if (!error.IsSuccess())
            {
                return error;
            }

			error = this->ValidateBackupFiles(dir, restoreData);
			if (!error.IsSuccess())
			{
				return error;
			}

            if (backupChainGuid == Guid::Empty())
            {
                backupChainGuid = restoreData.BackupChainGuid;
            }
            else if (restoreData.BackupChainGuid != backupChainGuid)
            {
                auto msg = wformatString(
                    GET_STORE_RC(BackupChainId_Mismatch),
                    dir,
                    restoreData.BackupChainGuid,
                    backupChainGuid);

                WriteWarning(TraceComponent, "{0}: {1}.", this->TraceId, msg);

                return ErrorCode(ErrorCodeValue::InvalidBackupChain, std::move(msg));
            }

            auto res = backupChainOrderedDirList.insert(std::make_pair(restoreData.BackupIndex, dir));

            if (res.second == false)
            {
                auto msg = wformatString(
                    GET_STORE_RC(Duplicate_Backups),
                    dir, 
                    res.first->second,
                    restoreData.BackupChainGuid,
                    restoreData.BackupIndex);

                WriteWarning(TraceComponent, "{0}: {1}.", this->TraceId, msg);

                return ErrorCode(ErrorCodeValue::DuplicateBackups, std::move(msg));
            }

            presentBackupIndexes.append(wformatString("{0}, ", restoreData.BackupIndex));
        }

        presentBackupIndexes.erase(presentBackupIndexes.size() - 2, 2);
        presentBackupIndexes += L")";
        
        // Ensure backup-chain is valid. A valid backup chain contains
        // one full backup and zero or more continous incremental backups.
        ULONG backupIdx = 0;        
        for (auto & item : backupChainOrderedDirList)
        {
            if (item.first != backupIdx)
            {
                auto msg = wformatString(
                    GET_STORE_RC(Invalid_BackupChain),
                    restoreDir,
                    backupIdx,
                    presentBackupIndexes);

                WriteWarning(TraceComponent, "{0}: {1}.", this->TraceId, msg);

                return ErrorCode(ErrorCodeValue::InvalidBackupChain, std::move(msg));
            }

            backupIdx++;
        }

        return ErrorCodeValue::Success;
    }

	ErrorCode ReplicatedStore::ValidateBackupInfo(__in std::wstring const & dataFileName, __in BackupRestoreData const & restoreData)
	{
		bool isValid = true;
		wstring errMsg;

		if (restoreData.BackupType != StoreBackupType::Full &&
			restoreData.BackupType != StoreBackupType::Incremental)
		{
			isValid = false;
			errMsg = wformatString(
				GET_STORE_RC(InvalidRestoreData),
				dataFileName,
				L"BackupType",
				restoreData);
		}

		if (restoreData.BackupChainGuid == Guid::Empty())
		{
			isValid = false;
			errMsg = wformatString(
				GET_STORE_RC(InvalidRestoreData),
				dataFileName,
				L"BackupChainGuid",
				restoreData);
		}

		if (restoreData.BackupType == StoreBackupType::Full && restoreData.BackupIndex != 0)
		{
			isValid = false;
			errMsg = wformatString(
				GET_STORE_RC(InvalidRestoreData),
				dataFileName,
				L"BackupIndex",
				restoreData);
		}

		if (restoreData.BackupType == StoreBackupType::Incremental && restoreData.BackupIndex < 1)
		{
			isValid = false;
			errMsg = wformatString(
				GET_STORE_RC(InvalidRestoreData),
				dataFileName,
				L"BackupIndex",
				restoreData);
		}

		if (!isValid)
		{
			WriteWarning(TraceComponent, "{0}: {1}", this->TraceId, errMsg);
			return ErrorCode(ErrorCodeValue::InvalidRestoreData, std::move(errMsg));
		}

		return ErrorCode::Success();
	}

	ErrorCode ReplicatedStore::ValidateBackupFiles(__in std::wstring const & restoreDir, __in BackupRestoreData const & restoreData)
	{
		unordered_set<wstring> backupFiles;
		this->GetBackupFiles(restoreDir, backupFiles);

		backupFiles.erase(*BackupRestoreData::FileName);
		
		wstring missingFiles;
		for (auto const & file : restoreData.BackupFiles)
		{
			if (backupFiles.find(file) == backupFiles.end())
			{
				missingFiles.append(file + L";");
			}
			else
			{
				backupFiles.erase(file);
			}
		}

		if (!missingFiles.empty())
		{
			missingFiles.erase(--missingFiles.end());

			auto msg = wformatString(GET_STORE_RC(MissingBackupFile), missingFiles, restoreDir);

			WriteWarning(TraceComponent, "{0}: {1}.", this->TraceId, msg);
			return ErrorCode(ErrorCodeValue::InvalidBackup, std::move(msg));
		}

		if (!backupFiles.empty())
		{
			wstring extraFiles;
			for (auto const & file : backupFiles)
			{
				extraFiles.append(file + L";");
			}

			extraFiles.erase(--extraFiles.end());

			auto msg = wformatString(GET_STORE_RC(ExtraBackupFile), extraFiles, restoreDir);

			WriteWarning(TraceComponent, "{0}: {1}.", this->TraceId, msg);
			return ErrorCode(ErrorCodeValue::InvalidBackup, std::move(msg));
		}

		return ErrorCode::Success();
	}

    ErrorCode ReplicatedStore::ReadAndValidateRestoreData(
        wstring const & dataFileName,
        bool validateBackupInfo,
        __out BackupRestoreData & restoreData)
    {
        if (!File::Exists(dataFileName))
        {
			auto msg = wformatString(GET_STORE_RC(RestoreDataFileMissing), dataFileName);

			WriteWarning(TraceComponent, "{0}: {1}.", this->TraceId, msg);
			return ErrorCode(ErrorCodeValue::InvalidBackup, std::move(msg));
        }

        File file;
        auto error = file.TryOpen(
            dataFileName,
            FileMode::Open,
            FileAccess::Read,
            FileShare::Read,
            FileAttributes::ReadOnly);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to open restore data from '{1}': {2}",
                this->TraceId,
                dataFileName,
                error);

            return error;
        }

        if (file.size() == 0)
        {
            WriteWarning(
                TraceComponent, 
                "{0} Restore data file '{1}' is empty",
                this->TraceId,
                dataFileName);

            return ErrorCodeValue::OperationFailed;
        }

        int fileSize = static_cast<int>(file.size());
        vector<byte> buffer(fileSize);

        int bytesRead = file.TryRead(reinterpret_cast<void*>(buffer.data()), fileSize);
        if (bytesRead <= 0)
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to read from '{1}': expected {2} bytes, read {3} bytes",
                this->TraceId,
                dataFileName,
                fileSize,
                bytesRead);

            return ErrorCodeValue::OperationFailed;
        }

        error = FabricSerializer::Deserialize(restoreData, buffer);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to deserialize restore data from '{1}': {2}",
                this->TraceId,
                dataFileName,
                error);

            return error;
        }

        ServicePartitionInformation partitionInfo;
        error = this->GetCurrentPartitionInfo(partitionInfo);

        if (!error.IsSuccess())
        {
            return error;
        }

        if (!partitionInfo.AreEqualIgnorePartitionId(restoreData.PartitionInfo))
        {
			auto msg = wformatString(
				GET_STORE_RC(RestorePartitionInfoMismatch),
                partitionInfo,
                restoreData.PartitionInfo,
                dataFileName);

            WriteWarning(TraceComponent, "{0}: {1}", this->TraceId, msg);

            return ErrorCode(ErrorCodeValue::InvalidOperation, std::move(msg));
        }

        if (validateBackupInfo)
        {
			error = this->ValidateBackupInfo(dataFileName, restoreData);
        }

        return error;
    }

    ErrorCode ReplicatedStore::GetCurrentPartitionInfo(__out ServiceModel::ServicePartitionInformation & result)
    {
        FABRIC_SERVICE_PARTITION_INFORMATION const * buffer;
        auto hr = partitionCPtr_->GetPartitionInfo(&buffer);

        if (FAILED(hr))
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to get partition information: {1}",
                this->TraceId,
                ErrorCode::FromHResult(hr));

            return ErrorCode::FromHResult(hr);
        }

        auto error = result.FromPublicApi(*buffer);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent, 
                "{0} Failed to convert partition information: {1}",
                this->TraceId,
                error);

            return error;
        }

        return error;
    }
    
    ErrorCode ReplicatedStore::GetCurrentStoreTime(__out int64 & currentStoreTime)
    {
        auto error = this->CheckReadAccess(nullptr, L"");

        if (!error.IsSuccess()) { return error; }

        FabricTimeControllerSPtr localFabricTimeControllerSPtr;
        {
            AcquireReadLock grab(stateMachineUPtr_->StateLock);
            localFabricTimeControllerSPtr=fabricTimeControllerSPtr_;
        }
        if(localFabricTimeControllerSPtr)
        {
            error=localFabricTimeControllerSPtr->GetCurrentStoreTime(currentStoreTime);
        }
        else
        {
            error=ErrorCodeValue::NotReady;
        }

        return error;
    }

    /*
        This method is running under state machine lock. Thread safety is guaranteed so 
        no lock is needed
    */
    bool ReplicatedStore::TryStartFabricTimer()
    {
        if(!fabricTimeControllerSPtr_ && enableFabricTimer_)
        {
            fabricTimeControllerSPtr_=make_shared<FabricTimeController>(*this);
            fabricTimeControllerSPtr_->Start(test_StartFabricTimerWithCancelWait_);

            return true;
        }
        else
        {
            return false;
        }
    }
        
    /*
        This method is running under state machine lock. Thread safety is guaranteed so 
        no lock is needed
    */
    bool ReplicatedStore::TryCancelFabricTimer()
    {
        if(fabricTimeControllerSPtr_ && enableFabricTimer_)
        {
            fabricTimeControllerSPtr_->Cancel();
            fabricTimeControllerSPtr_.reset();
            return true;
        }
        else
        {
            return false;
        }
    }
}
