// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IClaimsTokenAuthenticator
    {
    public:
        virtual ~IClaimsTokenAuthenticator() {};

        virtual Common::AsyncOperationSPtr BeginAuthenticate(
            std::wstring const& inputToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) = 0;

        virtual Common::ErrorCode EndAuthenticate(
            Common::AsyncOperationSPtr const& operation,
            __out Common::TimeSpan &expirationTime,
            __out std::vector<ServiceModel::Claim> &claimsList) = 0;
    };
}
