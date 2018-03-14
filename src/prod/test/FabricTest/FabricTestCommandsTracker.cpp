// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricTest;

FabricTestCommandsTracker::CommandHandle::CommandHandle()
    : wait_(false)
    , result_(false)
{
}

bool FabricTestCommandsTracker::CommandHandle::WaitForResult(TimeSpan const & timeout)
{
    return wait_.WaitOne(timeout) ? result_ : false;
}

void FabricTestCommandsTracker::CommandHandle::SetResult(bool result)
{
    result_ = result;
    wait_.Set();
}

FabricTestCommandsTracker::FabricTestCommandsTracker()
    : handles_()
    , handlesLock_()
{
}

bool FabricTestCommandsTracker::RunAsync(CommandCallback const & command)
{
    auto handle = make_shared<CommandHandle>();
    {
        AcquireExclusiveLock lock(handlesLock_);

        handles_.push_back(handle);
    }

    Threadpool::Post([this, handle, command]()
    {
        bool result = command();
        handle->SetResult(result);
    });

    return true;
}

bool FabricTestCommandsTracker::WaitForAll(TimeSpan const & timeout)
{
    TimeoutHelper timeoutHelper(timeout);

    vector<shared_ptr<CommandHandle>> handles;
    {
        AcquireExclusiveLock lock(handlesLock_);

        handles.swap(handles_);
    }

    bool result = true;
    for (auto const & handle : handles)
    {
        result = (result && handle->WaitForResult(timeoutHelper.GetRemainingTime()));
    }

    return result;
}

bool FabricTestCommandsTracker::IsEmpty() const
{
    AcquireExclusiveLock lock(handlesLock_);

    return handles_.empty();
}
