// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServicePackageOperation::ServicePackageOperation()
    : packageName_(),
    packageRolloutVersion_(RolloutVersion::Zero),
    versionInstanceId_(0),
    packageDescription_(),
    upgradeType_(UpgradeType::Invalid)
{
}

ServicePackageOperation::ServicePackageOperation(
    wstring const & packageName, 
    RolloutVersion const & packageRolloutVersion,
    int64 const versionInstanceId,
    ServicePackageDescription const & packageDescription,
    ServiceModel::UpgradeType::Enum const upgradeType)
    : packageName_(packageName),
    packageRolloutVersion_(packageRolloutVersion),
    versionInstanceId_(versionInstanceId),
    packageDescription_(packageDescription),
    upgradeType_(upgradeType)
{
}

ServicePackageOperation::ServicePackageOperation(ServicePackageOperation const & other)
    : packageName_(other.packageName_),
    packageRolloutVersion_(other.packageRolloutVersion_),
    versionInstanceId_(other.versionInstanceId_),
    packageDescription_(other.packageDescription_),
    upgradeType_(other.upgradeType_)
{
}

ServicePackageOperation::ServicePackageOperation(ServicePackageOperation && other)
    : packageName_(move(other.packageName_)),
    packageRolloutVersion_(move(other.packageRolloutVersion_)),
    versionInstanceId_(other.versionInstanceId_),
    packageDescription_(move(other.packageDescription_)),
    upgradeType_(other.upgradeType_)
{
}

ServicePackageOperation const & ServicePackageOperation::operator = (ServicePackageOperation const & other)
{
    if (this != &other)
    {
        this->packageName_ = other.packageName_;
        this->packageRolloutVersion_ = other.packageRolloutVersion_;
        this->versionInstanceId_ = other.versionInstanceId_;
        this->packageDescription_ = other.packageDescription_;            
        this->upgradeType_ = other.upgradeType_;
    }

    return *this;
}

ServicePackageOperation const & ServicePackageOperation::operator = (ServicePackageOperation && other)
{
    if (this != &other)
    {
        this->packageName_ = move(other.packageName_);
        this->packageRolloutVersion_ = move(other.packageRolloutVersion_);
        this->versionInstanceId_ = other.versionInstanceId_;
        this->packageDescription_ = move(other.packageDescription_);
        this->upgradeType_ = other.upgradeType_;
    }

    return *this;
}

void ServicePackageOperation::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("Name={0},RolloutVersion={1},VersionInstanceId={2},UpgradeType={3}", packageName_, packageRolloutVersion_, versionInstanceId_, upgradeType_);
}

