// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationUpgradeSpecification::ApplicationUpgradeSpecification()
    : applicationId_(),
    applicationVersion_(),
    instanceId_(),
    upgradeType_(UpgradeType::Rolling),
    isMonitored_(false),
    isManual_(false),
    packageUpgrades_(),
    applicationName_(),
    spRGSettings_(),
    cpContainersImages_(),
    networks_()
{
}

ApplicationUpgradeSpecification::ApplicationUpgradeSpecification(
    ApplicationIdentifier const & applicationId, 
    ApplicationVersion const & applicationVersion, 
    wstring const & applicationName,
    uint64 instanceId, 
    UpgradeType::Enum upgradeType,
    bool isMonitored,
    bool isManual,
    vector<ServicePackageUpgradeSpecification> && packageUpgrades,
    vector<ServiceTypeRemovalSpecification> && removedTypes,
    ServicePackageResourceGovernanceMap && spRGSettings,
    CodePackageContainersImagesMap && cpContainersImages,
    vector<wstring> && networks)
    : applicationId_(applicationId),
    applicationVersion_(applicationVersion),
    instanceId_(instanceId),
    upgradeType_(upgradeType),
    isMonitored_(isMonitored),
    isManual_(isManual),
    packageUpgrades_(move(packageUpgrades)),
    removedTypes_(move(removedTypes)),
    applicationName_(move(applicationName)),
    spRGSettings_(move(spRGSettings)),
    cpContainersImages_(move(cpContainersImages)),
    networks_(move(networks))
{
}

ApplicationUpgradeSpecification::ApplicationUpgradeSpecification(ApplicationUpgradeSpecification const & other)
    : applicationId_(other.applicationId_),
    applicationVersion_(other.applicationVersion_),
    instanceId_(other.instanceId_),
    upgradeType_(other.upgradeType_),
    isMonitored_(other.isMonitored_),
    isManual_(other.isManual_),
    packageUpgrades_(other.packageUpgrades_),
    removedTypes_(other.removedTypes_),
    applicationName_(other.applicationName_),
    spRGSettings_(other.spRGSettings_),
    cpContainersImages_(other.cpContainersImages_),
    networks_(other.networks_)
{
}

ApplicationUpgradeSpecification::ApplicationUpgradeSpecification(ApplicationUpgradeSpecification && other)
    : applicationId_(move(other.applicationId_)),
    applicationVersion_(move(other.applicationVersion_)),
    instanceId_(other.instanceId_),
    upgradeType_(other.upgradeType_),
    isMonitored_(other.isMonitored_),
    isManual_(other.isManual_),
    packageUpgrades_(move(other.packageUpgrades_)),
    removedTypes_(move(other.removedTypes_)),
    applicationName_(move(other.applicationName_)),
    spRGSettings_(move(other.spRGSettings_)),
    cpContainersImages_(move(other.cpContainersImages_)),
    networks_(move(other.networks_))
{
}


ApplicationUpgradeSpecification const & ApplicationUpgradeSpecification::operator = (ApplicationUpgradeSpecification const & other)
{
    if (this != &other)
    {
        this->applicationId_ = other.applicationId_;
        this->applicationVersion_ = other.applicationVersion_;
        this->instanceId_ = other.instanceId_;
        this->upgradeType_ = other.upgradeType_;
        this->isMonitored_ = other.isMonitored_;
        this->isManual_ = other.isManual_;
        this->packageUpgrades_ = other.packageUpgrades_;
        this->removedTypes_ = other.removedTypes_;
        this->applicationName_ = other.applicationName_;
        this->spRGSettings_ = other.spRGSettings_;
        this->cpContainersImages_ = other.cpContainersImages_;
        this->networks_ = other.networks_;
    }

    return *this;
}

ApplicationUpgradeSpecification const & ApplicationUpgradeSpecification::operator = (ApplicationUpgradeSpecification && other)
{
    if (this != &other)
    {
        this->applicationId_ = move(other.applicationId_);
        this->applicationVersion_ = move(other.applicationVersion_);
        this->instanceId_ = other.instanceId_;
        this->upgradeType_ = other.upgradeType_;
        this->isMonitored_ = other.isMonitored_;
        this->isManual_ = other.isManual_;
        this->packageUpgrades_ = move(other.packageUpgrades_);
        this->removedTypes_ = move(other.removedTypes_);
        this->applicationName_ = move(other.applicationName_);
        this->spRGSettings_ = move(other.spRGSettings_);
        this->cpContainersImages_ = move(other.cpContainersImages_);
        this->networks_ = move(other.networks_);
    }

    return *this;
}

bool ApplicationUpgradeSpecification::GetUpgradeVersionForPackage(wstring const & packageName, RolloutVersion & result) const
{
    for(auto it = packageUpgrades_.cbegin(); it != packageUpgrades_.cend(); ++it)
    {
        ServicePackageUpgradeSpecification const & package = *it;
        if (package.ServicePackageName == packageName)
        {
            result = package.RolloutVersionValue;
            return true;
        }
    }

    return false;
}

bool ApplicationUpgradeSpecification::GetUpgradeVersionForPackage(wstring const & packageName, ServicePackageVersionInstance & result) const
{
    RolloutVersion version;
    if (GetUpgradeVersionForPackage(packageName, version))
    {
        result = ServicePackageVersionInstance(ServicePackageVersion(applicationVersion_, version), instanceId_);
        return true;
    }

    return false;
}

bool ApplicationUpgradeSpecification::GetUpgradeVersionForServiceType(ServiceTypeIdentifier const & typeId, RolloutVersion & result) const
{
    if (typeId.ApplicationId != ApplicationId)
    {
        return false;
    }

    return GetUpgradeVersionForPackage(typeId.ServicePackageId.ServicePackageName, result);
}

bool ApplicationUpgradeSpecification::GetUpgradeVersionForServiceType(ServiceTypeIdentifier const & typeId, ServiceModel::ServicePackageVersionInstance & result) const
{
    if (typeId.ApplicationId != ApplicationId)
    {
        return false;
    }

    return GetUpgradeVersionForPackage(typeId.ServicePackageId.ServicePackageName, result);
}

bool ApplicationUpgradeSpecification::AddPackage(
    std::wstring const & packageName, 
    RolloutVersion const & version,
    std::vector<std::wstring> const & codePackages)
{
    for (auto it = packageUpgrades_.cbegin(); it != packageUpgrades_.cend(); ++it)
    {
        ServicePackageUpgradeSpecification const & package = *it;
        if (package.ServicePackageName == packageName)
        {
            return (version == package.RolloutVersionValue);
        }
    }

    packageUpgrades_.push_back(ServicePackageUpgradeSpecification(packageName, version, codePackages));

    return true;
}

bool ApplicationUpgradeSpecification::IsServiceTypeBeingDeleted(ServiceTypeIdentifier const & typeId) const
{
    if (typeId.ApplicationId != ApplicationId)
    {
        return false;
    }

    for(auto it = removedTypes_.cbegin(); it != removedTypes_.cend(); ++it)
    {
        ServiceTypeRemovalSpecification const & specification = *it;
        if (specification.ServicePackageName == typeId.ServicePackageId.ServicePackageName && specification.HasServiceTypeName(typeId.ServiceTypeName))
        {
            return true;
        }
    }

    return false;
}

bool ApplicationUpgradeSpecification::IsCodePackageBeingUpgraded(ServiceTypeIdentifier const& typeId, wstring const& codePackageName) const
{
    if (typeId.ApplicationId != ApplicationId)
    {
        return false;
    }

    for (auto const& packageUpgrade : packageUpgrades_)
    {
        if (packageUpgrade.ServicePackageName == typeId.ServicePackageId.ServicePackageName)
        {
            if (packageUpgrade.IsCodePackageInfoIncluded)
            {
                auto it = find(packageUpgrade.CodePackageNames.begin(), packageUpgrade.CodePackageNames.end(), codePackageName);
                return (it != packageUpgrade.CodePackageNames.end());
            }

            return true;
        }
    }

    return false;
}

void ApplicationUpgradeSpecification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "Id={0},Version={1}:{2},ApplicationName={3},Type={4},IsMonitored={5},IsManual={6}",
        applicationId_, applicationVersion_, instanceId_, applicationName_, upgradeType_, isMonitored_, isManual_);

    if (!packageUpgrades_.empty())
    {
        w.Write(",PackageUpgrades={");
        for(auto iter = packageUpgrades_.cbegin(); iter != packageUpgrades_.cend(); ++iter)
        {
            w.Write("{");
            w.Write("{0}", *iter);
            w.Write("}");
        }
        w.Write("}");
    }

    if (!removedTypes_.empty())
    {
        w.Write(",RemovedTypes={");
        for(auto iter = removedTypes_.cbegin(); iter != removedTypes_.cend(); ++iter)
        {
            w.Write("{");
            w.Write("{0}", *iter);
            w.Write("}");
        }
        w.Write("}");
    }

    if (!spRGSettings_.empty())
    {
        w.Write(",RGSettings={");
        for (auto & spRG : spRGSettings_)
        {
            w.Write("{");
            w.Write("{0}", spRG.first);
            w.Write(",");
            w.Write("{0}", spRG.second);
            w.Write("}");
        }
        w.Write("}");
    }

    if (!cpContainersImages_.empty())
    {
        w.Write(",CPContainersImages={");
        for (auto & cpCI : cpContainersImages_)
        {
            w.Write("{");
            w.Write("{0}", cpCI.first);
            w.Write(",");
            w.Write("{0}", cpCI.second);
            w.Write("}");
        }
        w.Write("}");
    }

    w.Write(",NetworkCount={0}", networks_.size());
}

void ApplicationUpgradeSpecification::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<ServiceModel::ApplicationIdentifier>(applicationId_);
    context.Write<ServiceModel::ApplicationVersion>(applicationVersion_);
    context.Write<wstring>(applicationName_);
    context.Write<uint64>(instanceId_);
    context.WriteCopy<std::wstring>(Common::wformatString(upgradeType_));
    context.Write<bool>(isMonitored_);
    context.Write<bool>(isManual_);

    {
        std::wstring buffer;
        Common::StringWriter textWriter(buffer);
        std::for_each(packageUpgrades_.cbegin(), packageUpgrades_.cend(), [&](const ServicePackageUpgradeSpecification& pkg) 
        {
            textWriter.Write("{");
            textWriter.Write("{0}", pkg);
            textWriter.Write("}");
        });
                
        context.WriteCopy<std::wstring>(buffer);
    }

    {
        std::wstring buffer;
        Common::StringWriter textWriter(buffer);
        std::for_each(removedTypes_.cbegin(), removedTypes_.cend(), [&](const ServiceTypeRemovalSpecification& type) 
        {
            textWriter.Write("{");
            textWriter.Write("{0}", type);
            textWriter.Write("}");
        });
                
        context.WriteCopy<std::wstring>(buffer);
    }
}