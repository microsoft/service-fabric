// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Constants.h"
#include "ApplicationDescription.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const ApplicationDescription::FormatHeader = make_global<wstring>(L"Application Descriptions :");

ApplicationDescription::ApplicationDescription(std::wstring && name)
    : appName_(move(name)),
    capacities_(map<std::wstring, ApplicationCapacitiesDescription>()),
    minimumNodes_(0),
    scaleoutCount_(0),
    applicationId_(0),
    upgradeInProgess_(false),
    upgradeCompletedUDs_(set<wstring>()),
    servicePackages_(),
    upgradingServicePackages_(),
    applicationIdentifer_()
{
}

ApplicationDescription::ApplicationDescription(
    std::wstring && name,
    std::map<std::wstring, ApplicationCapacitiesDescription> && capacities,
    int scaleoutCount)
    : appName_(move(name)),
    capacities_(move(capacities)),
    minimumNodes_(0),
    scaleoutCount_(scaleoutCount),
    applicationId_(0),
    upgradeInProgess_(false),
    upgradeCompletedUDs_(set<wstring>()),
    servicePackages_(),
    upgradingServicePackages_(),
    packagesToCheck_(),
    applicationIdentifer_()
{
}

ApplicationDescription::ApplicationDescription(
    std::wstring && name,
    std::map<std::wstring, ApplicationCapacitiesDescription> && capacities,
    int minimumNodes,
    int scaleoutCount,
    std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> && servicePackages,
    std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> && upgradingServicePackages,
    bool isUpgradeInProgress,
    std::set<std::wstring> && completedUDs,
    ServiceModel::ApplicationIdentifier applicationIdentifier)
    : appName_(move(name)),
    capacities_(move(capacities)),
    minimumNodes_(minimumNodes),
    scaleoutCount_(scaleoutCount),
    applicationId_(0),
    upgradeInProgess_(isUpgradeInProgress),
    upgradeCompletedUDs_(move(completedUDs)),
    servicePackages_(move(servicePackages)),
    upgradingServicePackages_(move(upgradingServicePackages)),
    packagesToCheck_(),
    applicationIdentifer_(applicationIdentifier)
{
    for (auto upgradingSPIt = upgradingServicePackages_.begin(); upgradingSPIt != upgradingServicePackages_.end(); ++upgradingSPIt)
    {
        auto const& spIt = servicePackages_.find(upgradingSPIt->first);
        if (spIt != servicePackages_.end())
        {
            // If SP does not exist, upgrade is not needed.
            // If we are increasing resources while the upgrade is in progress we consider the merged load from current load and new load
            // because it can happen that we increase 1 resource but decrease the other
            // If we are downgrading resources we keep the original load during upgrade
            if (spIt->second.IsAnyResourceReservationIncreased(upgradingSPIt->second))
            {
                spIt->second = ServicePackageDescription(spIt->second, upgradingSPIt->second);
                packagesToCheck_.insert(spIt->first);
            }
        }
        //this is a new service package
        else
        {
            servicePackages_.insert(std::pair<ServiceModel::ServicePackageIdentifier, ServicePackageDescription>(upgradingSPIt->first, ServicePackageDescription(upgradingSPIt->second)));
        }
    }
}

ApplicationDescription::ApplicationDescription(ApplicationDescription const & other)
    : appName_(other.appName_),
    capacities_(other.capacities_),
    minimumNodes_(other.minimumNodes_),
    scaleoutCount_(other.scaleoutCount_),
    applicationId_(other.applicationId_),
    upgradeInProgess_(other.upgradeInProgess_),
    upgradeCompletedUDs_(other.upgradeCompletedUDs_),
    servicePackages_(other.servicePackages_),
    upgradingServicePackages_(other.upgradingServicePackages_),
    packagesToCheck_(other.packagesToCheck_),
    applicationIdentifer_(other.applicationIdentifer_)
{
}

ApplicationDescription::ApplicationDescription(ApplicationDescription && other)
    : appName_(move(other.appName_)),
    capacities_(move(other.capacities_)),
    minimumNodes_(other.minimumNodes_),
    scaleoutCount_(other.scaleoutCount_),
    applicationId_(other.applicationId_),
    upgradeInProgess_(other.upgradeInProgess_),
    upgradeCompletedUDs_(move(other.upgradeCompletedUDs_)),
    servicePackages_(move(other.servicePackages_)),
    upgradingServicePackages_(move(other.upgradingServicePackages_)),
    packagesToCheck_(move(other.packagesToCheck_)),
    applicationIdentifer_(move(other.applicationIdentifer_))
{
}

ApplicationDescription & ApplicationDescription::operator = (ApplicationDescription && other)
{
    if (this != &other)
    {
        appName_ = move(other.appName_);
        capacities_ = move(other.capacities_);
        minimumNodes_ = other.minimumNodes_;
        scaleoutCount_ = move(other.scaleoutCount_);
        applicationId_ = other.applicationId_;
        upgradeInProgess_ = other.upgradeInProgess_;
        upgradeCompletedUDs_ = move(other.upgradeCompletedUDs_);
        servicePackages_ = move(other.servicePackages_);
        upgradingServicePackages_ = move((other.upgradingServicePackages_));
        packagesToCheck_ = move(other.packagesToCheck_);
        applicationIdentifer_ = move(other.applicationIdentifer_);
    }

    return *this;
}

bool ApplicationDescription::operator == (ApplicationDescription const & other) const
{
    // it is only used when we update the service description
    ASSERT_IFNOT(appName_ == other.appName_, "Comparison between two different applications");

    return (
        appName_ == other.appName_ &&
        capacities_ == other.capacities_ &&
        scaleoutCount_ == other.scaleoutCount_ &&
        minimumNodes_ == other.minimumNodes_ &&
        applicationId_ == other.applicationId_ &&
        upgradeInProgess_ == other.upgradeInProgess_ &&
        upgradeCompletedUDs_ == other.upgradeCompletedUDs_ &&
        servicePackages_ == other.servicePackages_ &&
        upgradingServicePackages_ == other.upgradingServicePackages_ &&
        packagesToCheck_ == other.packagesToCheck_ &&
        applicationIdentifer_ == other.applicationIdentifer_);
}

bool ApplicationDescription::operator != (ApplicationDescription const & other) const
{
    return !(*this == other);
}

void ApplicationDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} id:{1} capacities:{2} minimumNodes:{3} scaleoutCount:{4} upgradeInProgress: {5}, upgradeCompletedUDs: {6}, servicePackages: {7}, upgradingServicePackages: {8}, packagesToCheck: {9}, applicationIdentifier: {10}",
        appName_,
        applicationId_,
        capacities_,
        minimumNodes_,
        scaleoutCount_,
        upgradeInProgess_,
        PlacementAndLoadBalancing::UDsToString(upgradeCompletedUDs_),
        servicePackages_,
        upgradingServicePackages_,
        packagesToCheck_,
        applicationIdentifer_);
}

void ApplicationDescription::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
