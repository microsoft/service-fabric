// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ApplicationUpgradeUpdateDescription");

ApplicationUpgradeUpdateDescription::ApplicationUpgradeUpdateDescription() 
    : applicationName_()
    , upgradeType_(UpgradeType::Rolling)
    , updateDescription_()
    , healthPolicy_()
{ 
}

ApplicationUpgradeUpdateDescription::ApplicationUpgradeUpdateDescription(ApplicationUpgradeUpdateDescription && other) 
    : applicationName_(move(other.applicationName_))
    , upgradeType_(move(other.upgradeType_))
    , updateDescription_(move(other.updateDescription_))
    , healthPolicy_(move(other.healthPolicy_))
{ 
}

bool ApplicationUpgradeUpdateDescription::operator == (ApplicationUpgradeUpdateDescription const & other) const
{
    if (applicationName_ != other.applicationName_) { return false; }

    if (upgradeType_ != other.upgradeType_) { return false; }

    CHECK_EQUALS_IF_NON_NULL( updateDescription_ )

    CHECK_EQUALS_IF_NON_NULL( healthPolicy_ )

    return true;
}

bool ApplicationUpgradeUpdateDescription::operator != (ApplicationUpgradeUpdateDescription const & other) const
{
    return !(*this == other);
}

void ApplicationUpgradeUpdateDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w << "[ " << applicationName_ << ", " << upgradeType_;
   
   if (updateDescription_) { w << ", " << *updateDescription_; }

   if (healthPolicy_) { w << ", " << *healthPolicy_; }

   w << " ]";
}

Common::ErrorCode ApplicationUpgradeUpdateDescription::FromPublicApi(
    FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION const & publicDescription)
{
    wstring applicationNameString;
    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.ApplicationName, false, applicationNameString);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    if (!NamingUri::TryParse(applicationNameString, applicationName_))
    {
        return ErrorCodeValue::InvalidNameUri;
    }

    return ModifyUpgradeHelper::FromPublicUpdateDescription<
        ApplicationUpgradeUpdateDescription,
        FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION,
        ApplicationHealthPolicy,
        FABRIC_APPLICATION_HEALTH_POLICY>(
            *this,
            publicDescription);
}
