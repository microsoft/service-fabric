// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;
using namespace Data::Utilities;
using namespace LoggingReplicatorTests;

#define TESTCOPYSTREAMCONVERTER_TAG 'CSCT'

TestCopyStreamConverter::TestCopyStreamConverter(
    __in Data::LoggingReplicator::CopyStream & copyStream,
    __in ULONG32 maximumsuccessfulOperationCount)
    :copyStream_(&copyStream),
    maximumsuccessfulOperationCount_(maximumsuccessfulOperationCount),
    currentOperationCount_(0)
{
}

TestCopyStreamConverter::~TestCopyStreamConverter()
{
}

TestCopyStreamConverter::SPtr TestCopyStreamConverter::Create(
    __in Data::LoggingReplicator::CopyStream & copyStream,
    __in ULONG32 maximumsuccessfulOperationCount,
    __in KAllocator & allocator)
{
    TestCopyStreamConverter * value = _new(TESTCOPYSTREAMCONVERTER_TAG, allocator) TestCopyStreamConverter(copyStream, maximumsuccessfulOperationCount);

    CODING_ERROR_ASSERT(value != nullptr);

    return TestCopyStreamConverter::SPtr(value);
}

Awaitable<NTSTATUS> TestCopyStreamConverter::GetOperationAsync(__out Data::LoggingReplicator::IOperation::SPtr & result)
{
    currentOperationCount_++;
    if (currentOperationCount_ > maximumsuccessfulOperationCount_) {
        result = nullptr;
        co_return STATUS_SUCCESS;
    }

    OperationData::CSPtr op;
    co_await copyStream_->GetNextAsync(CancellationToken::None, op);

    if (op == nullptr)
    {
        result = nullptr;
        co_return STATUS_SUCCESS;
    }
    
    TestOperation::SPtr testOp = TestOperation::Create(*op, GetThisAllocator());
    result = testOp.RawPtr();

    co_return STATUS_SUCCESS;
}
