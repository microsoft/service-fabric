// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;

ServiceType::ServiceType()
    : StoreData(),
      isStale_(false),
      idString_(),
      codePackages_()
{
}

ServiceType::ServiceType(
    ServiceModel::ServiceTypeIdentifier const& type,
    ApplicationEntrySPtr const& applicationEntry)
    : StoreData(PersistenceState::ToBeInserted),
      type_(type),
      serviceTypeDisabledNodes_(),
      codePackages_(),
      applicationEntry_(applicationEntry),
      lastUpdateTime_(DateTime::Zero),
      isStale_(false),
      idString_()
{
}

ServiceType::ServiceType(
    ServiceType const& other,
    ApplicationEntrySPtr const& applicationEntry)
    : StoreData(other),
      type_(other.type_),
      serviceTypeDisabledNodes_(other.serviceTypeDisabledNodes_),
      codePackages_(other.codePackages_),
      applicationEntry_(applicationEntry),
      lastUpdateTime_(other.lastUpdateTime_),
      isStale_(false),
      idString_()
{
}

ServiceType::ServiceType(
    ServiceType const & other,
    Federation::NodeId const & nodeId,
    ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated),
      type_(other.type_),
      serviceTypeDisabledNodes_(other.serviceTypeDisabledNodes_),
      codePackages_(other.codePackages_),
      applicationEntry_(other.applicationEntry_),
      lastUpdateTime_(other.lastUpdateTime_),
      isStale_(false),
      idString_()
{
    if (serviceTypeUpdateEvent == ServiceTypeUpdateKind::Enabled)
    {
        auto it = remove_if(
            serviceTypeDisabledNodes_.begin(),
            serviceTypeDisabledNodes_.end(),
            [&nodeId] (NodeId const& id) { return (id == nodeId); });
        serviceTypeDisabledNodes_.erase(it, serviceTypeDisabledNodes_.end());
    }
    else
    {
        auto it = find(serviceTypeDisabledNodes_.begin(), serviceTypeDisabledNodes_.end(), nodeId);
        if (it == serviceTypeDisabledNodes_.end())
        {
            serviceTypeDisabledNodes_.push_back(nodeId);
        }
    }

}

ServiceType::ServiceType(
    ServiceType const& other,
    ServiceModel::ServicePackageVersionInstance const& versionInstance,
    wstring const& codePackageName)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated)
    , type_(other.type_)
    , serviceTypeDisabledNodes_(other.serviceTypeDisabledNodes_)
    , codePackages_()
    , applicationEntry_(other.applicationEntry_)
    , lastUpdateTime_(other.lastUpdateTime_)
    , isStale_(false)
    , idString_()
    
{
    codePackages_[versionInstance] = codePackageName;
}

wstring const& ServiceType::GetStoreType()
{
    return FailoverManagerStore::ServiceTypeType;
}

wstring const& ServiceType::GetStoreKey() const
{
    if (idString_.empty())
    {
        idString_ = type_.ToString();
    }

    return idString_;
}

Reliability::LoadBalancingComponent::ServiceTypeDescription ServiceType::GetPLBServiceTypeDescription() const
{
    set<NodeId> disabledNodes;
    for (auto it = serviceTypeDisabledNodes_.begin(); it != serviceTypeDisabledNodes_.end(); ++it)
    {
        disabledNodes.insert(*it);
    }

    return Reliability::LoadBalancingComponent::ServiceTypeDescription(wstring(type_.QualifiedServiceTypeName), move(disabledNodes));
}

bool ServiceType::IsServiceTypeDisabled(NodeId const& nodeId) const
{
    auto it = find(serviceTypeDisabledNodes_.begin(), serviceTypeDisabledNodes_.end(), nodeId);
    if (it != serviceTypeDisabledNodes_.end())
    {
        return true;
    }

    return false;
}

bool ServiceType::TryGetCodePackageName(__out wstring & codePackageName) const
{
    if (codePackages_.empty())
    {
        return false;
    }

    codePackageName = codePackages_.begin()->second;
    return true;
}

bool ServiceType::TryGetServicePackageVersionInstance(__out ServiceModel::ServicePackageVersionInstance & servicePackageVersionInstance) const
{
    if (codePackages_.empty())
    {
        return false;
    }

    servicePackageVersionInstance = codePackages_.begin()->first;
    return true;
}


void ServiceType::PostUpdate(DateTime updateTime)
{
    lastUpdateTime_ = updateTime;
}

void ServiceType::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.WriteLine("CodePackages={0}", codePackages_);
    w.WriteLine("DisabledNodes={0}", serviceTypeDisabledNodes_);
    w.Write("{0} {1}", OperationLSN, lastUpdateTime_);
}
