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

#define TESTSTATEREPLICATOR_TAG 'peRT'

TestStateReplicator::TestStateReplicator(__in ApiFaultUtility & apiFaultUtility)
    : IStateReplicator()
    , KObject()
    , KShared()
    , lsn_(2) // replicator lsn is initialized to 1 during recovery. so new lsn must start from 2
    , failReplicateAfterLsn_(0)
    , apiFaultUtility_(&apiFaultUtility)
    , beginReplicateException_(STATUS_SUCCESS)
    , endReplicateException_(STATUS_SUCCESS)
    , copyStreamMaximumSuccessfulOperationCount_(MAXULONG32)
    , replicationSteramMaximumSuccessfulOperationCount_(MAXULONG32)
{
}

TestStateReplicator::~TestStateReplicator()
{
}

TestStateReplicator::SPtr TestStateReplicator::Create(
    __in ApiFaultUtility & faultUtility,
    __in KAllocator & allocator)
{
    TestStateReplicator * pointer = _new(TESTSTATEREPLICATOR_TAG, allocator) TestStateReplicator(faultUtility);
    CODING_ERROR_ASSERT(pointer != nullptr);

    return TestStateReplicator::SPtr(pointer);
}

CompletionTask::SPtr TestStateReplicator::ReplicateAsync(
    __in Data::Utilities::OperationData const & replicationData,
    __out LONG64 & logicalSequenceNumber)
{
    UNREFERENCED_PARAMETER(replicationData);

    logicalSequenceNumber = -1;
    CompletionTask::SPtr result = CompletionTask::Create(GetThisAllocator());

    if (!NT_SUCCESS(beginReplicateException_))
    {
        result->CompleteAwaiters(beginReplicateException_);
        return result;
    }

    logicalSequenceNumber = static_cast<LONG64>(lsn_++);

    LONG64 localLSN = logicalSequenceNumber;

    Common::Threadpool::Post([localLSN, result, this]
    {
        NTSTATUS status = apiFaultUtility_->WaitUntilSignaled(ApiName::ReplicateAsync);
        if (!NT_SUCCESS(status))
        {
            result->CompleteAwaiters(status);
        }
        else
        {
            status = this->ReturnErrorAfterLsnIfNeeded(localLSN);
            result->CompleteAwaiters(status);
        }
    });

    return result;
}

void LoggingReplicatorTests::TestStateReplicator::FailReplicationAfterLSN(
    __in LONG64 lsn, 
    __in NTSTATUS e)
{
    failReplicateAfterLsn_.store(static_cast<uint64>(lsn));
    endReplicateException_ = e;
}

void LoggingReplicatorTests::TestStateReplicator::SetBeginReplicateException(__in NTSTATUS e)
{
    beginReplicateException_ = e;
}

void TestStateReplicator::InitializePrimaryCopyStream(__in Data::LoggingReplicator::CopyStream & copyStream)
{
    copyStream_ = &copyStream;
}

NTSTATUS TestStateReplicator::GetCopyStream(__out Data::LoggingReplicator::IOperationStream::SPtr & result)
{
    TestCopyStreamConverter::SPtr testCopyStream = TestCopyStreamConverter::Create(*copyStream_, copyStreamMaximumSuccessfulOperationCount_, GetThisAllocator());
    result = testCopyStream.RawPtr();
    return STATUS_SUCCESS;
}

NTSTATUS TestStateReplicator::GetReplicationStream(__out Data::LoggingReplicator::IOperationStream::SPtr & result)
{
    TestCopyStreamConverter::SPtr testReplicationStream = TestCopyStreamConverter::Create(*copyStream_, replicationSteramMaximumSuccessfulOperationCount_, GetThisAllocator());
    result = testReplicationStream.RawPtr();
    return STATUS_SUCCESS;
}

int64 LoggingReplicatorTests::TestStateReplicator::GetMaxReplicationMessageSize()
{
    return 0;
}

NTSTATUS LoggingReplicatorTests::TestStateReplicator::ReturnErrorAfterLsnIfNeeded(__in uint64 lsn)
{
    auto currentAllowedLsn = failReplicateAfterLsn_.load();
    if(currentAllowedLsn == 0 || lsn <= currentAllowedLsn)
    {
        return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(endReplicateException_))
    {
        return endReplicateException_;
    }

    return STATUS_SUCCESS;
}
