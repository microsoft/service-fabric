// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

InternalProvisionedApplicationTypeQueryResult::InternalProvisionedApplicationTypeQueryResult()
    : appTypeName_(),
    serviceManifests_(),
    codePackages_(),
    configPackages_(),
    dataPackages_()
{
}

InternalProvisionedApplicationTypeQueryResult::InternalProvisionedApplicationTypeQueryResult(
    wstring const & appTypeName,
    vector<wstring> const & serviceManifests,
    vector<wstring> const & codePackages,
    vector<wstring> const & configPackages,
    vector<wstring> const & dataPackages)
    : appTypeName_(appTypeName), 
    serviceManifests_(serviceManifests),
    codePackages_(codePackages),
    configPackages_(configPackages),
    dataPackages_(dataPackages)

{
}

InternalProvisionedApplicationTypeQueryResult::InternalProvisionedApplicationTypeQueryResult(
    InternalProvisionedApplicationTypeQueryResult && other) 
    : appTypeName_(move(other.appTypeName_)),
    serviceManifests_(move(other.serviceManifests_)),
    codePackages_(move(other.codePackages_)),
    configPackages_(move(other.configPackages_)),
    dataPackages_(move(other.dataPackages_))

{
}

InternalProvisionedApplicationTypeQueryResult const & InternalProvisionedApplicationTypeQueryResult::operator=(InternalProvisionedApplicationTypeQueryResult && other)
{
    if(this != &other)
    {
        this->appTypeName_ = move(other.appTypeName_);
        this->serviceManifests_ = move(other.serviceManifests_);
        this->codePackages_ = move(other.codePackages_);
        this->configPackages_ = move(other.configPackages_);
        this->dataPackages_ = move(other.dataPackages_);
    }
    return *this;
}

void InternalProvisionedApplicationTypeQueryResult::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("ApplicationType Name = {0} ", appTypeName_);
}
