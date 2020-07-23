//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace RCQNightWatchTXRService;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

StringLiteral const TraceComponent("RCQNightWatchTXRService");

RCQTestRunner::SPtr RCQTestRunner::Create(
    __in Common::ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    RCQTestRunner * pointer = _new('xxxx', allocator) RCQTestRunner(
        testParametersJson,
        txnReplicator);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return RCQTestRunner::SPtr(pointer);
}

RCQTestRunner::RCQTestRunner(
    __in Common::ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator)
    : NightWatchTXRService::TestRunner(testParametersJson, txnReplicator)
    , rcqSPtr_(nullptr)
    , testParameters_()
{
    JsonSerializerFlags flags = JsonSerializerFlags::EnumInStringFormat;
    JsonHelper::Deserialize<NightWatchTXRService::NightWatchTestParameters>(testParameters_, testParametersJson, flags);
}

RCQTestRunner::~RCQTestRunner()
{
}

Awaitable<void> RCQTestRunner::DoOperation()
{
    TxnReplicator::IStateProvider2::SPtr stateProvider;
    KUriView nameUri(L"fabric:/nightwatchRCQ");
    KStringView typeName(
        (PWCHAR)TestStateProvider::TypeName.cbegin(),
        (ULONG)TestStateProvider::TypeName.size() + 1,
        (ULONG)TestStateProvider::TypeName.size());

    KBuffer::SPtr value = nullptr;
    KBuffer::Create(128, value, GetThisAllocator());

    {
        TxnReplicator::Transaction::SPtr transaction;
        THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));
        KFinally([&]() { transaction->Dispose(); });
        
        bool alreadyExist = false;
        NTSTATUS status = co_await txnReplicator_->GetOrAddAsync(
            *transaction,
            nameUri,
            typeName,
            stateProvider,
            alreadyExist);
        THROW_ON_FAILURE(status);
        ASSERT_IFNOT(stateProvider != nullptr, "State provider should not be null");
        THROW_ON_FAILURE(co_await transaction->CommitAsync());
    }

    const int ITEM_COUNT = 10;

    while (true)
    {
        rcqSPtr_ = dynamic_cast<Data::Collections::IReliableConcurrentQueue<KBuffer::SPtr>*>(stateProvider.RawPtr());

        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            TxnReplicator::Transaction::SPtr transaction;
            THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));
            KFinally([&]() { transaction->Dispose(); });

            co_await rcqSPtr_->EnqueueAsync(
                *transaction,
                value,
                Common::TimeSpan::MaxValue,
                CancellationToken::None
            );
            
            THROW_ON_FAILURE(co_await transaction->CommitAsync());

            IncOperation(1);

            if (ShouldStop())
                co_return;
        }

        for (int i = 0; i < ITEM_COUNT; ++i)
        {
            TxnReplicator::Transaction::SPtr transaction;
            THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));
            KFinally([&]() { transaction->Dispose(); });

            bool succeeded = co_await rcqSPtr_->TryDequeueAsync(
                *transaction,
                value,
                Common::TimeSpan::MaxValue,
                CancellationToken::None
            );

            ASSERT_IFNOT(succeeded, "TryDequeueAsync should always succeed");
            
            THROW_ON_FAILURE(co_await transaction->CommitAsync());
            
            IncOperation(1);

            if (ShouldStop())
                co_return;
        }
    }
}

/*Awaitable<NightWatchTXRService::PerfResult::SPtr> RCQTestRunner::Run()
{
    auto result = co_await TestRunnerBase::Run(testParameters_.MaxOutstandingOperations, testParameters_.NumberOfOperations);

    co_return result;
}*/