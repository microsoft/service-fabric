// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyTokenValidationService :
        public Common::ComponentRoot,
        public ITokenValidationService
    {
        DENY_COPY(ComProxyTokenValidationService);

    public:
        ComProxyTokenValidationService(Common::ComPointer<IFabricTokenValidationService> const & comImpl);
        virtual ~ComProxyTokenValidationService();

        virtual Common::AsyncOperationSPtr BeginValidateToken(
            std::wstring const & authToken,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndValidateToken(
            Common::AsyncOperationSPtr const & asyncOperation,
            IFabricTokenClaimResult ** result);

        virtual Common::ErrorCode GetTokenServiceMetadata(IFabricTokenServiceMetadataResult ** result);

    private:
        class ValidateTokenAsyncOperation;

        Common::ComPointer<IFabricTokenValidationService> const comImpl_;
    };
}
