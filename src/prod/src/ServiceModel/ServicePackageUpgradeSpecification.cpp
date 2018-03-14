// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServicePackageUpgradeSpecification::ServicePackageUpgradeSpecification()
    : packageName_(),
    packageRolloutVersion_(),
    isCodePackageInfoIncluded_(false),
    codePackageNames_()
{
}

ServicePackageUpgradeSpecification::ServicePackageUpgradeSpecification(
    wstring const & packageName, 
    RolloutVersion const & packageRolloutVersion,
    vector<wstring> const & codePackageNames)
    : packageName_(packageName),
    packageRolloutVersion_(packageRolloutVersion),
    isCodePackageInfoIncluded_(true),
    codePackageNames_(codePackageNames)
{
}

ServicePackageUpgradeSpecification::ServicePackageUpgradeSpecification(ServicePackageUpgradeSpecification const & other)
    : packageName_(other.packageName_),
    packageRolloutVersion_(other.packageRolloutVersion_),
    isCodePackageInfoIncluded_(other.isCodePackageInfoIncluded_),
    codePackageNames_(other.codePackageNames_)
{
}

ServicePackageUpgradeSpecification::ServicePackageUpgradeSpecification(ServicePackageUpgradeSpecification && other)
    : packageName_(move(other.packageName_)),
    packageRolloutVersion_(move(other.packageRolloutVersion_)),
    isCodePackageInfoIncluded_(other.isCodePackageInfoIncluded_),
    codePackageNames_(move(other.codePackageNames_))
{
}

ServicePackageUpgradeSpecification const & ServicePackageUpgradeSpecification::operator = (ServicePackageUpgradeSpecification const & other)
{
    if (this != &other)
    {
        this->packageName_ = other.packageName_;
        this->packageRolloutVersion_ = other.packageRolloutVersion_;
        this->isCodePackageInfoIncluded_ = other.isCodePackageInfoIncluded_;
        this->codePackageNames_ = other.codePackageNames_;
    }

    return *this;
}

ServicePackageUpgradeSpecification const & ServicePackageUpgradeSpecification::operator = (ServicePackageUpgradeSpecification && other)
{
    if (this != &other)
    {
        this->packageName_ = move(other.packageName_);
        this->packageRolloutVersion_ = move(other.packageRolloutVersion_);
        this->isCodePackageInfoIncluded_ = other.isCodePackageInfoIncluded_;
        this->codePackageNames_ = move(other.codePackageNames_);
    }

    return *this;
}

void ServicePackageUpgradeSpecification::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w.Write(
       "PackageName={0},RolloutVersion={1}", 
       packageName_, 
       packageRolloutVersion_);

   if (isCodePackageInfoIncluded_)
   {
       w.Write(", CodePackages={0}", codePackageNames_);
   }
}
