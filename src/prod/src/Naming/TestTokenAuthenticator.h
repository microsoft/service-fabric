// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming 
{
    class TestTokenAuthenticator 
        : public Api::IClaimsTokenAuthenticator
    {
        DENY_COPY(TestTokenAuthenticator);

    public:
        TestTokenAuthenticator(){}

        Common::AsyncOperationSPtr BeginAuthenticate(
            std::wstring const& inputToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndAuthenticate(
            Common::AsyncOperationSPtr const& operation,
            __out Common::TimeSpan &expirationTime,
            __out std::vector<ServiceModel::Claim> &claimsList);

    private:
        class AuthenticateAsyncOperation;
    };
}
