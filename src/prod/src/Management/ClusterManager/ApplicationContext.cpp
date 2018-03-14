// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ApplicationContext");

RolloutContextType::Enum const ApplicationContextType(RolloutContextType::Application);

ApplicationContext::ApplicationContext()
    : DeletableRolloutContext(ApplicationContextType) 
    , applicationName_()
    , applicationId_()
    , globalInstanceCount_(0)
    , applicationVersion_(0)
    , appManifestId_()
    , packageInstance_(0)
    , appTypeName_()
    , appTypeVersion_()
    , appParameters_()
    , capacityDescription_()
    , isUpgrading_(false)
    , isCommitPending_(false)
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , hasReportedHealthEntity_(false)
    , updateID_(0)
    , applicationDefinitionKind_(ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
{
}

ApplicationContext::ApplicationContext(
    Common::NamingUri const & appName)
    : DeletableRolloutContext(ApplicationContextType) 
    , applicationName_(appName)
    , applicationId_()
    , globalInstanceCount_(0)
    , applicationVersion_(0)
    , appManifestId_()
    , packageInstance_(0)
    , appTypeName_()
    , appTypeVersion_()
    , appParameters_()
    , capacityDescription_()
    , isUpgrading_(false)
    , isCommitPending_(false)
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , hasReportedHealthEntity_(false)
    , updateID_(0)
    , applicationDefinitionKind_(ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
{
}

ApplicationContext::ApplicationContext(
    Common::NamingUri const & appName,
    bool const isForceDelete)
    : DeletableRolloutContext(ApplicationContextType, isForceDelete)
    , applicationName_(appName)
    , applicationId_()
    , globalInstanceCount_(0)
    , applicationVersion_(0)
    , packageInstance_(0)
    , appTypeName_()
    , appTypeVersion_()
    , appManifestId_()
    , appParameters_()
    , capacityDescription_()
    , isUpgrading_(false)
    , isCommitPending_(false)
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , hasReportedHealthEntity_(false)
    , updateID_(0)
    , applicationDefinitionKind_(ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
{
}

ApplicationContext::ApplicationContext(
    ApplicationContext && other)
    : DeletableRolloutContext(move(other))
    , applicationName_(move(other.applicationName_))
    , applicationId_(move(other.applicationId_))
    , globalInstanceCount_(move(other.globalInstanceCount_))
    , applicationVersion_(move(other.applicationVersion_))
    , packageInstance_(move(other.packageInstance_))
    , appTypeName_(move(other.appTypeName_))
    , appTypeVersion_(move(other.appTypeVersion_))
    , appManifestId_(move(other.appManifestId_))
    , appParameters_(move(other.appParameters_))
    , capacityDescription_(move(other.capacityDescription_))
    , isUpgrading_(move(other.isUpgrading_))
    , isCommitPending_(move(other.isCommitPending_))
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_(move(other.pendingDefaultServices_))
    , hasReportedHealthEntity_(move(other.hasReportedHealthEntity_))
    , updateID_(other.updateID_)
    , applicationDefinitionKind_(other.ApplicationDefinitionKind)
{
}

ApplicationContext & ApplicationContext::operator=(
    ApplicationContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        applicationName_ = move(other.applicationName_);
        applicationId_ = move(other.applicationId_);
        globalInstanceCount_ = move(other.globalInstanceCount_);
        applicationVersion_ = move(other.applicationVersion_);
        packageInstance_ = move(other.packageInstance_);
        appTypeName_ = move(other.appTypeName_);
        appTypeVersion_ = move(other.appTypeVersion_);
        appManifestId_ = move(other.appManifestId_);
        appParameters_ = move(other.appParameters_);
        capacityDescription_ = move(other.capacityDescription_);
        isUpgrading_ = move(other.isUpgrading_);
        isCommitPending_ = move(other.isCommitPending_);
        applicationDefinitionKind_ = other.applicationDefinitionKind_;
        // don't move: pendingDefaultServicesLock_()
        pendingDefaultServices_ = move(other.pendingDefaultServices_);
        hasReportedHealthEntity_ = move(other.hasReportedHealthEntity_);
        updateID_ = other.updateID_;
    }
    return *this;
}

ApplicationContext::ApplicationContext(
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & clientRequest,
    Common::NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    uint64 globalInstanceCount,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    std::wstring const & appManifestId,
    std::map<std::wstring, std::wstring> const & appParameters)
    : DeletableRolloutContext(ApplicationContextType, replica, clientRequest)
    , applicationName_(appName)
    , applicationId_(appId)
    , globalInstanceCount_(globalInstanceCount)
    , applicationVersion_(0)
    , packageInstance_(DateTime::Now().Ticks)
    , appTypeName_(typeName)
    , appTypeVersion_(typeVersion)
    , appManifestId_(appManifestId)
    , appParameters_(appParameters)
    , capacityDescription_()
    , isUpgrading_(false)
    , isCommitPending_(false)
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , hasReportedHealthEntity_(false)
    , updateID_(0)
    , applicationDefinitionKind_(ApplicationDefinitionKind::Enum::ServiceFabricApplicationDescription)
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor({1}, {2}, Instance = {3}, {4}, {5}, {6})",
        this->TraceId,
        appName,
        appId,
        globalInstanceCount,
        typeName,
        typeVersion,
        applicationDefinitionKind_);
}

ApplicationContext::ApplicationContext(
    Store::ReplicaActivityId const &replicaActivityId,
    Common::NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    uint64 globalInstanceCount,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion,
    std::wstring const & appManifestId,
    std::map<std::wstring, std::wstring> const & appParameters,
    ApplicationDefinitionKind::Enum const applicationDefinitionKind)
    : DeletableRolloutContext(ApplicationContextType, replicaActivityId)
    , applicationName_(appName)
    , applicationId_(appId)
    , globalInstanceCount_(globalInstanceCount)
    , applicationVersion_(0)
    , packageInstance_(DateTime::Now().Ticks)
    , appTypeName_(typeName)
    , appTypeVersion_(typeVersion)
    , appManifestId_(appManifestId)
    , appParameters_(appParameters)
    , capacityDescription_()
    , isUpgrading_(false)
    , isCommitPending_(false)
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , hasReportedHealthEntity_(false)
    , updateID_(0)
    , applicationDefinitionKind_(applicationDefinitionKind)
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor({1}, {2}, Instance = {3}, {4}, {5})",
        this->TraceId,
        appName,
        appId,
        globalInstanceCount,
        typeName,
        typeVersion);
}

ApplicationContext::~ApplicationContext()
{
    if (replicaSPtr_)
    {
        WriteNoise(
            TraceComponent,
            "{0} ~dtor",
            this->TraceId);
    }
}

ApplicationQueryResult ApplicationContext::ToQueryResult(bool excludeApplicationParameters) const
{
    ApplicationStatus::Enum applicationStatus;

    if (this->IsUpgrading)
    {
        applicationStatus = ApplicationStatus::Upgrading;
    }
    else if (this->IsPending)
    {
        applicationStatus = ApplicationStatus::Creating;
    }
    else if (this->IsDeleting)
    {
        applicationStatus = ApplicationStatus::Deleting;
    }
    else if (this->IsFailed)
    {
        applicationStatus = ApplicationStatus::Failed;
    }
    else
    {
        applicationStatus = ApplicationStatus::Ready;
    }

    return ApplicationQueryResult(
        this->ApplicationName,
        this->TypeName.Value,
        this->TypeVersion.Value,
        applicationStatus,
        excludeApplicationParameters ? map<wstring, wstring>() : this->Parameters,
        ApplicationDefinitionKind::ToPublicApi(applicationDefinitionKind_),
        L"", // deprecated field as of v2.2
        map<wstring, wstring>()); // deprecated field as of v2.2
}

std::wstring const & ApplicationContext::get_Type() const
{
    return Constants::StoreType_ApplicationContext;
}

std::wstring ApplicationContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}", applicationName_);
    return temp;
}

void ApplicationContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "ApplicationContext({0})[{1}, {2}, {3} ({4}, {5}, {6}) ({7}, {8}) ({9}) ({10})]",
        this->Status,
        applicationName_,
        applicationId_,
        applicationDefinitionKind_,
        globalInstanceCount_,
        applicationVersion_,
        packageInstance_,
        appTypeName_,
        appTypeVersion_,
        capacityDescription_,
        appManifestId_);
}

bool ApplicationContext::CanAcceptUpgrade(
    ApplicationUpgradeDescription const & upgradeDescription,
    StoreDataApplicationManifest const & currentManifest) const
{
    if (this->TypeVersion != ServiceModelVersion(upgradeDescription.TargetApplicationTypeVersion))
    {
        return true;
    }
    
    auto currentMergedParameters = currentManifest.GetMergedApplicationParameters(this->Parameters);
    auto targetMergedParameters = currentManifest.GetMergedApplicationParameters(upgradeDescription.ApplicationParameters);

    // Don't expect count of parameters to change since the merge is based on the same manifest.
    // ImageBuilder will pick up on any validation errors due to extra (invalid) parameters that
    // may exist.
    //
    if (currentMergedParameters.size() != targetMergedParameters.size())
    {
        return true;
    }

    // Even if type version doesn't change, allow upgrades in which only the 
    // application parameters have changed
    //
    for (auto it=targetMergedParameters.begin(); it!=targetMergedParameters.end(); ++it)
    {
        auto find_it = currentMergedParameters.find(it->first);
        if (find_it == currentMergedParameters.end() || find_it->second != it->second)
        {
            return true;
        }
    }

    return false;
}

void ApplicationContext::SetApplicationVersion(uint64 applicationVersion)
{
    applicationVersion_ = applicationVersion;
}

void ApplicationContext::SetApplicationAndTypeVersion(uint64 applicationVersion, ServiceModelVersion const & version)
{
    applicationVersion_ = applicationVersion;
    appTypeVersion_ = version;
}

void ApplicationContext::SetApplicationParameters(std::map<std::wstring, std::wstring> const & applicationParameters)
{
    appParameters_ = applicationParameters;
}

Common::ErrorCode ApplicationContext::SetApplicationCapacity(Reliability::ApplicationCapacityDescription const & applicationCapacity)
{
    wstring errorMessage;
    if (!applicationCapacity.TryValidate(errorMessage))
    {
        return ErrorCode(ErrorCodeValue::InvalidArgument, move(errorMessage));
    }

    capacityDescription_ = applicationCapacity;

    return ErrorCodeValue::Success;
}

void ApplicationContext::SetHasReportedHealthEntity()
{
    hasReportedHealthEntity_ = true;
}

wstring ApplicationContext::GetKeyPrefix(ServiceModelApplicationId const & appId)
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}{1}",
        appId,
        Constants::TokenDelimeter);

    return temp;
}

ErrorCode ApplicationContext::SetUpgradePending(Store::StoreTransaction const & storeTx, uint64 packageInstance) 
{ 
    isUpgrading_ = true; 

    if (packageInstance > 0)
    {
        packageInstance_ = packageInstance;
    }

    return this->UpdateStatus(storeTx, RolloutStatus::Pending);
}

ErrorCode ApplicationContext::SetUpgradeComplete(Store::StoreTransaction const & storeTx, uint64 packageInstance) 
{ 
    isUpgrading_ = false; 
    packageInstance_ = packageInstance; 

    return this->Complete(storeTx);
}

void ApplicationContext::AddPendingDefaultService(ServiceModelServiceNameEx && names)
{
    AcquireExclusiveLock lock(pendingDefaultServicesLock_);

    auto findIter = find(pendingDefaultServices_.begin(), pendingDefaultServices_.end(), names);
    if (findIter == pendingDefaultServices_.end())
    {
        pendingDefaultServices_.push_back(move(names));
    }
}

void ApplicationContext::ClearPendingDefaultServices()
{
    AcquireExclusiveLock lock(pendingDefaultServicesLock_);

    pendingDefaultServices_.clear();
}
