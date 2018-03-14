// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceGroupMemberQueryResult::ServiceGroupMemberQueryResult() :
    serviceName_(),
    serviceGroupMemberDescriptions_()
{
}

ServiceGroupMemberQueryResult::ServiceGroupMemberQueryResult(
    Uri const & serviceName,
    vector<CServiceGroupMemberDescription> & serviceGroupMemberDescriptions) :
    serviceName_(serviceName),
    serviceGroupMemberDescriptions_(serviceGroupMemberDescriptions)
{
}

ServiceGroupMemberQueryResult::ServiceGroupMemberQueryResult(ServiceGroupMemberQueryResult && other) :
    serviceName_(move(other.serviceName_)),
    serviceGroupMemberDescriptions_(move(other.serviceGroupMemberDescriptions_))
{
}

ServiceGroupMemberQueryResult const & ServiceGroupMemberQueryResult::operator = (ServiceGroupMemberQueryResult && other)
{
    if (this != &other)
    {
        serviceName_ = move(other.serviceName_);
        serviceGroupMemberDescriptions_ = move(other.serviceGroupMemberDescriptions_);
    }

    return *this;
}

void ServiceGroupMemberQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_ITEM & publicServiceGroupMemberQueryResult) const
{
    publicServiceGroupMemberQueryResult.ServiceName = heap.AddString(serviceName_.ToString());
    
    ULONG length = static_cast<ULONG>(serviceGroupMemberDescriptions_.size());
    auto publicArray = heap.AddArray<FABRIC_SERVICE_GROUP_MEMBER_MEMBER_QUERY_RESULT_ITEM>(length);

    for (int i = 0; i < serviceGroupMemberDescriptions_.size(); i++)
    {
        publicArray[i].ServiceType = heap.AddString(serviceGroupMemberDescriptions_[i].ServiceType);
        publicArray[i].ServiceName = heap.AddString(serviceGroupMemberDescriptions_[i].ServiceName);
    }

    auto publicServiceGroupMembers = heap.AddItem<FABRIC_SERVICE_GROUP_MEMBER_MEMBER_QUERY_RESULT_LIST>();
    publicServiceGroupMembers->Count = length;
    publicServiceGroupMembers->Items = publicArray.GetRawArray();

    publicServiceGroupMemberQueryResult.Members = publicServiceGroupMembers.GetRawPointer();
}

ServiceGroupMemberQueryResult::~ServiceGroupMemberQueryResult()
{
}

ServiceGroupMemberQueryResult ServiceGroupMemberQueryResult::CreateServiceGroupMemberQueryResult(
    Uri const & serviceName,
    vector<CServiceGroupMemberDescription> & cServiceGroupMemberDescriptions)
{
    return ServiceGroupMemberQueryResult(serviceName, cServiceGroupMemberDescriptions);
}
