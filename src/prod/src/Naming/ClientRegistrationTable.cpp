// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ClientServerTransport;
using namespace Naming;

StringLiteral const TraceComponent("ClientRegistrationTable");

//
// ClientRegistration
//
    
ClientRegistration::ClientRegistration() 
    : notificationSender_()
    , clientId_()
    , registrationId_()
    , filtersById_()
    , isConnectionProcessingComplete_(false)
{ }

ClientRegistration::ClientRegistration(
    wstring const & traceId,
    EntreeServiceTransportSPtr const &transport, 
    ClientIdentityHeader const & clientIdentity,
    wstring && clientId,
    VersionRangeCollectionSPtr const & initialClientVersions,
    ServiceNotificationSender::TargetFaultHandler const & targetFaultHandler)
    : notificationSender_()
    , clientId_(move(clientId))
    , registrationId_(clientIdentity)
    , filtersById_()
    , isConnectionProcessingComplete_(false)
{
    notificationSender_ = make_shared<ServiceNotificationSender>(
        wformatString("[{0}+{1}]", traceId, registrationId_),
        transport,
        clientIdentity,
        initialClientVersions,
        targetFaultHandler,
        *this);
}

ErrorCode ClientRegistration::TryAddFilter(ServiceNotificationFilterSPtr const & filter)
{
    // Duplicate filter ID on the same client registration. Can happen due
    // to retries by the client.
    //
    auto result = filtersById_.insert(FilterPair(filter->FilterId, filter));
    if (!result.second)
    {
        WriteInfo(
            TraceComponent,
            "{0}: filter ID already exists: existing={1} incoming={2}",
            registrationId_,
            *(result.first),
            *filter);

        return ErrorCodeValue::ServiceNotificationFilterAlreadyExists;
    }

    return ErrorCodeValue::Success;
}

ServiceNotificationFilterSPtr ClientRegistration::TryRemoveFilter(uint64 filterId)
{
    auto it = filtersById_.find(filterId);

    if (it != filtersById_.end())
    {
        auto filter = it->second;

        filtersById_.erase(it);

        return filter;
    }
    else
    {
        return ServiceNotificationFilterSPtr();
    }
}

void ClientRegistration::StartSender()
{
    notificationSender_->Start();
}

void ClientRegistration::StopSender()
{
    notificationSender_->Stop();
}

bool ClientRegistration::IsSenderStopped() const
{
    return notificationSender_->IsStopped;
}

void ClientRegistration::CreateAndEnqueueNotification(
    ActivityId const & activityId,
    GenerationNumber const & cacheGeneration,
    VersionRangeCollectionSPtr && cacheVersions,
    MatchedServiceTableEntryMap && matchedPartitions,
    VersionRangeCollectionSPtr && updateVersions)
{
    notificationSender_->CreateAndEnqueueNotification(
        activityId,
        cacheGeneration,
        move(cacheVersions),
        move(matchedPartitions),
        move(updateVersions));
}

void ClientRegistration::CreateAndEnqueueNotificationForClientSynchronization(
    ActivityId const & activityId,
    GenerationNumber const & cacheGeneration,
    VersionRangeCollectionSPtr && cacheVersions,
    MatchedServiceTableEntryMap && matchedPartitions,
    VersionRangeCollectionSPtr && updateVersions)
{
    notificationSender_->CreateAndEnqueueNotificationForClientSynchronization(
        activityId,
        cacheGeneration,
        move(cacheVersions),
        move(matchedPartitions),
        move(updateVersions));
}

bool ClientRegistration::IsConnectionProcessingComplete()
{
    return isConnectionProcessingComplete_;
}

void ClientRegistration::SetConnectionProcessingComplete()
{
    isConnectionProcessingComplete_ = true;
}

//
// ClientRegistrationTable
//

ClientRegistrationTable::ClientRegistrationTable() : registrations_() { }

void ClientRegistrationTable::Clear()
{
    for (auto const & it : registrations_)
    {
        it.second->StopSender();
    }
    
    registrations_.clear();
}

bool ClientRegistrationTable::TryAddOrGetExistingRegistration(
    __inout ClientRegistrationSPtr & registration)
{
    auto result = registrations_.insert(RegistrationPair(registration->Id, registration));
    if (!result.second)
    {
        registration = result.first->second;
    }
    else
    {
        registration->StartSender();
    }

    return result.second;
}

ClientRegistrationSPtr ClientRegistrationTable::TryGetRegistration(
    ClientIdentityHeader const & clientIdentity, 
    wstring const & clientId)
{
    return this->TryGetRegistration(ClientRegistrationId(clientIdentity), clientId);
}

ClientRegistrationSPtr ClientRegistrationTable::TryGetRegistration(
    ClientRegistrationId const & clientIdentity,
    wstring const & clientId)
{
    auto it = registrations_.find(clientIdentity);

    if (it != registrations_.end())
    {
        if (clientId.empty() || it->second->ClientId == clientId)
        {
            return it->second;
        }
    }

    return ClientRegistrationSPtr();
}

ClientRegistrationSPtr ClientRegistrationTable::TryRemoveRegistration(
    ClientIdentityHeader const & clientIdentity,
    wstring const & clientId)
{
    return this->TryRemoveRegistration(ClientRegistrationId(clientIdentity), clientId);
}

ClientRegistrationSPtr ClientRegistrationTable::TryRemoveRegistration(
    ClientRegistrationId const & clientIdentity,
    wstring const & clientId)
{
    auto it = registrations_.find(clientIdentity);

    if (it != registrations_.end())
    {
        if (clientId.empty() || it->second->ClientId == clientId)
        {
            auto registration = it->second;

            registrations_.erase(it);

            registration->StopSender();

            return registration;
        }
    }

    return ClientRegistrationSPtr();
}
