// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ITokenValidationService
    {
    public:
        virtual ~ITokenValidationService() {};

        virtual Common::AsyncOperationSPtr BeginValidateToken(
            std::wstring const & authToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndValidateToken(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out IFabricTokenClaimResult **) = 0;

        virtual Common::ErrorCode GetTokenServiceMetadata(
            __out IFabricTokenServiceMetadataResult **) = 0;
    };
}
