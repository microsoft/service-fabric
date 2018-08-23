//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Store;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("SingleInstanceDeploymentUpgradeContext");

SingleInstanceDeploymentUpgradeContext::SingleInstanceDeploymentUpgradeContext()
    : RolloutContext(RolloutContextType::SingleInstanceDeploymentUpgrade)
    , deploymentName_()
    , deploymentType_(DeploymentType::Enum::Invalid)
    , applicationName_()
    , upgradeDescription_()
    , appTypeName_()
    , currentTypeVersion_()
    , targetTypeVersion_()
    , versionToUnprovision_()
    , currentStatus_(SingleInstanceDeploymentUpgradeState::Enum::Invalid)
    , statusDetails_()
    , isInterrupted_(false)
{
}

SingleInstanceDeploymentUpgradeContext::SingleInstanceDeploymentUpgradeContext(wstring const &deploymentName)
    : RolloutContext(RolloutContextType::SingleInstanceDeploymentUpgrade)
    , deploymentName_(deploymentName)
    , deploymentType_(DeploymentType::Enum::Invalid)
    , applicationName_()
    , upgradeDescription_()
    , appTypeName_()
    , currentTypeVersion_()
    , targetTypeVersion_()
    , versionToUnprovision_()
    , currentStatus_(SingleInstanceDeploymentUpgradeState::Enum::Invalid)
    , statusDetails_()
    , isInterrupted_(false)
{
}

SingleInstanceDeploymentUpgradeContext::SingleInstanceDeploymentUpgradeContext(
    ComponentRoot const & replica,
    ClientRequestSPtr const & request,
    wstring const &deploymentName,
    DeploymentType::Enum const deploymentType,
    NamingUri const &applicationName,
    shared_ptr<ApplicationUpgradeDescription> const &upgradeDescription,
    ServiceModelTypeName const &typeName,
    ServiceModelVersion const &currentVersion,
    ServiceModelVersion const &targetVersion)
    : RolloutContext(RolloutContextType::SingleInstanceDeploymentUpgrade, replica, request)
    , deploymentName_(deploymentName)
    , deploymentType_(deploymentType)
    , applicationName_(applicationName)
    , upgradeDescription_(upgradeDescription)
    , appTypeName_(typeName)
    , currentTypeVersion_(currentVersion)
    , targetTypeVersion_(targetVersion)
    , versionToUnprovision_(currentVersion)
    , currentStatus_(SingleInstanceDeploymentUpgradeState::Enum::ProvisioningTarget)
    , statusDetails_(wformatString( GET_RC( Deployment_Provisioning ), targetVersion.Value))
    , isInterrupted_(false)
{

}

wstring const & SingleInstanceDeploymentUpgradeContext::get_Type() const
{
    return Constants::StoreType_SingleInstanceDeploymentUpgradeContext;
}

wstring SingleInstanceDeploymentUpgradeContext::ConstructKey() const
{
    wstring temp;
    StringWriter writer(temp);
    writer.Write("{0}", deploymentName_);
    return temp;
}

void SingleInstanceDeploymentUpgradeContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "SingleInstanceDeploymentUpgradeContext({0})[DeploymentName : {1}, DeploymentType: {2}, Deployment State : {3}, {4}, App type name: {5}, Current Version: {6}, Target Version: {7}, isInterrupted: {8}]",
        this->Status, 
        deploymentName_,
        deploymentType_,
        currentStatus_,
        statusDetails_,
        appTypeName_, 
        currentTypeVersion_,
        targetTypeVersion_,
        isInterrupted_);
}

ErrorCode SingleInstanceDeploymentUpgradeContext::StartUpgrade(Store::StoreTransaction const &storeTx, std::wstring const &statusDetails)
{
    InnerUpdateUpgradeStatus(SingleInstanceDeploymentUpgradeState::Enum::RollingForward);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode SingleInstanceDeploymentUpgradeContext::StartRollback(Store::StoreTransaction const &storeTx, std::wstring const &statusDetails)
{
    InnerUpdateUpgradeStatus(SingleInstanceDeploymentUpgradeState::Enum::RollingBack);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode SingleInstanceDeploymentUpgradeContext::StartUnprovision(Store::StoreTransaction const &storeTx, std::wstring const &statusDetails)
{
    InnerUpdateUpgradeStatus(SingleInstanceDeploymentUpgradeState::Enum::UnprovisioningCurrent);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode SingleInstanceDeploymentUpgradeContext::StartUnprovisionTarget(Store::StoreTransaction const &storeTx, wstring const & statusDetails)
{
    InnerUpdateUpgradeStatus(SingleInstanceDeploymentUpgradeState::Enum::UnprovisioningTarget);
    statusDetails_ = statusDetails;
    return storeTx.Update(*this);
}

ErrorCode SingleInstanceDeploymentUpgradeContext::FinishUpgrading(StoreTransaction const &storeTx)
{
    // Leave the rollback or upgrading status details as is, 
    // so that the descriptive status is present when queried later.

    if (currentStatus_ == SingleInstanceDeploymentUpgradeState::Enum::UnprovisioningTarget)
    {
        this->InnerUpdateUpgradeStatus(SingleInstanceDeploymentUpgradeState::Enum::CompletedRollback);
    }
    else
    {
        this->InnerUpdateUpgradeStatus(SingleInstanceDeploymentUpgradeState::Enum::CompletedRollforward);
    }

    return this->UpdateStatus(storeTx, RolloutStatus::Completed);
}

ErrorCode SingleInstanceDeploymentUpgradeContext::UpdateUpgradeStatus(Store::StoreTransaction const & storeTx, SingleInstanceDeploymentUpgradeState::Enum const status)
{
    InnerUpdateUpgradeStatus(status);
    return storeTx.Update(*this);
}

ErrorCode SingleInstanceDeploymentUpgradeContext::TryInterrupt(StoreTransaction const &storeTx)
{
    if (currentStatus_ == SingleInstanceDeploymentUpgradeState::Enum::ProvisioningTarget)
    {
        // can be interrupted.
        isInterrupted_ = true;
        return storeTx.Update(*this);
    }
    else if (currentStatus_ == SingleInstanceDeploymentUpgradeState::Enum::RollingForward)
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
                storeTx.Update(*this); 
            }

            return error;
        }
    }
    else
    {
        return ErrorCodeValue::ApplicationAlreadyInTargetVersion;
    }

    return ErrorCodeValue::Success;
}

void SingleInstanceDeploymentUpgradeContext::InnerUpdateUpgradeStatus(SingleInstanceDeploymentUpgradeState::Enum const status)
{
    this->currentStatus_ = status;
}
