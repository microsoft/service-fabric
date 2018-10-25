//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("SingleInstanceDeploymentContext");

RolloutContextType::Enum const SingleInstanceDeploymentContextType(RolloutContextType::SingleInstanceDeployment);

SingleInstanceDeploymentContext::SingleInstanceDeploymentContext()
    : DeletableRolloutContext(SingleInstanceDeploymentContextType)
    , deploymentName_()
    , deploymentType_(DeploymentType::Enum::Invalid)
    , applicationName_()
    , deploymentStatus_(SingleInstanceDeploymentStatus::Enum::Invalid)
    , applicationId_()
    , globalInstanceCount_(0)
    , appTypeName_()
    , appTypeVersion_()
    , appTypeNewVersion_()
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , statusDetails_()
{
}

SingleInstanceDeploymentContext::SingleInstanceDeploymentContext(
    wstring const & deploymentName)
    : DeletableRolloutContext(SingleInstanceDeploymentContextType)
    , deploymentName_(deploymentName)
    , deploymentType_(DeploymentType::Enum::Invalid)
    , applicationName_()
    , deploymentStatus_(SingleInstanceDeploymentStatus::Enum::Invalid)
    , applicationId_()
    , globalInstanceCount_(0)
    , appTypeName_()
    , appTypeVersion_()
    , appTypeNewVersion_()
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , statusDetails_()
{
}

SingleInstanceDeploymentContext::SingleInstanceDeploymentContext(
    Common::ComponentRoot const & replica,
    ClientRequestSPtr const & clientRequest,
    wstring const & deploymentName,
    DeploymentType::Enum const deploymentType,
    NamingUri const & applicationName,
    SingleInstanceDeploymentStatus::Enum const deploymentStatus,
    ServiceModelApplicationId applicationId,
    uint64 globalInstanceCount,
    ServiceModelTypeName appTypeName,
    ServiceModelVersion appTypeVersion,
    wstring statusDetails)
    : DeletableRolloutContext(SingleInstanceDeploymentContextType, replica, clientRequest)
    , deploymentName_(deploymentName)
    , deploymentType_(deploymentType)
    , applicationName_(applicationName)
    , deploymentStatus_(deploymentStatus)
    , applicationId_(applicationId)
    , globalInstanceCount_(globalInstanceCount)
    , appTypeName_(appTypeName)
    , appTypeVersion_(appTypeVersion)
    , appTypeNewVersion_()
    , pendingDefaultServicesLock_()
    , pendingDefaultServices_()
    , statusDetails_()
{
}

ErrorCode SingleInstanceDeploymentContext::UpdateDeploymentStatus(Store::StoreTransaction const & storeTx, SingleInstanceDeploymentStatus::Enum const status)
{
    InnerUpdateStatus(status);
    return storeTx.Update(*this);
}
    
void SingleInstanceDeploymentContext::AddPendingDefaultService(ServiceModelServiceNameEx && name)
{
    AcquireExclusiveLock lock(pendingDefaultServicesLock_);

    auto findIter = find(pendingDefaultServices_.begin(), pendingDefaultServices_.end(), name);
    if (findIter == pendingDefaultServices_.end())
    {
        pendingDefaultServices_.push_back(move(name));
    }
}

void SingleInstanceDeploymentContext::ClearPendingDefaultServices()
{
    AcquireExclusiveLock lock(pendingDefaultServicesLock_);

    pendingDefaultServices_.clear();
}

SingleInstanceDeploymentQueryResult SingleInstanceDeploymentContext::ToQueryResult() const
{
    return SingleInstanceDeploymentQueryResult(
        deploymentName_,
        applicationName_,
        deploymentStatus_,
        statusDetails_);
}

wstring const & SingleInstanceDeploymentContext::get_Type() const
{
    return Constants::StoreType_SingleInstanceDeploymentContext;
}

wstring SingleInstanceDeploymentContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}", deploymentName_);
    return temp;
}

void SingleInstanceDeploymentContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "SingleInstanceDeploymentContext({0})[{1}, {2}, {3}, {4} ({5}) ({6}, {7})]",
        this->Status,
        deploymentName_,
        deploymentType_,
        applicationName_,
        applicationId_,
        globalInstanceCount_,
        appTypeName_,
        appTypeVersion_);
}

ErrorCode SingleInstanceDeploymentContext::StartDeleting(Store::StoreTransaction const & storeTx)
{
    this->statusDetails_ = L"";
    InnerUpdateStatus(SingleInstanceDeploymentStatus::Enum::Deleting);
    return this->UpdateStatus(storeTx, RolloutStatus::DeletePending);
}

ErrorCode SingleInstanceDeploymentContext::StartReplacing(Store::StoreTransaction const & storeTx)
{
    this->statusDetails_ = L"";
    InnerUpdateStatus(SingleInstanceDeploymentStatus::Enum::Deleting);
    return this->UpdateStatus(storeTx, RolloutStatus::Enum::Replacing);
}

ErrorCode SingleInstanceDeploymentContext::FinishCreating(Store::StoreTransaction const & storeTx, wstring const & statusDetails)
{
    this->statusDetails_ = statusDetails;
    InnerUpdateStatus(SingleInstanceDeploymentStatus::Enum::Ready);
    return this->Complete(storeTx);
}

ErrorCode SingleInstanceDeploymentContext::SwitchReplaceToCreate(
    Store::StoreTransaction const & storeTx)
{
    this->statusDetails_ = L"";
    InnerUpdateStatus(SingleInstanceDeploymentStatus::Enum::Creating);
    return RolloutContext::SwitchReplaceToCreate(storeTx);
}

ErrorCode SingleInstanceDeploymentContext::StartUpgrading(
    Store::StoreTransaction const & storeTx)
{
    this->statusDetails_ = GET_RC( Deployment_Upgrade_In_Progress );
    InnerUpdateStatus(SingleInstanceDeploymentStatus::Enum::Upgrading);
    return this->UpdateStatus(storeTx, RolloutStatus::Pending);
}

ErrorCode SingleInstanceDeploymentContext::ClearUpgrading(
    Store::StoreTransaction const & storeTx,
    wstring const & statusDetails)
{
    return this->FinishCreating(storeTx, statusDetails);
}

void SingleInstanceDeploymentContext::InnerUpdateStatus(SingleInstanceDeploymentStatus::Enum const deploymentStatus)
{
    WriteNoise(TraceComponent, "{0} InnerUpdateStatus({1}, {2}", this->TraceId, deploymentStatus, this->SequenceNumber);
    this->DeploymentStatus = deploymentStatus;
}
