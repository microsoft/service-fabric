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

StringLiteral const TracePrincipalsProviderContext("PrincipalsProviderContext");

PrincipalsProviderContext::PrincipalsProviderContext() 
    : securityPrincipalInformation_(),
    lock_()
{
}

PrincipalsProviderContext::~PrincipalsProviderContext()
{
}

void PrincipalsProviderContext::AddSecurityPrincipals(vector<SecurityPrincipalInformation> const & principalsInfo)
{
    for(auto iter = principalsInfo.begin(); iter != principalsInfo.end(); ++iter)
    {
        auto principal = make_shared<SecurityPrincipalInformation>((*iter).Name, (*iter).SecurityPrincipalId, (*iter).SidString, (*iter).IsLocalUser);
        securityPrincipalInformation_.push_back(move(principal));
    }
}

ErrorCode PrincipalsProviderContext::TryGetPrincipalId(wstring const & name, __out wstring & userId) const
{
    AcquireReadLock lock(lock_);
    for (auto it = securityPrincipalInformation_.begin(); it != securityPrincipalInformation_.end(); ++it)
    {
        if (StringUtility::AreEqualCaseInsensitive((*it)->Name, name))
        {
            userId = (*it)->SecurityPrincipalId;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
    return ErrorCode(ErrorCodeValue::ApplicationPrincipalDoesNotExist);
}

vector<SidSPtr> const & PrincipalsProviderContext::GetPrincipalSids()
{
    ASSERT_IF(principalSids_.size() != securityPrincipalInformation_.size(), "InitializaeSecurityPrincipalSids must be called before GetPrincipalSids");
    return principalSids_;
}

ErrorCode PrincipalsProviderContext::InitializeSecurityPrincipalSids()
{
    if(principalSids_.empty())
    {
        AcquireWriteLock lock(lock_);
        if(principalSids_.empty())
        {
            for (auto it = securityPrincipalInformation_.begin(); it != securityPrincipalInformation_.end(); ++it)
            {
                SidSPtr userSid;
                auto error = BufferedSid::CreateSPtrFromStringSid((*it)->SidString, userSid);
                if(!error.IsSuccess())
                {
                    principalSids_.clear();
                    return error;
                }
                principalSids_.push_back(move(userSid));
            }
        }
    }
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode PrincipalsProviderContext::TryGetPrincipalInfo(
    wstring const & name,
    __out SecurityPrincipalInformationSPtr & principalInfo)
{
    for (auto it = securityPrincipalInformation_.begin(); it != securityPrincipalInformation_.end(); ++it)
    {
        if (StringUtility::AreEqualCaseInsensitive((*it)->Name, name))
        {
            principalInfo = *it;
            return ErrorCode(ErrorCodeValue::Success);
        }
    }
    return ErrorCode(ErrorCodeValue::ApplicationPrincipalDoesNotExist);
}
