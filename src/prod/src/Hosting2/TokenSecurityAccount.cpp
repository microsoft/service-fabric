// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceTokenSecurityAccount("TokenSecurityAccount");

TokenSecurityAccount::TokenSecurityAccount(AccessTokenSPtr accessToken) :
    SecurityUser(L"", L"", L"", L"", false, SecurityPrincipalAccountType::Enum::NetworkService, vector<wstring>(), vector<wstring>()),
    accessToken_(accessToken)
{
    
}

TokenSecurityAccount::~TokenSecurityAccount()
{
}

ErrorCode TokenSecurityAccount::LoadAccount(wstring const& accountName)
{
    ASSERT_IFNOT(accountName.empty(), "AccountName cannot be present for TokenSecurityAccount");
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode TokenSecurityAccount::CreateLogonToken(__out AccessTokenSPtr & userToken)
{
    if(this->accessToken_)
    {
        userToken = accessToken_;
    }
    return ErrorCode(ErrorCodeValue::Success);
}
