// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServicePackageDownloadSpecification::ServicePackageDownloadSpecification()
    : packageName_(),
    packageRolloutVersion_()
{
}

ServicePackageDownloadSpecification::ServicePackageDownloadSpecification(
    wstring const & packageName, 
    RolloutVersion const & packageRolloutVersion)
    : packageName_(packageName),
    packageRolloutVersion_(packageRolloutVersion)
{
}

ServicePackageDownloadSpecification::ServicePackageDownloadSpecification(ServicePackageDownloadSpecification const & other)
    : packageName_(other.packageName_),
    packageRolloutVersion_(other.packageRolloutVersion_)
{
}

ServicePackageDownloadSpecification::ServicePackageDownloadSpecification(ServicePackageDownloadSpecification && other)
    : packageName_(move(other.packageName_)),
    packageRolloutVersion_(move(other.packageRolloutVersion_))
{
}

ServicePackageDownloadSpecification const & ServicePackageDownloadSpecification::operator = (ServicePackageDownloadSpecification const & other)
{
    if (this != &other)
    {
        this->packageName_ = other.packageName_;
        this->packageRolloutVersion_ = other.packageRolloutVersion_;
    }

    return *this;
}

ServicePackageDownloadSpecification const & ServicePackageDownloadSpecification::operator = (ServicePackageDownloadSpecification && other)
{
    if (this != &other)
    {
        this->packageName_ = move(other.packageName_);
        this->packageRolloutVersion_ = move(other.packageRolloutVersion_);
    }

    return *this;
}

void ServicePackageDownloadSpecification::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write("PackageName={0},RolloutVersion={1}", packageName_, packageRolloutVersion_);
}
