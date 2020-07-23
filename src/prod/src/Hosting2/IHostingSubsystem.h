// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{   
    // This is the interface of the hosting subsystem to other components
    // This enables other components to provide a dummy implementation of hosting for CITs
    class IHostingSubsystem : public Common::AsyncFabricComponent
    {
        DENY_COPY(IHostingSubsystem);

    public:
        typedef std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> AffectedRuntimesSet;
        
        IHostingSubsystem()
        {
        }

        virtual ~IHostingSubsystem()
        {
        }

        __declspec(property(get = get_FabricActivatorClient)) IFabricActivatorClientSPtr const & FabricActivatorClientObj;
        virtual IFabricActivatorClientSPtr const & get_FabricActivatorClient() const = 0;

        __declspec(property(get = get_NodeName)) std::wstring const NodeName;
        virtual std::wstring const get_NodeName() const = 0;

        //
        // Hosting MUST only use properties of the service description that do not change
        // through the life of the service. As of now there is no mechanism to update the
        // service description.
        //
        virtual Common::ErrorCode FindServiceTypeRegistration(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            Reliability::ServiceDescription const & serviceDescription, 
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out uint64 & sequenceNumber,
            __out Hosting2::ServiceTypeRegistrationSPtr & registration) = 0;

        virtual Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            __out std::wstring & hostId) = 0;

        virtual Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out std::wstring & hostId) = 0;

        virtual Common::AsyncOperationSPtr BeginDownloadApplication(
            Hosting2::ApplicationDownloadSpecification const & appDownloadSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDownloadApplication(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::ErrorCode AnalyzeApplicationUpgrade(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            __out AffectedRuntimesSet & affectedRuntimeIds) = 0;

        virtual Common::AsyncOperationSPtr BeginUpgradeApplication(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;        

        virtual Common::ErrorCode EndUpgradeApplication(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDownloadFabric(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDownloadFabric(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginValidateFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndValidateFabricUpgrade(  
            __out bool & shouldRestartReplica,
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            bool const shouldRestartReplica,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;        

        virtual Common::ErrorCode EndFabricUpgrade(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring applicationNameArgument,
            std::wstring serviceManifestNameArgument,
            std::wstring codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring const & applicationNameArgument,
            std::wstring const & serviceManifestNameArgument,
            std::wstring const & servicePackageActivationId,
            std::wstring const & codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndRestartDeployedPackage(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr& reply) = 0;

        // The service host specified by this host id should be terminated
        // On termination the AppHostDown notification should be fired
        // if hostId is the NativeSystemServiceHOstId then fabric.exe should be terminated
        virtual Common::AsyncOperationSPtr BeginTerminateServiceHost(
            std::wstring const & hostId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual void EndTerminateServiceHost(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::ErrorCode IncrementUsageCount(
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext) = 0;

        virtual void DecrementUsageCount(
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext) = 0;

        virtual void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient) = 0;
        virtual void InitializePassThroughClientFactory(Api::IClientFactoryPtr const & healthClient) = 0;
        virtual void InitializeLegacyTestabilityRequestHandler(LegacyTestabilityRequestHandlerSPtr const & legacyTestabilityRequestHandler) = 0;

        virtual ServiceTypeRegisteredEventHHandler RegisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHandler const & handler) = 0;
        virtual bool UnregisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHHandler const & hHandler) = 0;

        virtual ServiceTypeDisabledEventHHandler RegisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHandler const & handler) = 0;
        virtual bool UnregisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHHandler const & hHandler) = 0;

        virtual ServiceTypeEnabledEventHHandler RegisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHandler const & handler) = 0;
        virtual bool UnregisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHHandler const & hHandler) = 0;

        virtual RuntimeClosedEventHHandler RegisterRuntimeClosedEventHandler(RuntimeClosedEventHandler const & handler) = 0;
        virtual bool UnregisterRuntimeClosedEventHandler(RuntimeClosedEventHHandler const & hHandler) = 0;

        virtual ApplicationHostClosedEventHHandler RegisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHandler const & handler) = 0;
        virtual bool UnregisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHHandler const & hHandler) = 0;

        virtual uint64 GetResourceNodeCapacity(std::wstring const& resourceName) const = 0;

        virtual AvailableContainerImagesEventHHandler RegisterSendAvailableContainerImagesEventHandler(AvailableContainerImagesEventHandler const & handler) = 0;
        virtual bool UnregisterSendAvailableContainerImagesEventHandler(AvailableContainerImagesEventHHandler const & hHandler) = 0;

        __declspec(property(get = get_NetworkInventoryAgent, put = set_NetworkInventoryAgent)) NetworkInventoryAgentSPtr NetworkInventoryAgent;
        virtual NetworkInventoryAgentSPtr const get_NetworkInventoryAgent() const = 0;
        virtual void set_NetworkInventoryAgent(NetworkInventoryAgentSPtr value) = 0;
    };

    struct HostingSubsystemConstructorParameters
    {
        Federation::FederationSubsystemSPtr Federation;
        Transport::IpcServer * IpcServer;
        std::wstring NodeWorkingDirectory;
        Common::FabricNodeConfigSPtr NodeConfig;
        Common::ComponentRoot const * Root;
        bool ServiceTypeBlocklistingEnabled;
        std::wstring DeploymentFolder;
        KtlLogger::KtlLoggerNodeSPtr KtlLoggerNode;
    };
    
    typedef IHostingSubsystemSPtr HostingSubsystemFactoryFunctionPtr(HostingSubsystemConstructorParameters & parameters);

    HostingSubsystemFactoryFunctionPtr HostingSubsystemFactory;
}