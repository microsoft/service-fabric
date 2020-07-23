// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpKerberosAuthHandler : public HttpAuthHandler
    {
    public:
        HttpKerberosAuthHandler(
            __in Transport::SecurityProvider::Enum handlerType,
            __in FabricClientWrapperSPtr &client)
            : HttpAuthHandler(handlerType)
        {
            UNREFERENCED_PARAMETER(client);
        }

        void OnCheckAccess(Common::AsyncOperationSPtr const& operation);

        Common::ErrorCode OnInitialize(Transport::SecuritySettings const& securitySettings);
        Common::ErrorCode OnSetSecurity(Transport::SecuritySettings const& securitySettings);

    private:

        void UpdateAuthenticationHeaderOnFailure(Common::AsyncOperationSPtr const& operation);

        bool CheckToken(
            __in HttpServer::IRequestMessageContext const& request,
            HANDLE const& hToken,
            PSECURITY_DESCRIPTOR const& securityDescriptor,
            __in DWORD desiredAccess,
            __in PGENERIC_MAPPING genericMapping) const; 

        Common::SecurityDescriptorSPtr allowedClientsSecurityDescriptor_;
        Common::SecurityDescriptorSPtr adminClientsSecurityDescriptor_;
    };
}
