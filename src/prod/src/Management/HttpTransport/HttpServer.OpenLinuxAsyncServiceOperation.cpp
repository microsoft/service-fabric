// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpServer;
using namespace Transport;

bool HttpServerImpl::OpenLinuxAsyncServiceOperation::StartLinuxServiceOperation()
{   
     return server_->StartOpen(listenUri_, reqHandler_);
}

ErrorCode HttpServerImpl::OpenLinuxAsyncServiceOperation::End(__in AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<OpenLinuxAsyncServiceOperation>(operation);
    return thisPtr->Error;
}

