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
using namespace RobustTXRService;

StringLiteral const TraceComponent("RobustTestRunner");

RobustTestRunner::SPtr RobustTestRunner::Create(
    __in ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator,
    __in KAllocator & allocator)
{
    RobustTestRunner * pointer = _new(ROBUSTTEST_TAG, allocator)RobustTestRunner(testParametersJson, txnReplicator);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return RobustTestRunner::SPtr(pointer);
}

RobustTestRunner::RobustTestRunner(
    __in ByteBufferUPtr const & testParametersJson,
    __in TxnReplicator::ITransactionalReplicator & txnReplicator)
    : NightWatchTXRService::TestRunner(testParametersJson, txnReplicator)
    , testParameters_()
    , random_(GetTickCount())
{
    JsonSerializerFlags flags = JsonSerializerFlags::EnumInStringFormat;
    JsonHelper::Deserialize<TestParameters>(testParameters_, testParametersJson, flags);
    testParameters_.PopulateUpdateOperationValues();
}

RobustTestRunner::~RobustTestRunner()
{
}

Awaitable<PerfResult::SPtr> RobustTestRunner::Run()
{
    auto result = co_await TestRunnerBase::Run(1, testParameters_.NumberOfTransactions);

    co_return result;
}

Awaitable<void> RobustTestRunner::DoOperation()
{
    TxnReplicator::Transaction::SPtr transaction1;
    Common::Guid id = Common::Guid::NewGuid();
    wstring key = wformatString("fabrictest:/txrsp/:{0}", id);
    KUriView keyUri(&key[0]);

    THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction1));
    NTSTATUS status = co_await txnReplicator_->AddAsync(
        *transaction1,
        keyUri,
        TxnReplicator::TestCommon::NoopStateProvider::TypeName,
        nullptr);
    if (!NT_SUCCESS(status))
    {
        throw ktl::Exception(status);
    }
        
    THROW_ON_FAILURE(co_await transaction1->CommitAsync());
    transaction1->Dispose();

    TxnReplicator::Transaction::SPtr transaction2;
    THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction2));
    TxnReplicator::IStateProvider2::SPtr stateProvider;

    status = txnReplicator_->Get(keyUri, stateProvider);
    if (!NT_SUCCESS(status) && status != SF_STATUS_NAME_DOES_NOT_EXIST)
    {
        throw ktl::Exception(status);
    }

    TxnReplicator::TestCommon::NoopStateProvider::SPtr testStateProvider = down_cast<TxnReplicator::TestCommon::NoopStateProvider, TxnReplicator::IStateProvider2>(stateProvider);
    THROW_ON_FAILURE(co_await transaction2->CommitAsync());
    transaction2->Dispose();

    while (true)
    {
        if (ShouldStop())
        {
            Trace.WriteInfo(
                TraceComponent,
                "Stopping. Operation Count = {0}",
                operationsCount_.load());
            co_return;
        }

        Trace.WriteInfo(
            TraceComponent,
            "Operation Count = {0}",
            operationsCount_.load());

        TxnReplicator::Transaction::SPtr transaction;
        THROW_ON_FAILURE(txnReplicator_->CreateTransaction(transaction));

        std::wstring valueString = testParameters_.GetOperationValue(getRandomNumber(testParameters_.OperationValuesCount));

        KStringView newValue(&valueString[0]);

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
        if (!NT_SUCCESS(status))
        {
            Trace.WriteInfo(
                TraceComponent,
                "First AddOperation failed with status {0}",
                status);
        }

        status = testStateProvider->AddOperation(
            *transaction,
            nullptr,
            op.RawPtr(),
            op.RawPtr(),
            nullptr);
        if (!NT_SUCCESS(status))
        {
            Trace.WriteInfo(
                TraceComponent,
                "Second AddOperation failed with status {0}",
                status);
        }

        if (NT_SUCCESS(status) && getRandomNumber() > testParameters_.SkipTransactionCompletionRatio)
        {
            Trace.WriteInfo(
                TraceComponent,
                "Commiting operation");

            THROW_ON_FAILURE(co_await transaction->CommitAsync());
            transaction->Dispose();
        }

        IncOperation(1);
    }
    co_return;
}

double RobustTestRunner::getRandomNumber()
{
    return random_.NextDouble();
}

int RobustTestRunner::getRandomNumber(int max)
{
    return random_.Next(0, max);
}