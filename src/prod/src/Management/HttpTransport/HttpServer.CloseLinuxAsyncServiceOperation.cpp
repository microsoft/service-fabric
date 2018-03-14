// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpServer;

bool HttpServerImpl::CloseLinuxAsyncServiceOperation::StartLinuxServiceOperation()
{
    return server_->StartClose();
}

ErrorCode HttpServerImpl::CloseLinuxAsyncServiceOperation::End(__in AsyncOperationSPtr const& operation)
{
    auto thisPtr = AsyncOperation::End<CloseLinuxAsyncServiceOperation>(operation);
    return thisPtr->Error;
}
