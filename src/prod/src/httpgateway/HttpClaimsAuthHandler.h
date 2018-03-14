// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpClaimsAuthHandler : public HttpAuthHandler
    {
    public:
        HttpClaimsAuthHandler(
            __in Transport::SecurityProvider::Enum handlerType,
            __in FabricClientWrapperSPtr &client);

        void OnCheckAccess(Common::AsyncOperationSPtr const& operation);

        Common::ErrorCode OnInitialize(Transport::SecuritySettings const& securitySettings);
        Common::ErrorCode OnSetSecurity(Transport::SecuritySettings const& securitySettings);

    private:

        void OnValidateTokenComplete(
            Common::AsyncOperationSPtr const& operation, 
            bool expectedCompletedSynchronously);

        Common::ErrorCode ExtractJwtFromAuthHeader(
            std::wstring const & authHeader,
            __out std::wstring & jwt);

        void ValidateAadToken(
            Common::AsyncOperationSPtr const & operation,
            std::wstring const & authorization);

        void FinishAccessCheck(
            Common::AsyncOperationSPtr const & operation,
            HttpServer::IRequestMessageContext &,
            Transport::SecuritySettings::RoleClaims &);

        void LoadIsAadServiceEnabled();

        FabricClientWrapperSPtr & client_;
        Transport::SecuritySettings::RoleClaimsOrList clientClaims_;
        Transport::SecuritySettings::RoleClaimsOrList adminClientClaims_;
        bool isAadServiceEnabled_;
    };
}
