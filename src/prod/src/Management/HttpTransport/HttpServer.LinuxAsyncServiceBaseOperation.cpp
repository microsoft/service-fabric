// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpServer;

void HttpServerImpl::LinuxAsyncServiceBaseOperation::OnStart(__in AsyncOperationSPtr const& thisSPtr)
{
    //
    // Self reference.
    //
    thisSPtr_ = shared_from_this();

    StartLinuxServiceOperation();

    cout << "Start or Close server operation succeeded!" << endl; 
    
    TryComplete(thisSPtr, ErrorCode::Success());

    thisSPtr_ = nullptr;
    
}

void HttpServerImpl::LinuxAsyncServiceBaseOperation::OnCancel()
{
    // no op.
}


