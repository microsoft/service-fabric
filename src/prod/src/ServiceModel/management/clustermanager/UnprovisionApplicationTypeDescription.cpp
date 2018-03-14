// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("UnprovisionApplicationTypeDescription");

UnprovisionApplicationTypeDescription::UnprovisionApplicationTypeDescription()
    : appTypeName_()
    , appTypeVersion_()
    , isAsync_(false)
{
}

UnprovisionApplicationTypeDescription::UnprovisionApplicationTypeDescription(
    std::wstring const & appTypeName,
    std::wstring const & appTypeVersion,
    bool isAsync)
    : appTypeName_(appTypeName)
    , appTypeVersion_(appTypeVersion)
    , isAsync_(isAsync)
{
}

UnprovisionApplicationTypeDescription::~UnprovisionApplicationTypeDescription()
{
}

ErrorCode UnprovisionApplicationTypeDescription::FromPublicApi(FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION const & publicDescription)
{
    TRY_PARSE_PUBLIC_STRING(publicDescription.ApplicationTypeName, appTypeName_);
    TRY_PARSE_PUBLIC_STRING(publicDescription.ApplicationTypeVersion, appTypeVersion_);

    isAsync_ = (publicDescription.Async == TRUE);
    
    return ErrorCode::Success();
}
