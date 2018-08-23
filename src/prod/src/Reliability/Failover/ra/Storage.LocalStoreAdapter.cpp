// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Common;
using namespace Transport;
using namespace Store;
using namespace Federation;
using namespace Infrastructure;
using Diagnostics::RAPerformanceCounters;
using namespace Storage;
using namespace Storage::Api;

namespace
{
    GlobalWString ReconfigurationAgentStoreInstance = make_global<wstring>(L"ReconfigurationAgentStoreInstance");

    GlobalWString RAStoreDirectory = make_global<wstring>(L"RA");
    GlobalWString RAStoreFilename = make_global<wstring>(L"RA.edb");
}

class LocalStoreAdapter::TransactionHolder
{
    DENY_COPY(TransactionHolder);
public:
    TransactionHolder(LocalStoreAdapter & store) :
        perfCounters_(store.GetPerfCounters())
    {
    }

    TransactionHolder(TransactionSPtr && txn, LocalStoreAdapter & store) :
        txn_(std::move(txn)),
        perfCounters_(store.GetPerfCounters())
    {
        perfCounters_.NumberOfStoreTransactionsPerSecond.Increment();
        perfCounters_.NumberOfActiveStoreTransactions.Increment();
    }

    TransactionHolder & operator=(TransactionHolder && other)
    {
        if (this != &other)
        {
            txn_ = std::move(other.txn_);
        }

        return *this;
    }

    ~TransactionHolder()
    {
        if (txn_ != nullptr)
        {
            perfCounters_.NumberOfActiveStoreTransactions.Decrement();
        }
    }

    __declspec(property(get = get_Transaction)) Store::IStoreBase::TransactionSPtr const & Transaction;
    Store::IStoreBase::TransactionSPtr const & get_Transaction() const
    {
        return txn_;
    }

private:
    TransactionSPtr txn_;
    Diagnostics::RAPerformanceCounters & perfCounters_;
};

class LocalStoreAdapter::CommitAsyncOperation : public Common::AsyncOperation
{
    DENY_COPY(CommitAsyncOperation);
public:
    CommitAsyncOperation(
        LocalStoreAdapter & store,
        RowIdentifier const & id,
        OperationType::Enum operationType,
        RowData && bytes,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent) :
        AsyncOperation(callback, parent),
        store_(store),
        id_(id),
        operationType_(operationType),
        bytes_(std::move(bytes)),
        timeout_(timeout),
        txnHolder_(store)
    {
    }

protected:
    void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
    {
        RowData bytes = std::move(bytes_);
        auto error = CreateTransaction();
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        error = store_.PerformOperationInternal(txnHolder_.Transaction, operationType_, id_, bytes);

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        auto op = BeginCommit(
            [this](Common::AsyncOperationSPtr const & innerOp)
            {
                if (!innerOp->CompletedSynchronously)
                {
                    FinishCommit(innerOp);
                }
            },
            thisSPtr);

        if (op->CompletedSynchronously)
        {
            FinishCommit(op);
        }
    }

private:
    void FinishCommit(Common::AsyncOperationSPtr const & txPtrCommitOperation)
    {
        auto error = EndCommit(txPtrCommitOperation);

        /*
            Release ESE callback threads immediately -> otherwise ESE callbacks will be delayed further
        */
        auto op = store_.GetThreadpool().BeginScheduleCommitCallback(
            [this, error] (Common::AsyncOperationSPtr const & scheduleCommitOp)
            {
                if (!scheduleCommitOp->CompletedSynchronously)
                {
                    FinishScheduleCommitCallback(scheduleCommitOp, error);
                }
            },
            txPtrCommitOperation->Parent);

        if (op->CompletedSynchronously)
        {
            FinishScheduleCommitCallback(op, error);
        }
    }

    void FinishScheduleCommitCallback(Common::AsyncOperationSPtr const & scheduleCommitCallbackOp, Common::ErrorCode const & txCommitError)
    {
        auto scheduleCommitError = store_.GetThreadpool().EndScheduleCommitCallback(scheduleCommitCallbackOp);
        ASSERT_IF(!scheduleCommitError.IsSuccess(), "Schedule commit must succeed");

        TryComplete(scheduleCommitCallbackOp->Parent, txCommitError);
    }

    Common::ErrorCode CreateTransaction()
    {
        return store_.CreateTransaction(txnHolder_);
    }

    Common::AsyncOperationSPtr BeginCommit(
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        store_.GetPerfCounters().NumberOfStoreCommitsPerSecond.Increment();
        store_.GetPerfCounters().NumberOfCommittingStoreTransactions.Increment();
        return txnHolder_.Transaction->BeginCommit(timeout_, callback, parent);
    }

    Common::ErrorCode EndCommit(Common::AsyncOperationSPtr const & commitOp)
    {
        store_.GetPerfCounters().NumberOfCommittingStoreTransactions.Decrement();
        return txnHolder_.Transaction->EndCommit(commitOp);
    }

    LocalStoreAdapter & store_;
    RowData bytes_;
    OperationType::Enum operationType_;
    Common::TimeSpan timeout_;
    TransactionHolder txnHolder_;

    // only used in the sync part of this API so safe to keep as const ref
    RowIdentifier const & id_;
};

// Constructor
LocalStoreAdapter::LocalStoreAdapter(
    Store::IStoreFactorySPtr const & storeFactory,
    ReconfigurationAgent & ra) : 
    storeFactory_(storeFactory),
    ra_(ra),
    isOpen_(false)
{
    ASSERT_IF(storeFactory == nullptr, "Factory can't be null");
}

ErrorCode LocalStoreAdapter::Open(
    wstring const & nodeId, 
    wstring const & workingDirectory,
    KtlLogger::KtlLoggerNodeSPtr const & ktlLogger)
{     
    wstring instance(*ReconfigurationAgentStoreInstance + nodeId);
    wstring raDirectory = Path::Combine(workingDirectory, RAStoreDirectory);

    Store::StoreFactoryParameters parameters;
    bool enableTStore = (FailoverConfig::GetConfig().EnableLocalTStore || Store::StoreConfig::GetConfig().EnableTStore);
    parameters.Type = enableTStore ? StoreType::TStore : StoreType::Local;
    parameters.NodeInstance = ra_.NodeInstance;
    parameters.NodeName = ra_.Reliability.NodeConfig->InstanceName;
    parameters.DirectoryPath = raDirectory;
    parameters.FileName = *RAStoreFilename;
    parameters.AssertOnFatalError = FailoverConfig::GetConfig().AssertOnStoreFatalError;
    parameters.KtlLogger = ktlLogger;
    parameters.AllowMigration = FailoverConfig::GetConfig().AllowLocalStoreMigration;

    ILocalStoreSPtr store;
    ServiceModel::HealthReport healthReport;
    auto error = storeFactory_->CreateLocalStore(parameters, store, healthReport);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = store->Initialize(instance, ra_.NodeId);
    if (error.IsSuccess())
    {
        store_ = move(store);
        isOpen_.store(true);

        ra_.HealthSubsystem.ReportStoreProviderEvent(move(healthReport));
    }
    else
    {
        store.reset();
    }

    return error;
}

void LocalStoreAdapter::Close()
{
    /*
        The RA semantics for the lifecycle of the store are described below:
        - The store_ variable contains a shared_ptr to the store

        - The store_ is initialized during Open

        - The store_ is not released in Dispose: This is because a transaction after it has been created 
          could still need the store to perform operations such as Add. New transactions will not be allowed
          after this point because the RA is closing

        - The store_ must be terminated and drained so that a future Open would be
          able to initialize the store again. This is required in FabricTest where after a node is cloed
          the same node (and by that definition the same ReliabilitySubsystem, RA, LFUM, RAStore object) 
          may be reusued by calling Open

        - This wait is a synchronous wait because the Open and Close on the RA are synchronous. This is not
          too much of a problem because during RA Open/Close the node itself is opening/closing so the amount
          of work being done is low anyway
    */
    if (!isOpen_.exchange(false))
    {
        return;
    }

    store_->Terminate();
    store_->Drain();
}

ErrorCode LocalStoreAdapter::Enumerate(
    RowType::Enum type,
    __out std::vector<Row>& rows)
{
    std::vector<Row> inner;
    Store::IStoreBase::TransactionSPtr txPtr;

    auto error = CreateTransaction(txPtr);
    if (!error.IsSuccess())
    {
        return error;
    }

    Store::IStoreBase::EnumerationSPtr enumSPtr;

    error = store_->CreateEnumerationByTypeAndKey(txPtr, Common::wformatString(type), L"", enumSPtr);
    if (!error.IsSuccess())
    {
        txPtr->Rollback();
        return error;
    }

    for (;;)
    {
        error = enumSPtr->MoveNext();
        if (!error.IsSuccess())
        {
            break;
        }

        RowData bytes;
        error = enumSPtr->CurrentValue(bytes);
        if (!error.IsSuccess())
        {
            return error;
        }

        wstring id;
        error = enumSPtr->CurrentKey(id);
        if (!error.IsSuccess())
        {
            return error;
        }

        Row r;
        r.Id = RowIdentifier(type, std::move(id));
        r.Data = std::move(bytes);
        inner.push_back(std::move(r));
    }

    if (error.IsSuccess() || error.IsError(Common::ErrorCodeValue::EnumerationCompleted))
    {
        error = txPtr->Commit();
    }

    if (!error.IsSuccess())
    {
        txPtr->Rollback();
    }

    if (error.IsSuccess())
    {
        rows = std::move(inner);
    }

    return error;
}

ErrorCode LocalStoreAdapter::CreateTransaction(TransactionSPtr & txPtr) const
{
    if (!isOpen_.load())
    {
        return ErrorCode(ErrorCodeValue::RAStoreNotUsable);
    }

    IStoreBase::TransactionSPtr localStoreTxPtr;
    ErrorCode error = store_->CreateTransaction(localStoreTxPtr);

    if (error.IsSuccess())
    {
        txPtr = move(localStoreTxPtr);
    }

    return error;
}

ErrorCode LocalStoreAdapter::CreateTransaction(TransactionHolder & holder) 
{
    IStoreBase::TransactionSPtr localStoreTxPtr;
    auto error = store_->CreateTransaction(localStoreTxPtr);

    if (error.IsSuccess())
    {
        holder = TransactionHolder(move(localStoreTxPtr), *this);
    }

    return error;
}

IThreadpool & LocalStoreAdapter::GetThreadpool()
{
    return ra_.Threadpool;
}

RAPerformanceCounters & LocalStoreAdapter::GetPerfCounters()
{
    return ra_.PerfCounters;
}

Common::AsyncOperationSPtr LocalStoreAdapter::BeginStoreOperation(
    OperationType::Enum operationType,
    RowIdentifier const & rowId,
    RowData && bytes,
    Common::TimeSpan const, //timeout
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return Common::AsyncOperation::CreateAndStart<CommitAsyncOperation>(*this, rowId, operationType, std::move(bytes), Common::TimeSpan::MaxValue, callback, parent);
}

Common::ErrorCode LocalStoreAdapter::EndStoreOperation(Common::AsyncOperationSPtr const & operation)
{
    return Common::AsyncOperation::End<Common::AsyncOperation>(operation)->Error;
}

Common::ErrorCode LocalStoreAdapter::PerformOperationInternal(
    Store::IStoreBase::TransactionSPtr const & txPtr,
    OperationType::Enum operationType,
    Storage::Api::RowIdentifier const & rowId,
    RowData const & bytes)
{
    switch (operationType)
    {
    case OperationType::Insert:
        return TryInsertOrUpdate(txPtr, false, rowId.TypeString, rowId.Id, bytes);

    case OperationType::Delete:
        return TryDelete(txPtr, rowId.TypeString, rowId.Id);

    case OperationType::Update:
        return TryInsertOrUpdate(txPtr, true, rowId.TypeString, rowId.Id, bytes);

    default:
        Common::Assert::CodingError("Invalid type");
        break;
    };
}

Common::ErrorCode LocalStoreAdapter::TryDelete(TransactionSPtr const& txPtr, std::wstring const & type, std::wstring const & id)
{
    return store_->Delete(txPtr, type, id, 0);
}

Common::ErrorCode LocalStoreAdapter::TryInsertOrUpdate(
    TransactionSPtr const & txPtr,
    bool isUpdate,
    std::wstring const & type,
    std::wstring const & id,
    RowData const & buffer)
{
    if (isUpdate)
    {
        return store_->Update(
            txPtr,
            type,
            id,
            0,
            id,
            &(*buffer.begin()),
            buffer.size(),
            Store::ILocalStore::OperationNumberUnspecified);
    }
    else
    {
        return store_->Insert(
            txPtr,
            type,
            id,
            &(*buffer.begin()),
            buffer.size(),
            Store::ILocalStore::OperationNumberUnspecified);
    }
}
