// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace TxnReplicator;
using namespace TxnReplicator::TestCommon;
using namespace Data;
using namespace Data::Utilities;

ComPointer<IFabricDataLossHandler> TestComProxyDataLossHandler::Create(
    __in KAllocator & allocator,
    __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler)
{
    TestComProxyDataLossHandler * pointer = _new(TEST_COMPROXY_DATALOSS_HANDLER_TAG, allocator) TestComProxyDataLossHandler(dataLossHandler);
    ASSERT_IF(pointer == nullptr, "OOM while allocating TestComProxyDataLossHandler");
    ASSERT_IFNOT(NT_SUCCESS(pointer->Status()), "Unsuccessful initialization while allocating TestComProxyDataLossHandler {0}", pointer->Status());

    ComPointer<IFabricDataLossHandler> result;
    result.SetAndAddRef(pointer);
    return result;
}

HRESULT TestComProxyDataLossHandler::BeginOnDataLoss(
    /* [in] */ IFabricAsyncOperationCallback * callback,
    /* [retval][out] */ IFabricAsyncOperationContext ** context)
{
    if (callback == NULL || context == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    TestAsyncOnDataLossContext::SPtr asyncOperation = TestAsyncOnDataLossContext::Create(GetThisAllocator(), dataLossHandlerSPtr_.RawPtr());

    return Ktl::Com::AsyncCallInAdapter::CreateAndStart(
        GetThisAllocator(),
        Ktl::Move(asyncOperation),
        callback,
        context,
        [&](Ktl::Com::AsyncCallInAdapter&,
            TestAsyncOnDataLossContext& operation,
            KAsyncContextBase::CompletionCallback completionCallback) -> HRESULT
    {
        return operation.StartOnDataLoss(
            nullptr,
            completionCallback);
    });
}

HRESULT TestComProxyDataLossHandler::EndOnDataLoss(
    /* [in] */ IFabricAsyncOperationContext * context,
    /* [retval][out] */ BOOLEAN * isStateChanged)
{
    if (!context || !isStateChanged)
    {
        return E_POINTER;
    }

    TestAsyncOnDataLossContext::SPtr asyncOperation;
    HRESULT hr = Ktl::Com::AsyncCallInAdapter::End(context, asyncOperation);

    if (SUCCEEDED(hr))
    {
        hr = asyncOperation->GetResult(*isStateChanged);
    }

    return hr;
}

TestComProxyDataLossHandler::TestComProxyDataLossHandler(
    __in_opt TxnReplicator::IDataLossHandler * const dataLossHandler)
    : KObject()
    , KShared()
    , dataLossHandlerSPtr_(dataLossHandler)
{
}

TestComProxyDataLossHandler::~TestComProxyDataLossHandler()
{
}
