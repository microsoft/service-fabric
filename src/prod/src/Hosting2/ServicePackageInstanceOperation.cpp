// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServicePackageInstanceOperation::ServicePackageInstanceOperation()
    : activationContext_()
    , packageOperation_()
	, servicePackagePublicActivationId_()
    , operationId_()
{
}

ServicePackageInstanceOperation::ServicePackageInstanceOperation(
    wstring const & packageName,
    ServicePackageActivationContext activationContext,
	wstring const & servicePackagePublicActivationId,
    RolloutVersion const & packageRolloutVersion,
    int64 const versionInstanceId,
    ServicePackageDescription const & packageDescription,
    ServiceModel::UpgradeType::Enum const upgradeType)
    : activationContext_(activationContext)
	, servicePackagePublicActivationId_(servicePackagePublicActivationId)
    , packageOperation_(packageName, packageRolloutVersion, versionInstanceId, packageDescription, upgradeType)
    , operationId_(wformatString("{0}_{1}", packageName, activationContext.ToString()))
{
}

ServicePackageInstanceOperation::ServicePackageInstanceOperation(
    ServicePackageOperation const & packageOperation,
    ServicePackageActivationContext const & activationContext,
	wstring const & servicePackagePublicActivationId)
    : activationContext_(activationContext)
	, servicePackagePublicActivationId_(servicePackagePublicActivationId)
    , packageOperation_(packageOperation)
    , operationId_(wformatString("{0}_{1}", packageOperation.PackageName, activationContext.ToString()))
{
}

ServicePackageInstanceOperation::ServicePackageInstanceOperation(ServicePackageInstanceOperation const & other)
    : activationContext_(other.activationContext_)
	, servicePackagePublicActivationId_(other.servicePackagePublicActivationId_)
    , packageOperation_(other.packageOperation_)
    , operationId_(other.operationId_)
{
}

ServicePackageInstanceOperation::ServicePackageInstanceOperation(ServicePackageInstanceOperation && other)
    : activationContext_(move(other.activationContext_))
	, servicePackagePublicActivationId_(move(other.servicePackagePublicActivationId_))
    , packageOperation_(move(other.packageOperation_))
    , operationId_(move(other.operationId_))
{
}

ServicePackageInstanceOperation const & ServicePackageInstanceOperation::operator = (ServicePackageInstanceOperation const & other)
{
    if (this != &other)
    {
        this->activationContext_ = other.activationContext_;
		this->servicePackagePublicActivationId_ = other.servicePackagePublicActivationId_;
        this->packageOperation_ = other.packageOperation_;
        this->operationId_ = other.operationId_;
    }

    return *this;
}

ServicePackageInstanceOperation const & ServicePackageInstanceOperation::operator = (ServicePackageInstanceOperation && other)
{
    if (this != &other)
    {
        this->activationContext_ = move(other.activationContext_);
		this->servicePackagePublicActivationId_ = move(other.servicePackagePublicActivationId_);
        this->packageOperation_ = move(other.packageOperation_);
        this->operationId_ = move(other.operationId_);
    }

    return *this;
}

void ServicePackageInstanceOperation::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(
        "PackageName={0}, ActivationContext={1}, ServicePackagePublicActivationId={2}, RolloutVersion={3}, VersionInstanceId={4}, UpgradeType={5}",
        ServicePackageName, 
        ActivationContext,
		PublicActivationId,
        PackageRolloutVersion,
        VersionInstanceId,
        UpgradeType);
}

