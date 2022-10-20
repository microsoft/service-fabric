// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(ServicePropertiesBase)

bool ServicePropertiesBase::operator==(ServicePropertiesBase const & other) const
{
    if (osType_ == other.osType_
        && codePackages_ == other.codePackages_
        && autoScalingPolicies_ == other.autoScalingPolicies_
        && networkRefs_ == other.networkRefs_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool ServicePropertiesBase::CanUpgrade(ServicePropertiesBase const & other) const
{
    if (osType_ != other.osType_
        || networkRefs_ != other.networkRefs_
        || diagnostics_ != other.diagnostics_)
    {
        return false;
    }

    // We only check codePackage both in *this and other, to make sure they CanUpgrade()
    for (auto const & codePackage : codePackages_)
    {
        auto findInOther = find_if(other.codePackages_.begin(), other.codePackages_.end(),
            [&codePackage](ContainerCodePackageDescription const & c) { return c.Name == codePackage.Name; });
        if (findInOther != other.codePackages_.end() && !codePackage.CanUpgrade(*findInOther))
        {
            return false;
        }
    }

    return true;
}
