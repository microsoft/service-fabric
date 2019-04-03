// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class TestHostingSubsystem : public Hosting2::IHostingSubsystem
    {
        DENY_COPY(TestHostingSubsystem);

    public:
        Hosting2::IFabricActivatorClientSPtr const & get_FabricActivatorClient() const;
        std::wstring const get_NodeName() const;

        __declspec(property(get = get_InnerHosting)) Hosting2::HostingSubsystem & InnerHosting;
        Hosting2::HostingSubsystem & get_InnerHosting() 
        { 
            return dynamic_cast<Hosting2::HostingSubsystem&>(*hosting_); 
        }

        Common::ErrorCode FindServiceTypeRegistration(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            Reliability::ServiceDescription const & serviceDescription,
			ServiceModel::ServicePackageActivationContext const & activationContext,
            __out uint64 & sequenceNumber,
            __out Hosting2::ServiceTypeRegistrationSPtr & registration);

        Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            __out std::wstring & hostId);

       Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out std::wstring & hostId);

        Common::AsyncOperationSPtr BeginDownloadApplication(
            Hosting2::ApplicationDownloadSpecification const & appDownloadSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDownloadApplication(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode AnalyzeApplicationUpgrade(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            __out AffectedRuntimesSet & affectedRuntimeIds);

        Common::AsyncOperationSPtr BeginUpgradeApplication(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndUpgradeApplication(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDownloadFabric(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDownloadFabric(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginValidateFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndValidateFabricUpgrade(
            __out bool & shouldRestartReplica,
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            bool const shouldRestartReplica,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndFabricUpgrade(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginTerminateServiceHost(
            std::wstring const & hostId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        void EndTerminateServiceHost(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode IncrementUsageCount(
			ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext);

        void DecrementUsageCount(
			ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext);

        void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient);

        void InitializePassThroughClientFactory(Api::IClientFactoryPtr const & healthClient);

        void InitializeLegacyTestabilityRequestHandler(Hosting2::LegacyTestabilityRequestHandlerSPtr const & legacyTestabilityRequestHandler);

        Hosting2::ServiceTypeRegisteredEventHHandler RegisterServiceTypeRegisteredEventHandler(Hosting2::ServiceTypeRegisteredEventHandler const & handler);
        bool UnregisterServiceTypeRegisteredEventHandler(Hosting2::ServiceTypeRegisteredEventHHandler const & hHandler);

        Hosting2::ServiceTypeDisabledEventHHandler RegisterServiceTypeDisabledEventHandler(Hosting2::ServiceTypeDisabledEventHandler const & handler);
        bool UnregisterServiceTypeDisabledEventHandler(Hosting2::ServiceTypeDisabledEventHHandler const & hHandler);

        Hosting2::ServiceTypeEnabledEventHHandler RegisterServiceTypeEnabledEventHandler(Hosting2::ServiceTypeEnabledEventHandler const & handler);
        bool UnregisterServiceTypeEnabledEventHandler(Hosting2::ServiceTypeEnabledEventHHandler const & hHandler);

        Hosting2::RuntimeClosedEventHHandler RegisterRuntimeClosedEventHandler(Hosting2::RuntimeClosedEventHandler const & handler);
        bool UnregisterRuntimeClosedEventHandler(Hosting2::RuntimeClosedEventHHandler const & hHandler);

        Hosting2::ApplicationHostClosedEventHHandler RegisterApplicationHostClosedEventHandler(Hosting2::ApplicationHostClosedEventHandler const & handler);
        bool UnregisterApplicationHostClosedEventHandler(Hosting2::ApplicationHostClosedEventHHandler const & hHandler);

        Hosting2::AvailableContainerImagesEventHHandler RegisterSendAvailableContainerImagesEventHandler(
            Hosting2::AvailableContainerImagesEventHandler const & handler);
        bool UnregisterSendAvailableContainerImagesEventHandler(Hosting2::AvailableContainerImagesEventHHandler const & hHandler);

        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        void OnAbort();

        Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring applicationNameArgument,
            std::wstring serviceManifestNameArgument,
            std::wstring codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring const & applicationNameArgument,
            std::wstring const & serviceManifestNameArgument,
            std::wstring const & servicePackageActivationId,
            std::wstring const & codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndRestartDeployedPackage(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr& reply) override;

        static Hosting2::IHostingSubsystemSPtr Create(Hosting2::HostingSubsystemConstructorParameters & parameters);

        uint64 GetResourceNodeCapacity(std::wstring const& resourceName) const;

        __declspec(property(get = get_NetworkInventoryAgent, put = set_NetworkInventoryAgent)) Hosting2::NetworkInventoryAgentSPtr NetworkInventoryAgent;
        Hosting2::NetworkInventoryAgentSPtr const get_NetworkInventoryAgent() const { return networkInventoryAgent_; }
        void set_NetworkInventoryAgent(Hosting2::NetworkInventoryAgentSPtr value) { networkInventoryAgent_ = value; }
                
    private:
        TestHostingSubsystem(Hosting2::HostingSubsystemConstructorParameters & parameters);

        Common::AsyncOperationSPtr WrapBeginWithFaultInjection(
            std::wstring const & name,
            ApiFaultHelper::ApiName api,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            std::function<Common::AsyncOperationSPtr()> beginFunc);

        Common::ErrorCode WrapEndWithFaultInjection(
            Common::AsyncOperationSPtr const & operation,
            std::function<Common::ErrorCode ()> endFunc);

        Common::ErrorCode WrapSyncApiWithFaultInjection(
            std::wstring const & name,
            ApiFaultHelper::ApiName api,
            std::function<Common::ErrorCode ()> syncFunc);

        Federation::NodeId const nodeId_;
        Hosting2::IHostingSubsystemSPtr hosting_;
        Hosting2::NetworkInventoryAgentSPtr networkInventoryAgent_;
    };
}
