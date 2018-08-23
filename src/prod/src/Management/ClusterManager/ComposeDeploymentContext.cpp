// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ComposeDeploymentContext");

RolloutContextType::Enum const ComposeDeploymentContextType(RolloutContextType::ComposeDeployment);

ComposeDeploymentContext::ComposeDeploymentContext()
    : DeletableRolloutContext(ComposeDeploymentContextType)
    , applicationName_()
    , deploymentName_()
    , applicationId_()
    , globalInstanceCount_(0)
    , appTypeName_()
    , appTypeVersion_()
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , composeDeploymentStatus_(ComposeDeploymentStatus::Provisioning)
    , statusDetails_()
{
}

ComposeDeploymentContext::ComposeDeploymentContext(
    Common::NamingUri const & appName)
    : DeletableRolloutContext(ComposeDeploymentContextType)
    , applicationName_(appName)
    , deploymentName_(ClusterManagerReplica::GetDeploymentNameFromAppName(appName.ToString()))
    , applicationId_()
    , globalInstanceCount_(0)
    , appTypeName_()
    , appTypeVersion_()
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , composeDeploymentStatus_(ComposeDeploymentStatus::Provisioning)
    , statusDetails_()
{
}

ComposeDeploymentContext::ComposeDeploymentContext(
    std::wstring const & deploymentName)
    : DeletableRolloutContext(ComposeDeploymentContextType)
    , applicationName_()
    , deploymentName_(deploymentName)
    , applicationId_()
    , globalInstanceCount_(0)
    , appTypeName_()
    , appTypeVersion_()
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , composeDeploymentStatus_(ComposeDeploymentStatus::Provisioning)
    , statusDetails_()
{
}

ComposeDeploymentContext::ComposeDeploymentContext(
    ComposeDeploymentContext && other)
    : DeletableRolloutContext(move(other))
    , applicationName_(move(other.applicationName_))
    , deploymentName_(move(other.deploymentName_))
    , applicationId_(move(other.applicationId_))
    , globalInstanceCount_(move(other.globalInstanceCount_))
    , appTypeName_(move(other.appTypeName_))
    , appTypeVersion_(move(other.appTypeVersion_))
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_(move(other.pendingDefaultServices_))
    , composeDeploymentStatus_(move(other.composeDeploymentStatus_))
    , statusDetails_(move(other.statusDetails_))
{
}

ComposeDeploymentContext & ComposeDeploymentContext::operator=(
    ComposeDeploymentContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        applicationName_ = move(other.applicationName_);
        deploymentName_ = move(other.deploymentName_);
        applicationId_ = move(other.applicationId_);
        globalInstanceCount_ = move(other.globalInstanceCount_);
        appTypeName_ = move(other.appTypeName_);
        appTypeVersion_ = move(other.appTypeVersion_);
        // don't move: pendingDefaultServicesLock_
        pendingDefaultServices_ = move(other.pendingDefaultServices_);
        composeDeploymentStatus_ = move(other.composeDeploymentStatus_);
        statusDetails_ = move(other.statusDetails_);
    }
    return *this;
}

ComposeDeploymentContext::ComposeDeploymentContext(
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & clientRequest,
    wstring const & deploymentName,
    Common::NamingUri const & appName,
    ServiceModelApplicationId const & appId,
    uint64 globalInstanceCount,
    ServiceModelTypeName const & typeName,
    ServiceModelVersion const & typeVersion)
    : DeletableRolloutContext(ComposeDeploymentContextType, replica,clientRequest)
    , deploymentName_(deploymentName)
    , applicationName_(appName)
    , applicationId_(appId)
    , globalInstanceCount_(globalInstanceCount)
    , appTypeName_(typeName)
    , appTypeVersion_(typeVersion)
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , composeDeploymentStatus_(ComposeDeploymentStatus::Provisioning)
    , statusDetails_()
{
}

void ComposeDeploymentContext::AddPendingDefaultService(ServiceModelServiceNameEx && name)
{
    AcquireExclusiveLock lock(pendingDefaultServicesLock_);

    auto findIter = find(pendingDefaultServices_.begin(), pendingDefaultServices_.end(), name);
    if (findIter == pendingDefaultServices_.end())
    {
        pendingDefaultServices_.push_back(move(name));
    }
}

void ComposeDeploymentContext::ClearPendingDefaultServices()
{
    AcquireExclusiveLock lock(pendingDefaultServicesLock_);

    pendingDefaultServices_.clear();
}

ErrorCode ComposeDeploymentContext::UpdateComposeDeploymentStatus(Store::StoreTransaction const & storeTx, ComposeDeploymentStatus::Enum const status)
{
    InnerUpdateComposeDeploymentStatus(status);
    return storeTx.Update(*this);
}

ComposeDeploymentStatusQueryResult ComposeDeploymentContext::ToQueryResult() const
{
   return ComposeDeploymentStatusQueryResult(
        this->DeploymentName,
        this->ApplicationName,
        this->ComposeDeploymentStatus,
        this->StatusDetails);
}

std::wstring const & ComposeDeploymentContext::get_Type() const
{
    return Constants::StoreType_ComposeDeploymentContext;
}

std::wstring ComposeDeploymentContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}", deploymentName_);
    return temp;
}

ErrorCode ComposeDeploymentContext::StartCreating(Store::StoreTransaction &storeTx)
{
    InnerUpdateComposeDeploymentStatus(ComposeDeploymentStatus::Provisioning);
    return this->UpdateStatus(storeTx, RolloutStatus::Pending);
}

ErrorCode ComposeDeploymentContext::StartDeleting(Store::StoreTransaction &storeTx)
{
    this->statusDetails_ = L"";
    InnerUpdateComposeDeploymentStatus(ComposeDeploymentStatus::Deleting);
    return this->UpdateStatus(storeTx, RolloutStatus::DeletePending);
}

ErrorCode ComposeDeploymentContext::StartUpgrading(Store::StoreTransaction &storeTx)
{
    this->statusDetails_ = GET_RC( Deployment_Upgrade_In_Progress );
    InnerUpdateComposeDeploymentStatus(ComposeDeploymentStatus::Upgrading);
    return this->UpdateStatus(storeTx, RolloutStatus::Pending);
}

ErrorCode ComposeDeploymentContext::FinishCreating(Store::StoreTransaction &storeTx, wstring const &statusDetails)
{
    this->statusDetails_ = statusDetails;
    InnerUpdateComposeDeploymentStatus(ComposeDeploymentStatus::Ready);
    return this->Complete(storeTx);
}

ErrorCode ComposeDeploymentContext::ClearUpgrading(Store::StoreTransaction &storeTx, wstring const &statusDetails)
{
    return this->FinishCreating(storeTx, statusDetails);
}

void ComposeDeploymentContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "ComposeDeploymentContext({0})[{1}, {2}, {3}, ({4}) ({5}, {6})]",
        this->Status,
        deploymentName_,
        applicationName_,
        applicationId_,
        globalInstanceCount_,
        appTypeName_,
        appTypeVersion_);
}

void ComposeDeploymentContext::InnerUpdateComposeDeploymentStatus(ComposeDeploymentStatus::Enum const status)
{
    WriteNoise(TraceComponent, "{0} InnerUpdateComposeDeploymentStatus({1}, {2}", this->TraceId, status, this->SequenceNumber);

    this->ComposeDeploymentStatus = status;
}

