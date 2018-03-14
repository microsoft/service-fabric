// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;

TestAsyncOnDataLossContext::TestAsyncOnDataLossContext(
    __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler)
    : dataLossHandlerSPtr_(dataLossHandler)
{
}

TestAsyncOnDataLossContext::~TestAsyncOnDataLossContext()
{
}

TestAsyncOnDataLossContext::SPtr TestAsyncOnDataLossContext::Create(
    __in KAllocator & allocator,
    __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler)
{
    TestAsyncOnDataLossContext::SPtr tmpPtr(_new(TEST_ASYNC_ONDATALOSS_CONTEXT_TAG, allocator)TestAsyncOnDataLossContext(dataLossHandler));

    ASSERT_IF(tmpPtr == nullptr, "Out of memory trying to allocate TestAsyncOnDataLossContext");
    ASSERT_IFNOT(NT_SUCCESS(tmpPtr->Status()), "TestAsyncOnDataLossContext status not success {0}", tmpPtr->Status());

    return tmpPtr;
}

HRESULT TestAsyncOnDataLossContext::StartOnDataLoss(
    __in_opt KAsyncContextBase * const parentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback callback)
{
    Start(parentAsyncContext, callback);

    return S_OK;
}

void TestAsyncOnDataLossContext::OnStart()
{
    DoWorkAsync();
}

HRESULT TestAsyncOnDataLossContext::GetResult(
    __out BOOLEAN & isStateChanged)
{
    isStateChanged = isStateChanged_;
    return Result();
}

ktl::Task TestAsyncOnDataLossContext::DoWorkAsync() noexcept
{
    KCoShared$ApiEntry();

    if (dataLossHandlerSPtr_ == nullptr)
    {
        isStateChanged_ = false;

        Complete(STATUS_SUCCESS);
        co_return;
    }

    try
    {
        isStateChanged_ = co_await dataLossHandlerSPtr_->DataLossAsync(ktl::CancellationToken::None);
    }
    catch (ktl::Exception & exception)
    {
        Complete(exception.GetStatus());
        co_return;
    }

    Complete(STATUS_SUCCESS);
}
