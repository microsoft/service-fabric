// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

HostedServiceMap::HostedServiceMap()
    : lock_(),
    map_(),
    isClosed_(false)
{
}

HostedServiceMap::~HostedServiceMap()
{
}

ErrorCode HostedServiceMap::Add(HostedServiceSPtr const & hostedService)
{
    {
        AcquireWriteLock writeLock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(hostedService->ServiceName);
        if (iter != map_.end())
        {
            return ErrorCode(ErrorCodeValue::AlreadyExists);
        }
        else
        {
            map_.insert(make_pair(hostedService->ServiceName, hostedService));
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode HostedServiceMap::Contains(wstring const & serviceName, __out bool & contains)
{
    HostedServiceSPtr hostedService;
    auto error = this->Get(serviceName, hostedService);
    if (error.IsSuccess())
    {
        contains = true;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (error.IsError(ErrorCodeValue::SystemServiceNotFound))
    {
        contains = false;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        contains = false;
        return error;
    }
}

ErrorCode HostedServiceMap::Get(
    wstring const & serviceName, 
    __out HostedServiceSPtr & hostedService)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(serviceName);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::SystemServiceNotFound);
        }

        hostedService = iter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode HostedServiceMap::Remove(
    wstring const & serviceName)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(serviceName);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::SystemServiceNotFound);
        }

        map_.erase(iter);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

vector<HostedServiceSPtr> HostedServiceMap::Close()
{
    vector<HostedServiceSPtr> retval;

    {
        AcquireWriteLock writelock(lock_);
        if (!isClosed_)
        {
            for(auto i = map_.begin(); i != map_.end(); ++i)
            {
                retval.push_back(move(i->second));
            }

            map_.clear();
            isClosed_ = true;
        }
    }

    return retval;
}

ErrorCode HostedServiceMap::Perform(HostedServiceMap::Operation const & operation, std::wstring key)
{
    {
        HostedServiceSPtr hostedService = nullptr;
        {
            AcquireReadLock readlock(lock_);
            if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }
            auto iter = map_.find(key);
            if (iter == map_.end())
            {
                return ErrorCode(ErrorCodeValue::SystemServiceNotFound);
            }
            hostedService = iter->second;
        }
        if(hostedService)
        {
            operation(hostedService);
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}
