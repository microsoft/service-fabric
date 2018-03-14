// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Hosting;

ServiceTypeMap::ServiceTypeMap(ReconfigurationAgent & ra) :
numberOfEntriesInMessage_(ra.Config.MaxNumberOfServiceTypeInMessageToFMEntry),
ra_(ra)
{
}

void ServiceTypeMap::OnServiceTypeRegistrationAdded(
    std::wstring const& activityId,
    Hosting2::ServiceTypeRegistrationSPtr const& registration,
    Infrastructure::BackgroundWorkManagerWithRetry& bgmr)
{
    bool isSendRequired = false;

    Add(activityId, registration, isSendRequired);

    if (isSendRequired)
    {
        bgmr.Request(activityId);
    }
}

void ServiceTypeMap::OnServiceTypeRegistrationRemoved(
    std::wstring const& activityId,
    Hosting2::ServiceTypeRegistrationSPtr const& registration)
{
    AcquireWriteLock grab(lock_);

    auto it = map_.find(registration->ServiceTypeId);
    if (it == map_.end())
    {
        Assert::TestAssert("Entity must exist {0} {1}", activityId, registration);
        return;
    }

    bool isRemoveRequired = false;
    it->second->OnRegistrationRemoved(activityId, isRemoveRequired);

    if (isRemoveRequired)
    {
        map_.erase(it);
    }
}

void ServiceTypeMap::OnWorkManagerRetry(
    std::wstring const& activityId,
    Infrastructure::BackgroundWorkManagerWithRetry& bgmr,
    ReconfigurationAgent& ra) const
{
    // TODO: This logic needs to be moved out of here
    bool isPending = false;
    
    typedef Infrastructure::EntityRetryComponent<ServiceTypeInfo>::RetryContext RetryContext;
    vector<pair<ServiceTypeMapEntitySPtr, RetryContext>> retryContexts;

    auto generationTime = Stopwatch::Now();

    // First generate all the retry contexts
    {
        AcquireReadLock grab(lock_);

        for (auto const & it : map_)
        {
            auto serviceType = it.second;
            
            serviceType->UpdateFlagIfRetryPending(isPending);

            auto retryContext = serviceType->CreateRetryContext(generationTime);
            if (retryContext.HasData)
            {
                retryContexts.push_back(make_pair(move(serviceType), move(retryContext)));
            }
        }
    }

    // Generate the data that goes into the message
    vector<ServiceTypeInfo> serviceTypeInfos;
    size_t maxIndex = min(static_cast<size_t>(numberOfEntriesInMessage_.GetValue()), retryContexts.size());
    for (int i = 0; i < maxIndex; i++)
    {
        retryContexts[i].second.Generate(serviceTypeInfos);
    }

    if (!serviceTypeInfos.empty())
    {
        ServiceTypeNotificationRequestMessageBody body(move(serviceTypeInfos));
        ra.FMTransportObj.SendServiceTypeInfoMessage(activityId, body);
    }

    // Tell the retry contexts the time of the retry
    auto retryTime = Stopwatch::Now();
    {
        AcquireWriteLock grab(lock_);
        for (int i = 0; i < maxIndex; i++)
        {
            retryContexts[i].first->OnRetry(retryContexts[i].second, retryTime);
        }
    }
    
    // Complete the bgmr
    bgmr.OnWorkComplete(isPending);
}

void ServiceTypeMap::OnReplyFromFM(std::vector<ServiceTypeInfo> const & infos)
{
    AcquireWriteLock grab(lock_);
    for (auto const & it : infos)
    {
        auto element = map_.find(it.VersionedServiceTypeId.Id);
        if (element != map_.end())
        {
            element->second->OnReply(it);
        }
    }
}

void ServiceTypeMap::Add(
    std::wstring const & activityId,
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    __out bool & isSendRequired)
{
    AcquireWriteLock grab(lock_);
    auto it = map_.find(registration->ServiceTypeId);
    if (it == map_.end())
    {
        auto entry = make_shared<ServiceTypeMapEntity>(ServiceTypeMapEntity(ra_.NodeInstance, registration, ra_.Config.PerServiceTypeMinimumIntervalBetweenMessageToFMEntry));
        auto result = map_.insert(make_pair(registration->ServiceTypeId, entry));
        ASSERT_IF(!result.second, "Element must have been inserted");
        it = result.first;
    }

    it->second->OnRegistrationAdded(activityId, registration, isSendRequired);
}
