// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class TokenSecurityAccount : public SecurityUser
    {
        DENY_COPY(TokenSecurityAccount)

    public:
        TokenSecurityAccount(Common::AccessTokenSPtr);
        virtual ~TokenSecurityAccount();

        virtual Common::ErrorCode LoadAccount(std::wstring const& accountName);
        virtual Common::ErrorCode CreateLogonToken(__out Common::AccessTokenSPtr & userToken);

    private:
        Common::AccessTokenSPtr accessToken_;
    };
}
