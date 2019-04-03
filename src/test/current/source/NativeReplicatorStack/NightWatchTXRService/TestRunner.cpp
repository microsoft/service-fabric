// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <SfStatus.h>

using namespace ktl;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace NightWatchTXRService;
using namespace TxnReplicator;

StringLiteral const TraceComponent("NightWatchTXRService");

TestRunner::SPtr TestRunner::Create(
    __in ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    TestRunner * pointer = _new(NIGHTWATCHTESTRUNNER_TAG, allocator)TestRunner(
        testParametersJson,
        txnReplicator);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return TestRunner::SPtr(pointer);
}

TestRunnerBase::TestRunnerBase(
    __in TxnReplicator::ITransactionalReplicator & txnReplicator)
    : txnReplicator_(&txnReplicator)
    , runningThreadsCount_(0)
    , operationsCount_(0)
    , maxOutstandingOperations_(0)
    , numberOfOperations_(0)
{
    KSharedPtr<AwaitableCompletionSource<void>> result;
    AwaitableCompletionSource<void>::Create(GetThisAllocator(), NIGHTWATCHTESTRUNNER_TAG, result);

    testCompletionTcs_ = result;
}

TestRunnerBase::~TestRunnerBase()
{
}

Awaitable<PerfResult::SPtr> TestRunnerBase::Run(__in ULONG maxOutstandingOperations, __in ULONG numberOfOperations)
{
    maxOutstandingOperations_ = maxOutstandingOperations;
    numberOfOperations_ = numberOfOperations;

    Stopwatch watch;
    watch.Start();
    int64 startTicks = DateTime::Now().Ticks;
    for (ULONG i = 0; i < maxOutstandingOperations_; i++)
    {
        DoWork();
    }

    CheckTestCompletion();

    co_await testCompletionTcs_->GetAwaitable();

    watch.Stop();
    int64 endTicks = DateTime::Now().Ticks;

    double opsPerMillisecond = (double)operationsCount_.load() / (double)watch.ElapsedMilliseconds;
    double opsPerSecond = opsPerMillisecond * 1000;
    double averageLatency = 1000.0 * (1.0 / (opsPerSecond / maxOutstandingOperations_));
    
    opsPerMillisecond = RoundTo(opsPerMillisecond, 2);
    opsPerSecond = RoundTo(opsPerSecond, 2);
    averageLatency = RoundTo(averageLatency, 2);

    if (exceptionThrown_.load())
    {
        co_return PerfResult::Create(
            exceptionSptr_.RawPtr(),
            GetThisAllocator());
    }
    else
    {
        co_return PerfResult::Create(
            (ULONG)operationsCount_.load(),
            watch.Elapsed.TotalSeconds(),
            opsPerSecond,
            opsPerMillisecond,
            opsPerSecond,
            averageLatency,
            1,
            startTicks,
            endTicks,
            L"",
            TestStatus::Enum::Finished,
            nullptr,
            GetThisAllocator());
    }
}

Task TestRunnerBase::DoWork()
{
    runningThreadsCount_ += 1;
    KFinally([&]() { runningThreadsCount_ -= 1; });

    co_await DoOperation();
}

Task TestRunnerBase::CheckTestCompletion()
{
    while (true)
    {
        NTSTATUS status = co_await KTimer::StartTimerAsync(GetThisAllocator(), NIGHTWATCHTESTRUNNER_TAG, 200, nullptr);
        THROW_ON_FAILURE(status);

        LONG runningCount = runningThreadsCount_.load();

        if (runningCount == 0)
        {
            break;
        }
    }

    testCompletionTcs_->TrySet();
}

ktl::Awaitable<void> TestRunner::DoOperation()
{
    Common::Guid threadKey = Common::Guid::NewGuid();
    wstring keyStateProvider = wformatString("fabrictest:/txrsp/:{0}", threadKey);

    LONG64 version = co_await AddTx(keyStateProvider);
    operationsCount_ += 1;

    //if any exception is thrown (on any thread), stop processing
    if (exceptionThrown_.load())
    {
        co_return;
    }

    while (true)
    {
        //if operation count is reached, stop processing
        ULONG operationsCount = (ULONG)operationsCount_.load();
        if (operationsCount > testParameters_.NumberOfOperations)
        {
            co_return;
        }

        //if any exception is thrown (on any thread), stop processing
        if (exceptionThrown_.load())
        {
            co_return;
        }

        version = co_await UpdateTx(keyStateProvider, version);
        operationsCount_ += 1;
    }
}

Awaitable<LONG64> TestRunner::AddTx(__in std::wstring const & key)
{
    TxnReplicator::Transaction::SPtr transaction;
    THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));

    TxnReplicator::IStateProvider2::SPtr stateProvider;

    KUriView keyUri(&key[0]);
    std::wstring valueString = L"i";
    KStringView value(&valueString[0]);

    NTSTATUS status = txnReplicator_->Get(keyUri, stateProvider);

    if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        status = co_await txnReplicator_->AddAsync(
            *transaction,
            keyUri,
            TxnReplicator::TestCommon::NoopStateProvider::TypeName,
            nullptr);
        THROW_ON_FAILURE(status);
    }
    else if (!NT_SUCCESS(status))
    {
        throw ktl::Exception(status);
    }

    THROW_ON_FAILURE(co_await transaction->CommitAsync());
    transaction->Dispose();

    co_return 1;
}

Awaitable<LONG64> TestRunner::UpdateTx(__in std::wstring const & key, __in LONG64 version)
{
    TxnReplicator::Transaction::SPtr transaction;
    THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));
    KFinally([transaction] {transaction->Dispose(); });

    TxnReplicator::IStateProvider2::SPtr stateProvider;

    KUriView keyUri(&key[0]);
    std::wstring valueString = testParameters_.UpdateOperationValue;

    KStringView newValue(&valueString[0]);

    NTSTATUS status = txnReplicator_->Get(keyUri, stateProvider);
    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        throw ktl::Exception(status);
    }

    TxnReplicator::TestCommon::NoopStateProvider::SPtr testStateProvider = down_cast<TxnReplicator::TestCommon::NoopStateProvider, TxnReplicator::IStateProvider2>(stateProvider);

    Data::Utilities::OperationData::SPtr op = Data::Utilities::OperationData::Create(GetThisAllocator());
    Data::Utilities::BinaryWriter bw(GetThisAllocator());
    bw.Write(newValue, Data::Utilities::UTF16);
    op->Append(*bw.GetBuffer(0));

    status = testStateProvider->AddOperation(
        *transaction,
        nullptr,
        op.RawPtr(),
        op.RawPtr(),
        nullptr);

    if (NT_SUCCESS(status) == false)
    {
        exceptionThrown_.store(true);
        exceptionSptr_ = Data::Utilities::SharedException::Create(ktl::Exception(SF_STATUS_TRANSACTION_ABORTED), GetThisAllocator());

        transaction->Abort();
        co_return transaction->CommitSequenceNumber;
    }

    THROW_ON_FAILURE(co_await transaction->CommitAsync());
    co_return transaction->CommitSequenceNumber;
}

TestRunner::TestRunner(
    __in ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator)
    : TestRunnerBase(txnReplicator)
    , testParameters_()
{
    JsonSerializerFlags flags = JsonSerializerFlags::EnumInStringFormat;
    JsonHelper::Deserialize<NightWatchTestParameters>(testParameters_, testParametersJson, flags);
    testParameters_.PopulateUpdateOperationValue();
}

TestRunner::TestRunner(
    __in TxnReplicator::ITransactionalReplicator & txnReplicator)
    : TestRunnerBase(txnReplicator)
    , testParameters_()
{
}

TestRunner::~TestRunner()
{
}

Awaitable<PerfResult::SPtr> TestRunner::Run()
{
    auto result = co_await TestRunnerBase::Run(testParameters_.MaxOutstandingOperations, testParameters_.NumberOfOperations);
    co_return result;
}