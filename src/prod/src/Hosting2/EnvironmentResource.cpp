// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;

EnvironmentResource::EnvironmentResource() 
    : resourceAccessEntries_() 
{
}

EnvironmentResource::~EnvironmentResource()
{
}

void EnvironmentResource::AddSecurityAccessPolicy(
    SecurityPrincipalInformationSPtr const & principalInfo,
    ServiceModel::GrantAccessType::Enum accessType)
{
    ResourceAccess resourceAccess;
    resourceAccess.PrincipalInfo = principalInfo;
    resourceAccess.AccessType = accessType;

    resourceAccessEntries_.push_back(move(resourceAccess));
}

Common::ErrorCode EnvironmentResource::GetDefaultSecurityAccess(
    __out ResourceAccess & securityAccess) const
{
    if (resourceAccessEntries_.empty())
    {
        return ErrorCode(ErrorCodeValue::NotFound);
    }

    if (resourceAccessEntries_.size() > 1)
    {
        Assert::CodingError("Currently we don't support associated multiple policies with this resource");
    }
            
    securityAccess = resourceAccessEntries_[0];
    return ErrorCode(ErrorCodeValue::Success);
}
