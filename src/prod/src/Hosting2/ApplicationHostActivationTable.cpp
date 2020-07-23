// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management::ResourceMonitor;

StringLiteral const TraceType("ApplicationHostTable");

ApplicationHostActivationTable::ApplicationHostActivationTable(ComponentRoot const & root, HostingSubsystem & hosting)
    : RootedObject(root),
    isClosed_(false),
    lock_(),
    map_(),
    hostIdIndex_(),
    codePackageInstanceIdIndexMap_(),
    hosting_(hosting),
    pendingRMUpdates_(),
    ongoingRMUpdates_(),
    timerRM_(),
    hostIdRM_()
{
}

ApplicationHostActivationTable::~ApplicationHostActivationTable()
{
}

ErrorCode ApplicationHostActivationTable::Add(
    ApplicationHostProxySPtr const & hostProxy)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(hostProxy->AppHostIsolationContext);
        if (iter != map_.end())
        {
            return ErrorCode(ErrorCodeValue::AlreadyExists);
        }

        map_.insert(iter, make_pair(hostProxy->AppHostIsolationContext, hostProxy));

        try
        {
            hostIdIndex_.insert(make_pair(hostProxy->HostId, hostProxy));

            if (hostProxy->HostType == ApplicationHostType::Enum::Activated_SingleCodePackage)
            {
                auto singlePkgAppHostProxy = static_cast<SingleCodePackageApplicationHostProxy*> (hostProxy.get());
                codePackageInstanceIdIndexMap_.insert(make_pair(singlePkgAppHostProxy->CodePackageInstanceId.ToString(), hostProxy));
            }
        }
        catch(...)
        {
            map_.erase(iter);
            hostIdIndex_.erase(hostProxy->HostId);
            throw;
        }

        AddUpdateForRM(hostProxy, true);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostActivationTable::Remove(
    wstring const & hostId, 
    __out ApplicationHostProxySPtr & hostProxy)
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto hostIdIter = hostIdIndex_.find(hostId);
        if (hostIdIter == hostIdIndex_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        auto iter = map_.find(hostIdIter->second->AppHostIsolationContext);
        ASSERT_IF(iter == map_.end(), "The map must contain an entry for {0}", hostIdIter->second->AppHostIsolationContext);

        hostProxy = hostIdIter->second;

        hostIdIndex_.erase(hostIdIter);
        map_.erase(iter);

        if (hostProxy->HostType == ApplicationHostType::Enum::Activated_SingleCodePackage)
        {
            auto singlePkgAppHostProxy = static_cast<SingleCodePackageApplicationHostProxy*> (hostProxy.get());
            codePackageInstanceIdIndexMap_.erase(singlePkgAppHostProxy->CodePackageInstanceId.ToString());
        }

        AddUpdateForRM(hostProxy, false);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostActivationTable::FindApplicationHostByCodePackageInstanceId(
    wstring const & codePackageInstanceId,
    __out ApplicationHostProxySPtr & hostProxy)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto codePackageInstanceIdIter = codePackageInstanceIdIndexMap_.find(codePackageInstanceId);
        if (codePackageInstanceIdIter == codePackageInstanceIdIndexMap_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        hostProxy = codePackageInstanceIdIter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostActivationTable::Find(
    wstring const & hostId,
    __out ApplicationHostProxySPtr & hostProxy)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto hostIdIter = hostIdIndex_.find(hostId);
        if (hostIdIter == hostIdIndex_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        hostProxy = hostIdIter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ApplicationHostActivationTable::Find(
    ApplicationHostIsolationContext const & isolationContext,
    __out ApplicationHostProxySPtr & hostProxy)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(isolationContext);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        hostProxy = iter->second;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

vector<ApplicationHostProxySPtr> ApplicationHostActivationTable::Close()
{
    vector<ApplicationHostProxySPtr> removed;

    {
        AcquireWriteLock lock(lock_);
        if (!isClosed_)
        {
            if (timerRM_)
            {
                timerRM_->Cancel();
                timerRM_.reset();
            }

            for (auto iter = map_.cbegin(); iter != map_.cend(); ++iter)
            {
                removed.push_back(iter->second);
            }
            isClosed_ = true;
        }

        map_.clear();
        hostIdIndex_.clear();
    }

    return removed;
}

void ApplicationHostActivationTable::AddUpdateForRM(ApplicationHostProxySPtr const & appProxy, bool isUp)
{
    //we are adding updates in order and consuming them in such a way by RM
    //by doing so if we send two updates for the same host we will always consider the latest update in RM
    if (appProxy->HostType == ApplicationHostType::Enum::Activated_SingleCodePackage && !hostIdRM_.empty())
    {
        //RM will monitor only single code package app hosts
        auto appHost = static_cast<SingleCodePackageApplicationHostProxy*> (appProxy.get());
        std::wstring appHostId;

        bool linuxContainerIsolation = false;
        //for processes we currently do not support Linux
        if (appHost->EntryPointType == ServiceModel::EntryPointType::Exe)
        {
#if !defined (PLATFORM_UNIX)
            appHostId = appHost->HostId;
#endif
        }
        else if (appHost->EntryPointType == ServiceModel::EntryPointType::ContainerHost)
        {
            appHostId = appHost->HostId;

            linuxContainerIsolation = appHost->GetLinuxContainerIsolation();
        }
        if (!appHostId.empty())
        {
            pendingRMUpdates_.push_back(ApplicationHostEvent(
                appHost->CodePackageInstanceId,
                appHost->ApplicationName,
                appHost->EntryPointType,
                appHostId,
                isUp,
                linuxContainerIsolation));
        }
    }
}

void ApplicationHostActivationTable::StartReportingToRM(std::wstring const & hostIdRM)
{
    AcquireWriteLock lock(lock_);

    auto root = Root.CreateComponentRoot();

    if (!timerRM_)
    {
        timerRM_ = Timer::Create(
            "Hosting.ApplicationHostActivationTable.RM",
            [this, root](TimerSPtr const & timer)
            {
                TimerCallbackRM(timer);
            });
    }

    pendingRMUpdates_.clear();

    //cache the host id for RM service so that we can periodically send application host changes
    hostIdRM_ = hostIdRM;
    
    for (auto & host : map_)
    {
        AddUpdateForRM(host.second, true);
    }

    timerRM_->Change(ResourceMonitor::ResourceMonitorServiceConfig::GetConfig().ApplicationHostEventsInterval);
}

void ApplicationHostActivationTable::TimerCallbackRM(TimerSPtr const & timer)
{
    std::vector<ApplicationHostEvent> allHostEvents;

    {
        AcquireWriteLock lock(lock_);

        if (isClosed_)
        {
            timer->Cancel();
            return;
        }

        if (!hostIdRM_.empty())
        {
            //we want to append the pending updates to the ongoing updates as there could be ongoing ones if last send failed
            for (auto & pending : pendingRMUpdates_)
            {
                ongoingRMUpdates_.push_back(move(pending));
            }

            //clear pending updates as all are moved to ongoing
            pendingRMUpdates_.clear();
            allHostEvents = ongoingRMUpdates_;
        }
    }

    if (allHostEvents.size() > 0)
    {
        WriteInfo(TraceType, "Sending {0} host events to RM", allHostEvents.size());

        auto request = CreateApplicationHostEventMessage(move(allHostEvents));

        auto operation = hosting_.IpcServerObj.BeginRequest(
            move(request),
            hostIdRM_,
            Common::TimeSpan::FromSeconds(5),
            [this, timer](AsyncOperationSPtr const & operation) { this->OnRequestSentToRM(operation, false, timer); },
            Root.CreateAsyncOperationRoot());

        this->OnRequestSentToRM(operation, true, timer);
    }
    else
    {
        timerRM_->Change(ResourceMonitor::ResourceMonitorServiceConfig::GetConfig().ApplicationHostEventsInterval);
    }
}

void ApplicationHostActivationTable::OnRequestSentToRM(AsyncOperationSPtr operation, bool expectedCompletedSynchronously, TimerSPtr const & timer)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    Transport::MessageUPtr reply;
    auto error = hosting_.IpcServerObj.EndRequest(operation, reply);

    {
        AcquireWriteLock lock(lock_);

        if (error.IsSuccess())
        {
            ongoingRMUpdates_.clear();
            WriteInfo(TraceType, "Successfully sent RM updates");
        }
        else
        {   
            WriteWarning(TraceType, "Failed to send application host events to RM error {0}", error);
        }

        timer->Change(ResourceMonitor::ResourceMonitorServiceConfig::GetConfig().ApplicationHostEventsInterval);
    }
}

Transport::MessageUPtr ApplicationHostActivationTable::CreateApplicationHostEventMessage(std::vector<ApplicationHostEvent> && allHostEvents)
{
    HostUpdateRM requestBody(move(allHostEvents));

    Transport::MessageUPtr request = make_unique<Transport::Message>(requestBody);
    request->Headers.Add(Transport::ActorHeader(Transport::Actor::ResourceMonitor));
    request->Headers.Add(Transport::ActionHeader(HostUpdateRM::HostUpdateRMAction));

    return move(request);
}

void ApplicationHostActivationTable::Test_GetAllRMReports(std::vector<ApplicationHostEvent> & pending, std::vector<ApplicationHostEvent> & ongoing)
{
    AcquireWriteLock lock(lock_);

    pending = pendingRMUpdates_;
    ongoing = ongoingRMUpdates_;
}

void ApplicationHostActivationTable::Test_GetHostProxyMap(HostProxyMap & proxyMap)
{
    AcquireWriteLock readLock(lock_);
    proxyMap = map_;
}
