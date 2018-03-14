// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

LockedFailoverUnitProxyPtr::LockedFailoverUnitProxyPtr(
    FailoverUnitProxySPtr const & entry)
    : entry_(entry)
{
    if (entry_)
    {
        entry_->AcquireLock();
    }
}

LockedFailoverUnitProxyPtr::~LockedFailoverUnitProxyPtr()
{
    if (entry_)
    {
        entry_->ReleaseLock();
        entry_ = nullptr;
    }
}

LockedFailoverUnitProxyPtr::operator bool() const
{
    return (entry_ != nullptr);
}

FailoverUnitProxy const* LockedFailoverUnitProxyPtr::operator->() const
{
    return get();
}

FailoverUnitProxy const& LockedFailoverUnitProxyPtr::operator*() const
{
    return *get();
}

FailoverUnitProxy * LockedFailoverUnitProxyPtr::operator->()
{
    return get();
}

FailoverUnitProxy * LockedFailoverUnitProxyPtr::get() const
{
    return entry_.get();
}
