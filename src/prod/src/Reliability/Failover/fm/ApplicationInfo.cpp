// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ApplicationInfo::ApplicationInfo()
    : StoreData()
{
}

ApplicationInfo::ApplicationInfo(ApplicationInfo const& other)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated),
      applicationId_(other.applicationId_),
      instanceId_(other.instanceId_),
      upgrade_(other.upgrade_ ? make_unique<ApplicationUpgrade>(*other.upgrade_) : nullptr),
      rollback_(other.rollback_ ? make_unique<ApplicationUpgrade>(*other.rollback_) : nullptr),
      lastUpdateTime_(other.lastUpdateTime_),
      idString_(other.idString_),
      isDeleted_(other.isDeleted_),
      applicationName_(other.applicationName_),
      updateId_(other.updateId_),
      capacityDescription_(other.capacityDescription_),
      resourceGovernanceDescription_(other.resourceGovernanceDescription_),
      codePackageContainersImages_(other.codePackageContainersImages_)
{
}

ApplicationInfo::ApplicationInfo(
    ServiceModel::ApplicationIdentifier const & applicationId,
    NamingUri const& applicationName,
    uint64 instanceId)
    : StoreData(PersistenceState::ToBeInserted),
      applicationId_(applicationId),
      instanceId_(instanceId),
      upgrade_(nullptr),
      rollback_(nullptr),
      lastUpdateTime_(DateTime::Now()),
      isDeleted_(false),
      applicationName_(applicationName),
      updateId_(0),
      capacityDescription_(),
      resourceGovernanceDescription_(),
      codePackageContainersImages_()
{
    PostRead(0);
}

ApplicationInfo::ApplicationInfo(ServiceModel::ApplicationIdentifier const & applicationId,
    Common::NamingUri const & applicationName,
    uint64 instanceId,
    uint64 updateId,
    ApplicationCapacityDescription const & appCapacity,
    ServiceModel::ServicePackageResourceGovernanceMap const& rgDescription,
    ServiceModel::CodePackageContainersImagesMap const& cpContainersImages)
    : StoreData(PersistenceState::ToBeInserted), 
      applicationId_(applicationId),
      applicationName_(applicationName),
      instanceId_(instanceId),
      updateId_(updateId),
      capacityDescription_(appCapacity),
      upgrade_(nullptr),
      rollback_(nullptr),
      lastUpdateTime_(DateTime::Now()),
      isDeleted_(false),
      resourceGovernanceDescription_(rgDescription),
      codePackageContainersImages_(cpContainersImages)
{
    PostRead(0);
}

ApplicationInfo::ApplicationInfo(
    ApplicationInfo const & other,
    ApplicationUpgradeUPtr && upgrade,
    ApplicationUpgradeUPtr && rollback)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated),
      applicationId_(other.applicationId_),
      idString_(other.idString_),
      upgrade_(move(upgrade)),
      rollback_(move(rollback)),
      lastUpdateTime_(other.lastUpdateTime_),
      isDeleted_(other.isDeleted_),
      applicationName_(other.applicationName_),
      updateId_(other.updateId_),
      capacityDescription_(other.capacityDescription_),
      resourceGovernanceDescription_(other.resourceGovernanceDescription_),
      codePackageContainersImages_(other.codePackageContainersImages_),
      networks_(other.networks_)
{
    instanceId_ = (upgrade_ ? upgrade_->Description.InstanceId : other.instanceId_);
}

ApplicationInfo::ApplicationInfo(ApplicationInfo const& other, uint64 instanceId)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated)
    , applicationId_(other.applicationId_)
    , idString_(other.idString_)
    , upgrade_(other.upgrade_ ? make_unique<ApplicationUpgrade>(*other.upgrade_) : nullptr)
    , rollback_(other.rollback_ ? make_unique<ApplicationUpgrade>(*other.rollback_) : nullptr)
    , lastUpdateTime_(other.lastUpdateTime_)
    , isDeleted_(other.isDeleted_)
    , applicationName_(other.applicationName_)
    , instanceId_(instanceId)
    , updateId_(other.updateId_)
    , capacityDescription_(other.capacityDescription_)
    , resourceGovernanceDescription_(other.resourceGovernanceDescription_)
    , codePackageContainersImages_(other.codePackageContainersImages_)
{
}

wstring const& ApplicationInfo::GetStoreType()
{
    return FailoverManagerStore::ApplicationType;
}

wstring const& ApplicationInfo::GetStoreKey() const
{
    return idString_;
}

void ApplicationInfo::AddThreadContext(vector<BackgroundThreadContextUPtr> & contexts, ApplicationInfoSPtr const & thisSPtr) const
{
    if (upgrade_)
    {
        BackgroundThreadContextUPtr context = upgrade_->CreateContext(thisSPtr);
        if (context)
        {
            contexts.push_back(move(context));
        }
    }
}

bool ApplicationInfo::GetUpgradeVersionForServiceType(ServiceTypeIdentifier const & typeId, ServicePackageVersionInstance & result) const
{
    if (!upgrade_)
    {
        return false;
    }

    return upgrade_->Description.Specification.GetUpgradeVersionForServiceType(typeId, result);
}

bool ApplicationInfo::GetUpgradeVersionForServiceType(ServiceTypeIdentifier const & typeId, ServicePackageVersionInstance & result, std::wstring const & upgradeDomain) const
{
    if (!upgrade_ || !upgrade_->CheckUpgradeDomain(upgradeDomain))
    {
        return false;
    }

    return upgrade_->Description.Specification.GetUpgradeVersionForServiceType(typeId, result);
}

bool ApplicationInfo::GetRollbackVersionForServiceType(ServiceTypeIdentifier const & typeId, ServicePackageVersionInstance & result) const
{
    if (!rollback_)
    {
        return false;
    }

    return rollback_->Description.Specification.GetUpgradeVersionForServiceType(typeId, result);
}

void ApplicationInfo::PostRead(int64 version)
{
    StoreData::PostRead(version);

    idString_ = (applicationId_.IsEmpty() ? L"Fabric" : applicationId_.ToString());
}

void ApplicationInfo::PostUpdate(Common::DateTime updateTime)
{
    lastUpdateTime_ = updateTime;
}

LoadBalancingComponent::ApplicationDescription ApplicationInfo::GetPLBApplicationDescription() const
{
    std::map<std::wstring, LoadBalancingComponent::ApplicationCapacitiesDescription> capacities;

    for (auto metric : capacityDescription_.Metrics)
    {
        std::wstring metricName = metric.Name;
        LoadBalancingComponent::ApplicationCapacitiesDescription metricDescription(move(metricName), 
            metric.TotalApplicationCapacity, 
            metric.MaximumNodeCapacity, 
            metric.ReservationNodeCapacity);
        capacities.insert(make_pair(metric.Name, move(metricDescription)));
    }

    std::wstring applicationName = applicationName_.ToString();

    bool isUpgradeInProgess = false;
    set<wstring> completedUDs;
    map<ServiceModel::ServicePackageIdentifier, LoadBalancingComponent::ServicePackageDescription> upgradedRG;

    if (upgrade_ != nullptr)
    {
        isUpgradeInProgess = true;
        completedUDs = move(upgrade_->GetCompletedDomains());
        upgradedRG = GetPLBServicePackageDescription(
            upgrade_->Description.Specification.UpgradedRGSettings,
            upgrade_->Description.Specification.UpgradedCPContainersImages);
    }

    auto spDescriptions = GetPLBServicePackageDescription(ResourceGovernanceDescription, CodePackageContainersImages);

    return LoadBalancingComponent::ApplicationDescription(
        move(applicationName),
        move(capacities), 
        capacityDescription_.MinimumNodes,
        capacityDescription_.MaximumNodes,
        move(spDescriptions),
        move(upgradedRG),
        isUpgradeInProgess,
        move(completedUDs),
        ServiceModel::ApplicationIdentifier(applicationId_));
}

std::map<ServiceModel::ServicePackageIdentifier, LoadBalancingComponent::ServicePackageDescription>
ApplicationInfo::GetPLBServicePackageDescription(
    ServiceModel::ServicePackageResourceGovernanceMap const & rgSettings,
    ServiceModel::CodePackageContainersImagesMap const & containersImagesMap) const
{
    map<ServiceModel::ServicePackageIdentifier, LoadBalancingComponent::ServicePackageDescription> spDescriptions;
    for (auto spRG = rgSettings.begin(); spRG != rgSettings.end(); ++spRG)
    {
        std::map<std::wstring, double> requiredResources;

        //for plb we set only the resources if the package is governed
        //we MUST pass all the metrics even the ones which have load 0
        if (spRG->second.IsGoverned)
        {
            requiredResources.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, spRG->second.CpuCores));
            requiredResources.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, spRG->second.MemoryInMB));
        }

        std::vector<std::wstring> containerImages;
        auto containerImagesIt = containersImagesMap.find(spRG->first);
        if (containerImagesIt != containersImagesMap.end())
        {
            for (auto& image : containerImagesIt->second.ContainersImages)
            {
                containerImages.push_back(image);
            }
        }

        LoadBalancingComponent::ServicePackageDescription spDesc(
            ServicePackageIdentifier(spRG->first),
            std::map<wstring, double>(requiredResources),
            std::vector<std::wstring>(containerImages));

        spDescriptions.insert(make_pair(ServicePackageIdentifier(spRG->first), move(spDesc)));
    }
    return spDescriptions;
}

bool ApplicationInfo::IsPLBSafetyCheckNeeded(ServiceModel::ServicePackageResourceGovernanceMap const & upgradingRG) const
{
    for (auto upgradingRGIt = upgradingRG.begin(); upgradingRGIt != upgradingRG.end(); ++upgradingRGIt)
    {
        auto itCurrentRG = resourceGovernanceDescription_.find(upgradingRGIt->first);
        if (itCurrentRG != resourceGovernanceDescription_.end())
        {
            //we need to check each metric and if one of them is increasing plb check is needed
            if (upgradingRGIt->second.CpuCores > itCurrentRG->second.CpuCores)
            {
                return true;
            }
            if (upgradingRGIt->second.MemoryInMB > itCurrentRG->second.MemoryInMB)
            {
                return true;
            }
        }
    }
    return false;
}

void ApplicationInfo::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w.Write(
        "{0} {1} {2} {3} {4} {5}",
        idString_,
        applicationName_,
        instanceId_,
        isDeleted_ ? L"D" : L"-",
        OperationLSN,
        lastUpdateTime_);

    if (upgrade_)
    {
        w.Write("\r\n");
        w.Write(*upgrade_);
    }

    if (rollback_)
    {
        w.Write("\r\nRollback: ");
        w.Write(*rollback_);
    }
}
