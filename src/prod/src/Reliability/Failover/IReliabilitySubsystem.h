// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // The interface to Reliability
    class IReliabilitySubsystem : public Common::FabricComponent, 
        public Common::RootedObject
    {
        DENY_COPY(IReliabilitySubsystem);

    public:

        __declspec(property(get=get_NodeConfig)) Common::FabricNodeConfigSPtr const & NodeConfig;
        Common::FabricNodeConfigSPtr const & get_NodeConfig() { return nodeConfig_; }

        __declspec(property(get=get_WorkingDirectory)) std::wstring const & WorkingDirectory;
        std::wstring const & get_WorkingDirectory() const { return workingDirectory_; }

        __declspec (property(get=get_FederationWrapper)) Reliability::FederationWrapper & FederationWrapper;
        virtual Reliability::FederationWrapper & get_FederationWrapper() = 0;

        __declspec (property(get=get_FederationWrapperBase)) Reliability::IFederationWrapper & FederationWrapperBase;
        virtual Reliability::IFederationWrapper & get_FederationWrapperBase() = 0;

        __declspec (property(get=get_ServiceAdminClient)) ServiceAdminClient & AdminClient;
        virtual ServiceAdminClient & get_ServiceAdminClient() = 0;

        __declspec (property(get=get_ServiceResolver)) ServiceResolver & Resolver;
        virtual ServiceResolver & get_ServiceResolver() = 0;

        __declspec (property(get=get_FederationSubsystem)) Federation::FederationSubsystem & Federation;
        virtual Federation::FederationSubsystem & get_FederationSubsystem() const = 0;

        __declspec (property(get=get_HealthClient)) Client::HealthReportingComponentSPtr const & HealthClient;
        virtual Client::HealthReportingComponentSPtr const & get_HealthClient() const = 0;

        __declspec (property(get=get_ClientFactory)) Api::IClientFactoryPtr const & ClientFactory;
        virtual Api::IClientFactoryPtr const & get_ClientFactory() const = 0;

        __declspec (property(get=get_securitySettings)) Transport::SecuritySettings const& SecuritySettings;
        virtual Transport::SecuritySettings const& get_securitySettings() const = 0;

        virtual void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient) = 0;
        virtual void InitializeClientFactory(Api::IClientFactoryPtr const &) = 0;

        virtual Common::ErrorCode SetSecurity(Transport::SecuritySettings const& value) = 0;

        virtual Common::HHandler RegisterFailoverManagerReadyEvent(Common::EventHandler const & handler) = 0;
        virtual bool UnRegisterFailoverManagerReadyEvent(Common::HHandler hHandler) = 0;

        virtual Common::HHandler RegisterFailoverManagerFailedEvent(Common::EventHandler const & handler) = 0;
        virtual bool UnRegisterFailoverManagerFailedEvent(Common::HHandler hHandler) = 0;

        virtual NodeDescription GetNodeDescription() const = 0;

        virtual void ProcessNodeUpAck(Transport::MessageUPtr && nodeUpReply, bool isFromFMM) = 0;
        virtual void ProcessLFUMUploadReply(Reliability::GenerationHeader const & generationHeader) = 0;

        virtual Store::ILocalStore & Test_GetLocalStore() = 0;
        virtual Common::ComponentRootWPtr Test_GetFailoverManagerService() const = 0;

        virtual ~IReliabilitySubsystem()
        {
        }

    protected:
        IReliabilitySubsystem(Common::ComponentRoot const * root, Common::FabricNodeConfigSPtr const & nodeConfig, std::wstring const & workingDirectory)
        : RootedObject(*root),
          nodeConfig_(nodeConfig),
          workingDirectory_(workingDirectory)
        {
        }

        Common::FabricNodeConfigSPtr nodeConfig_;
        std::wstring workingDirectory_;
    };

    struct ReliabilitySubsystemConstructorParameters
    {
        Common::ProcessTerminationServiceSPtr ProcessTerminationService;
        Federation::FederationSubsystemSPtr Federation;
        Transport::IpcServer * IpcServer;
        Hosting2::IHostingSubsystem * HostingSubsystem;
        Hosting2::IRuntimeSharingHelper * RuntimeSharingHelper;
        Common::FabricNodeConfigSPtr NodeConfig;
        std::map<std::wstring, uint> NodeCapacityMap;
        Transport::SecuritySettings const * SecuritySettings;
        std::wstring WorkingDir;
        Common::ComponentRoot const * Root;
        Store::IStoreFactorySPtr StoreFactory;
        KtlLogger::KtlLoggerNodeSPtr KtlLoggerNode;
    };

    typedef IReliabilitySubsystemUPtr ReliabilitySubsystemFactoryFunctionPtr(ReliabilitySubsystemConstructorParameters & parameters);

    ReliabilitySubsystemFactoryFunctionPtr ReliabilitySubsystemFactory;
}
