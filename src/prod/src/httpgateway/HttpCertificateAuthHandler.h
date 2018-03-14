// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpCertificateAuthHandler : public HttpAuthHandler
    {
    public:
        HttpCertificateAuthHandler(
            __in Transport::SecurityProvider::Enum handlerType,
            __in FabricClientWrapperSPtr &client)
            : HttpAuthHandler(handlerType)
        {
            UNREFERENCED_PARAMETER(client);
        }

        void OnCheckAccess(Common::AsyncOperationSPtr const& operation);

        Common::ErrorCode OnInitialize(Transport::SecuritySettings const&);
        Common::ErrorCode OnSetSecurity(Transport::SecuritySettings const&);
        Transport::SecuritySettingsSPtr Settings();

    private:

        void UpdateAuthenticationHeaderOnFailure(Common::AsyncOperationSPtr const& operation);
        void OnCertificateReadComplete(Common::AsyncOperationSPtr const& operation);
        void StartCertMonitorTimerIfNeeded_CallerHoldingWLock();
        void CertMonitorTimerCallback();

        Transport::SecuritySettingsSPtr securitySettings_;
        Common::TimerSPtr certMonitorTimer_;
        MUTABLE_RWLOCK(HttpCertificateAuthHandler, credentialLock_);
    };
}
