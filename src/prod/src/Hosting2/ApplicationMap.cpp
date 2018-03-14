// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ApplicationMap::ApplicationMap(ComponentRoot const & root)
    : RootedObject(root),
    lock_(),
    map_(),
    isClosed_(true),
    notifyDcaTimer_()
{
}

ApplicationMap::~ApplicationMap()
{
}

void ApplicationMap::Open()
{
    isClosed_ = false;
    auto dcaNotificationInterval = HostingConfig::GetConfig().DcaNotificationIntervalInSeconds;
    auto root = Root.CreateComponentRoot();
    notifyDcaTimer_ = Timer::Create("Hosting.NotifyDcaAboutServicePackages", [this, root](TimerSPtr const &) { this->NotifyDcaAboutServicePackages(); }, false);
    notifyDcaTimer_->Change(dcaNotificationInterval);
}

ErrorCode ApplicationMap::Add(Application2SPtr const & application)
{
    {
        AcquireWriteLock writeLock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(application->Id);
        if (iter != map_.end())
        {
            return ErrorCode(ErrorCodeValue::ApplicationManagerApplicationAlreadyExists);
        }
        else
        {
            map_.insert(make_pair(application->Id, application));
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationMap::Contains(ApplicationIdentifier const & applicationId, __out bool & contains)
{
    Application2SPtr application;
    auto error = this->Get(applicationId, application);
    if (error.IsSuccess())
    {
        contains = true;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else if (error.IsError(ErrorCodeValue::ApplicationManagerApplicationNotFound))
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

ErrorCode ApplicationMap::Get(
    ApplicationIdentifier const & applicationId,
    __out Application2SPtr & application)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(applicationId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::ApplicationManagerApplicationNotFound);
        }

        application = iter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationMap::Remove(
    ApplicationIdentifier const & applicationId,
    int64 const instanceId,
    __out Application2SPtr & application)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(applicationId);
        if ((iter == map_.end()) || (iter->second->InstanceId != instanceId))
        {
            return ErrorCode(ErrorCodeValue::ApplicationManagerApplicationNotFound);
        }

        application = iter->second;
        map_.erase(iter);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

vector<Application2SPtr> ApplicationMap::Close()
{
    vector<Application2SPtr> retval;

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

    if (notifyDcaTimer_)
    {
        notifyDcaTimer_->Cancel();
    }
    return retval;
}

ErrorCode ApplicationMap::VisitUnderLock(ApplicationMap::Visitor const & visitor)
{
     {
        AcquireReadLock readlock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        for(auto iter = map_.begin(); iter != map_.end(); ++iter)
        {
            if (!visitor(iter->second))
            {
                break;
            }
        }
     }

     return ErrorCode(ErrorCodeValue::Success);
}

Common::ErrorCode ApplicationMap::GetApplications(__out std::vector<Application2SPtr> & applications, wstring const & filterApplicationName, wstring const & continuationToken)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        for(auto iter = map_.begin(); iter != map_.end(); ++iter)
        {
            // If the application name doesn't respect the continuation token, then skip this item
            if (!continuationToken.empty() && (continuationToken >= iter->second->AppName))
            {
                continue;
            }

            if(filterApplicationName.empty())
            {
                applications.push_back(iter->second);
            }
            else if(iter->second->AppName == filterApplicationName)
            {
                applications.push_back(iter->second);
                break;
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void ApplicationMap::NotifyDcaAboutServicePackages()
{
    vector<Application2SPtr> applications;
    auto error = GetApplications(applications);
    if (!error.IsSuccess())
    {
        return;
    }

    for (auto iter = applications.begin(); iter != applications.end(); ++iter)
    {
        (*iter)->NotifyDcaAboutServicePackages();
    }

    // Read the notification interval setting again, in case it has changed since the last time
    // we read it.
    auto dcaNotificationInterval = HostingConfig::GetConfig().DcaNotificationIntervalInSeconds;

    {
        AcquireReadLock lock(lock_);
        if (!isClosed_)
        {
            notifyDcaTimer_->Change(dcaNotificationInterval);
        }
    }
}
