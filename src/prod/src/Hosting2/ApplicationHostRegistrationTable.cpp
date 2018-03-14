// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ApplicationHostRegistrationTable::ApplicationHostRegistrationTable(
    ComponentRoot const & root)
    : RootedObject(root),
    isClosed_(false),
    lock_(),
    map_()
{
}

ApplicationHostRegistrationTable::~ApplicationHostRegistrationTable()
{
}

ErrorCode ApplicationHostRegistrationTable::Add(ApplicationHostRegistrationSPtr const & registration)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(registration->HostId);
        if (iter != map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingApplicationHostAlreadyRegistered);
        }
        map_.insert(make_pair(registration->HostId, registration));
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostRegistrationTable::Remove(wstring const & hostId, __out ApplicationHostRegistrationSPtr & registration)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(hostId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingApplicationHostNotRegistered);
        }

        registration = iter->second;
        map_.erase(iter);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostRegistrationTable::Find(wstring const & hostId, __out ApplicationHostRegistrationSPtr & registration)
{
     {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(hostId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingApplicationHostNotRegistered);
        }

        registration = iter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

vector<ApplicationHostRegistrationSPtr> ApplicationHostRegistrationTable::Close()
{
    vector<ApplicationHostRegistrationSPtr> removed;
    {
        AcquireWriteLock lock(lock_);
        for(auto iter = map_.begin(); iter != map_.end(); ++iter)
        {
            removed.push_back(iter->second);
        }
        map_.clear();
        isClosed_ = true;
    }

    return removed;
}
