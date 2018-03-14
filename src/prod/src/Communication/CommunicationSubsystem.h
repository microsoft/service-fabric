// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    class CommunicationSubsystem;
    typedef std::unique_ptr<CommunicationSubsystem> CommunicationSubsystemUPtr;

    class CommunicationSubsystem : public Common::AsyncFabricComponent
    {
    public:
        CommunicationSubsystem(
            __in Reliability::IReliabilitySubsystem &,
            __in Hosting2::IHostingSubsystem &,
            __in Hosting2::IRuntimeSharingHelper &,
            __in Transport::IpcServer &,
            std::wstring const & listenAddress,
            std::wstring const & replicatorAddress,
            Transport::SecuritySettings const& clusterSecuritySettings,
            std::wstring const & workingDir,
            Common::FabricNodeConfigSPtr const& nodeConfig,
            Common::ComponentRoot const &);

        ~CommunicationSubsystem();

        __declspec (property(get=get_NamingService)) Naming::NamingService & NamingService;
        Naming::NamingService & CommunicationSubsystem::get_NamingService() const;

        void EnableServiceRoutingAgent();

        Common::ErrorCode SetClusterSecurity(Transport::SecuritySettings const & securitySettings);
        Common::ErrorCode SetNamingGatewaySecurity(Transport::SecuritySettings const & securitySettings);

        Common::AsyncOperationSPtr BeginCreateNamingService(Common::TimeSpan, Common::AsyncCallback const &, Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateNamingService(Common::AsyncOperationSPtr const &);

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) override;

        void OnAbort();

    private:
        class Impl;
        std::unique_ptr<CommunicationSubsystem::Impl> implUPtr_;
    };
}
