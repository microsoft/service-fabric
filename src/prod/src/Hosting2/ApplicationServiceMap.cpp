// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ApplicationServiceMap::ApplicationServiceMap()
    : lock_(),
    containersLock_(),
    map_(),
    containersMap_(),
    isClosed_(false)
{
}

ApplicationServiceMap::~ApplicationServiceMap()
{
}

ErrorCode ApplicationServiceMap::Add(ApplicationServiceSPtr const & applicationService)
{
    if (applicationService->IsContainerHost)
    {
        AcquireWriteLock writeLock(containersLock_);
        return AddCallerHoldsLock(applicationService, containersMap_);
    }
    else
    {
        AcquireWriteLock writeLock(lock_);
        return AddCallerHoldsLock(applicationService, map_);
    }
        
}

ErrorCode ApplicationServiceMap::AddCallerHoldsLock(
    ApplicationServiceSPtr const & applicationService,
    map<wstring, map<std::wstring, ApplicationServiceSPtr>> & map)
{
    if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

    auto iter = map.find(applicationService->ParentId);
    if (iter == map.end())
    {
        map.insert(make_pair(applicationService->ParentId, std::map<wstring, ApplicationServiceSPtr>()));
    }
    else
    {
        auto iter2 = iter->second.find(applicationService->ApplicationServiceId);
        if (iter2 != iter->second.end())
        {
            return ErrorCode(ErrorCodeValue::UserServiceAlreadyExists);
        }
    }
    map[applicationService->ParentId].insert(make_pair(applicationService->ApplicationServiceId, applicationService));

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationServiceMap::Contains(
    wstring const & parentId,
    wstring const & applicationId,
    __out bool & contains)
{
    ApplicationServiceSPtr appService;
    auto error = this->Get(parentId, applicationId, appService);
    if (error.IsSuccess())
    {
        contains = true;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (error.IsError(ErrorCodeValue::ServiceNotFound))
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

ErrorCode ApplicationServiceMap::ContainsParentId(
    wstring const & parentId,
    __out bool & contains)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(parentId);
        if (iter != map_.end())
        {
            contains = true;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
    {
        AcquireReadLock lock(containersLock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = containersMap_.find(parentId);
        if (iter != containersMap_.end())
        {
            contains = true;
        }
        else
        {
            contains = false;
        }
    }
    return ErrorCode(ErrorCodeValue::Success);
}

vector<ApplicationServiceSPtr> ApplicationServiceMap::RemoveAllByParentId(wstring const & parentId)
{
    vector<ApplicationServiceSPtr> applicationServices;
    {
        AcquireWriteLock lock(lock_);
        if (!isClosed_)
        {
            auto iter = map_.find(parentId);
            if(iter != map_.end())
            {
                for(auto it = iter->second.begin(); it != iter->second.end(); ++it)
                {
                    applicationServices.push_back(it->second);
                }
                map_.erase(iter);
            }
        }
    }
    {
        AcquireWriteLock lock(containersLock_);
        if (!isClosed_)
        {
            auto iter = containersMap_.find(parentId);
            if (iter != containersMap_.end())
            {
                for (auto it = iter->second.begin(); it != iter->second.end(); ++it)
                {
                    applicationServices.push_back(it->second);
                }
                containersMap_.erase(iter);
            }
        }
    }
    return applicationServices;
}

ErrorCode ApplicationServiceMap::Get(
    wstring const & parentId,
    wstring const & applicationId,
    __out ApplicationServiceSPtr & appService)
{
    {
        AcquireReadLock lock(lock_);
        auto error = GetCallerHoldsLock(
            parentId,
            applicationId,
            map_,
            appService);
        if (error.IsSuccess())
        {
            return error;
        }
    }
    {
        AcquireReadLock lock(containersLock_);
        return GetCallerHoldsLock(
            parentId,
            applicationId,
            containersMap_,
            appService);
    }
}

ErrorCode ApplicationServiceMap::GetCallerHoldsLock(
    wstring const & parentId,
    wstring const & applicationId,
    map<wstring, map<std::wstring, ApplicationServiceSPtr>> & map,
    __out ApplicationServiceSPtr & appService)
{
    {
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map.find(parentId);
        if (iter == map.end())
        {
            return ErrorCode(ErrorCodeValue::ServiceNotFound);
        }

        auto iter2 = iter->second.find(applicationId);
        if (iter2 != iter->second.end())
        {
            appService = iter2->second;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
    return ErrorCode(ErrorCodeValue::ServiceNotFound);
}

ErrorCode ApplicationServiceMap::Remove(
    wstring const & parentId,
    wstring const & applicationId)
{
    ApplicationServiceSPtr appService;
    return Remove(parentId, applicationId, appService);
}

ErrorCode ApplicationServiceMap::Remove(
    wstring const & parentId,
    wstring const & applicationId,
    ApplicationServiceSPtr & appService)
{
    {
        AcquireWriteLock lock(lock_);
        auto error = RemoveCallerHoldsLock(
            parentId,
            applicationId,
            map_,
            appService);
        if (error.IsSuccess() || error.IsError(ErrorCodeValue::ObjectClosed))
        {
            return error;
        }
    }
    {
        AcquireWriteLock lock(containersLock_);
        return RemoveCallerHoldsLock(
            parentId,
            applicationId,
            containersMap_,
            appService);
    }
}

ErrorCode ApplicationServiceMap::RemoveCallerHoldsLock(
    wstring const & parentId,
    wstring const & applicationId,
    map<wstring, map<std::wstring, ApplicationServiceSPtr>> & map,
    ApplicationServiceSPtr & appService)
{
    if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

    auto iter = map.find(parentId);
    if (iter == map.end())
    {
        return ErrorCode(ErrorCodeValue::SystemServiceNotFound);
    }
    auto iter2 = iter->second.find(applicationId);
    if (iter2 == iter->second.end())
    {
        return ErrorCode(ErrorCodeValue::SystemServiceNotFound);
    }
    else
    {
        appService = iter2->second;
    }
    iter->second.erase(iter2);
    return ErrorCode(ErrorCodeValue::Success);
}

vector<ApplicationServiceSPtr> ApplicationServiceMap::Close()
{
    vector<ApplicationServiceSPtr> retval;

    {
        AcquireWriteLock writelock(lock_);
        AcquireWriteLock writeLock(containersLock_);
        if (!isClosed_)
        {
            for(auto i = map_.begin(); i != map_.end(); ++i)
            {
                for(auto it = i->second.begin(); it != i->second.end(); ++it)
                {
                    retval.push_back(move(it->second));
                }
            }
            map_.clear();
            for (auto i = containersMap_.begin(); i != containersMap_.end(); ++i)
            {
                for (auto it = i->second.begin(); it != i->second.end(); ++it)
                {
                    retval.push_back(move(it->second));
                }
            }
            containersMap_.clear();
            isClosed_ = true;
        }
    }

    return retval;
}

map<wstring, map<wstring, ApplicationServiceSPtr>> ApplicationServiceMap::RemoveAllContainerServices()
{
    map<wstring, std::map<wstring, ApplicationServiceSPtr>> retval;
    {
        AcquireWriteLock writeLock(containersLock_);
        if (!isClosed_)
        {
            for (auto i = containersMap_.begin(); i != containersMap_.end(); ++i)
            {
                //retval.push_back(move(*i));
                retval[move(i->first)] = move(i->second);
            }
            containersMap_.clear();
        }
    }

    return retval;
}

map<wstring, map<wstring, ApplicationServiceSPtr>> ApplicationServiceMap::GetAllServices(bool forContainer)
{
    if (forContainer)
    {
        AcquireWriteLock writeLock(containersLock_);
        return containersMap_;
    }
    else
    {
        AcquireWriteLock writelock(lock_);
        return map_;
    }
}

void ApplicationServiceMap::GetCount(__out size_t & containersCount, __out size_t & applicationsCount)
{
    {
        AcquireWriteLock writelock(lock_);
        AcquireWriteLock writeLock(containersLock_);
        if (!isClosed_)
        {
            //For ScaleMin scenario, the count will be the total sum of applications and containers on all the nodes.
            for (auto iter = containersMap_.begin(); iter != containersMap_.end(); ++iter)
            {
                containersCount += iter->second.size();
            }

            for (auto iter = map_.begin(); iter != map_.end(); ++iter)
            {
                applicationsCount += iter->second.size();
            }
        }
    }
}
