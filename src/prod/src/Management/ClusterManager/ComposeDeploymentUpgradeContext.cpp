// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ComposeDeploymentUpgradeContext");

RolloutContextType::Enum const ComposeDeploymentUpgradeContextType(RolloutContextType::ComposeDeploymentUpgrade);

ComposeDeploymentUpgradeContext::ComposeDeploymentUpgradeContext()
    : RolloutContext(ComposeDeploymentUpgradeContextType)
    , deploymentName_()
    , applicationName_()
    , upgradeDescription_()
    , appTypeName_()
    , currentTypeVersion_()
    , targetTypeVersion_()
    , versionToUnprovision_()
    , currentStatus_(ComposeDeploymentUpgradeState::Enum::Invalid)
    , statusDetails_()
    , isInterrupted_(false)
{
}

ComposeDeploymentUpgradeContext::ComposeDeploymentUpgradeContext(
    ComposeDeploymentUpgradeContext && other)
    : RolloutContext(ComposeDeploymentUpgradeContextType)
    , deploymentName_(move(other.deploymentName_))
    , applicationName_(move(other.applicationName_))
    , upgradeDescription_(move(other.upgradeDescription_))
    , appTypeName_(move(other.appTypeName_))
    , currentTypeVersion_(move(other.currentTypeVersion_))
    , targetTypeVersion_(move(other.targetTypeVersion_))
    , versionToUnprovision_(move(other.versionToUnprovision_))
    , currentStatus_(other.currentStatus_)
    , statusDetails_(other.statusDetails_)
    , isInterrupted_(other.isInterrupted_)
{
}

ComposeDeploymentUpgradeContext::ComposeDeploymentUpgradeContext(wstring const &deploymentName)
    : RolloutContext(ComposeDeploymentUpgradeContextType)
    , deploymentName_(deploymentName)
    , applicationName_()
    , upgradeDescription_()
    , appTypeName_()
    , currentTypeVersion_()
    , targetTypeVersion_()
    , versionToUnprovision_()
    , currentStatus_(ComposeDeploymentUpgradeState::Enum::Invalid)
    , statusDetails_()
    , isInterrupted_(false)
{
}

ComposeDeploymentUpgradeContext::ComposeDeploymentUpgradeContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    wstring const &deploymentName,
    NamingUri const &applicationName,
    shared_ptr<ApplicationUpgradeDescription> const &upgradeDescription,
    ServiceModelTypeName const &typeName,
    ServiceModelVersion const &currentVersion,
    ServiceModelVersion const &targetVersion)
    : RolloutContext(ComposeDeploymentUpgradeContextType, replica, request)
    , deploymentName_(deploymentName)
    , applicationName_(applicationName)
    , upgradeDescription_(upgradeDescription)
    , appTypeName_(typeName)
    , currentTypeVersion_(currentVersion)
    , targetTypeVersion_(targetVersion)
    , versionToUnprovision_(currentVersion)
    , currentStatus_(ComposeDeploymentUpgradeState::Enum::ProvisioningTarget)
    , statusDetails_(wformatString( GET_RC( Deployment_Provisioning ), targetVersion.Value))
    , isInterrupted_(false)
{

}

ComposeDeploymentUpgradeContext & ComposeDeploymentUpgradeContext::operator=(
    ComposeDeploymentUpgradeContext && other)
{
    if (this != &other)
    {
        __super::operator = (move(other));
        deploymentName_ = move(other.deploymentName_);
        applicationName_ = move(other.applicationName_);
        upgradeDescription_ = move(other.upgradeDescription_);
        appTypeName_ = move(other.appTypeName_);
        currentTypeVersion_ = move(other.currentTypeVersion_);
        targetTypeVersion_ = move(other.targetTypeVersion_);
        versionToUnprovision_ = move(other.versionToUnprovision_);
        currentStatus_ = other.currentStatus_;
        statusDetails_ = other.statusDetails_;
        isInterrupted_ = other.isInterrupted_;
    }

    return *this;
}

std::wstring const & ComposeDeploymentUpgradeContext::get_Type() const
{
    return Constants::StoreType_ComposeDeploymentUpgradeContext;
}

std::wstring ComposeDeploymentUpgradeContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}", deploymentName_);
    return temp;
}

void ComposeDeploymentUpgradeContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "ComposeDeploymentUpgradeContext({0})[DeploymentName : {1}, Deployment State : {2}, {3}, App type name: {4}, Current Version: {5}, Target Version: {6}, isInterrupted: {7}]",
        this->Status, 
        deploymentName_,
        currentStatus_,
        statusDetails_,
        appTypeName_, 
        currentTypeVersion_,
        targetTypeVersion_,
        isInterrupted_);
}

ErrorCode ComposeDeploymentUpgradeContext::StartUpgrade(Store::StoreTransaction const &storeTx, std::wstring const &statusDetails)
{
    InnerUpdateUpgradeStatus(ComposeDeploymentUpgradeState::Enum::RollingForward);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode ComposeDeploymentUpgradeContext::StartRollback(Store::StoreTransaction const &storeTx, std::wstring const &statusDetails)
{
    InnerUpdateUpgradeStatus(ComposeDeploymentUpgradeState::Enum::RollingBack);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode ComposeDeploymentUpgradeContext::StartUnprovision(Store::StoreTransaction const &storeTx, std::wstring const &statusDetails)
{
    InnerUpdateUpgradeStatus(ComposeDeploymentUpgradeState::Enum::UnprovisioningCurrent);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode ComposeDeploymentUpgradeContext::StartUnprovisionTarget(Store::StoreTransaction const &storeTx, wstring const & statusDetails)
{
    InnerUpdateUpgradeStatus(ComposeDeploymentUpgradeState::Enum::UnprovisioningTarget);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode ComposeDeploymentUpgradeContext::FinishUpgrading(StoreTransaction const &storeTx)
{
    // Leave the rollback or upgrading status details as is, 
    // so that the descriptive status is present when queried later.

    if (currentStatus_ == ComposeDeploymentUpgradeState::Enum::UnprovisioningTarget)
    {
        this->InnerUpdateUpgradeStatus(ComposeDeploymentUpgradeState::Enum::CompletedRollback);
    }
    else
    {
        this->InnerUpdateUpgradeStatus(ComposeDeploymentUpgradeState::Enum::CompletedRollforward);
    }

    return this->UpdateStatus(storeTx, RolloutStatus::Completed);
}

ErrorCode ComposeDeploymentUpgradeContext::UpdateUpgradeStatus(Store::StoreTransaction const & storeTx, ComposeDeploymentUpgradeState::Enum const status)
{
    InnerUpdateUpgradeStatus(status);
    return storeTx.Update(*this);
}

ErrorCode ComposeDeploymentUpgradeContext::TryInterrupt(StoreTransaction const &storeTx)
{
    if (isInterrupted_)
    {
        return ErrorCodeValue::SingleInstanceApplicationUpgradeInProgress;
    }
    else if (currentStatus_ == ComposeDeploymentUpgradeState::Enum::ProvisioningTarget)
    {
        // can be interrupted.
        isInterrupted_ = true;
        return storeTx.Update(*this);
    }
    else if (currentStatus_ == ComposeDeploymentUpgradeState::Enum::RollingForward)
    {
        auto appUpgradeContext = make_unique<ApplicationUpgradeContext>(this->ApplicationName);
        auto error = storeTx.ReadExact(*appUpgradeContext);
        if (!error.IsSuccess())
        {
            return error;
        }

        bool interrupted = appUpgradeContext->TryInterrupt();
        if (interrupted)
        {
            error = storeTx.Update(*appUpgradeContext);
            if (error.IsSuccess()) 
            { 
                isInterrupted_ = true;
                return storeTx.Update(*this); 
            }
        }
    }
    else 
    {
        return ErrorCodeValue::ApplicationAlreadyInTargetVersion;
    }

    return ErrorCodeValue::Success;
}

void ComposeDeploymentUpgradeContext::InnerUpdateUpgradeStatus(ComposeDeploymentUpgradeState::Enum const status)
{
    this->currentStatus_ = status;
}
