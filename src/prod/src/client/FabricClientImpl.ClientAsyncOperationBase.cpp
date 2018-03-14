// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace std;

using namespace Client;

FabricClientImpl::ClientAsyncOperationBase::ClientAsyncOperationBase(
    __in FabricClientImpl & client,
    Transport::FabricActivityHeader && activityHeader,
    Common::ErrorCode && passThroughError,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const& callback,
    Common::AsyncOperationSPtr const& parent)
    : AsyncOperation(callback, parent)
    , client_(client)
    , activityHeader_(move(activityHeader))
    , passThroughError_(move(passThroughError))
    , timeoutHelper_(timeout)
{
}

void FabricClientImpl::ClientAsyncOperationBase::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!passThroughError_.IsSuccess())
    {
        this->TryComplete(thisSPtr, passThroughError_);
    }
    else
    {
        this->OnStartOperation(thisSPtr);
    }
}
