// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class IInternalTokenValidationServiceClient
    {
    public:
        virtual ~IInternalTokenValidationServiceClient() {};

        virtual Common::AsyncOperationSPtr BeginGetMetadata(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndGetMetadata(
            Common::AsyncOperationSPtr const &,
            __out ServiceModel::TokenValidationServiceMetadata &) = 0;

        virtual Common::AsyncOperationSPtr BeginValidateToken(
            std::wstring const &token,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndValidateToken(
            Common::AsyncOperationSPtr const &,
            __out Common::TimeSpan &expirationTime,
            __out std::vector<ServiceModel::Claim> &claimsList) = 0;

    };
}
