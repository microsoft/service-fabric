// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{ 
    class IHostingSubsystem;
    class HostingSubsystem;
    class ILegacyTestabilityRequestHandler;
    typedef std::unique_ptr<IHostingSubsystem> IHostingSubsystemUPtr;
    typedef std::weak_ptr<IHostingSubsystem> IHostingSubsystemWPtr;
    typedef std::shared_ptr<IHostingSubsystem> IHostingSubsystemSPtr;
    typedef std::unique_ptr<HostingSubsystem> HostingSubsystemUPtr;
    typedef Common::RootedObjectHolder<HostingSubsystem> HostingSubsystemHolder;
    typedef Common::RootedObjectWeakHolder<HostingSubsystem> HostingSubsystemWeakHolder;

    typedef std::shared_ptr<ILegacyTestabilityRequestHandler> LegacyTestabilityRequestHandlerSPtr;
    typedef std::weak_ptr<ILegacyTestabilityRequestHandler> LegacyTestabilityRequestHandlerWPtr;
    
    class FabricRuntimeManager;
    typedef std::unique_ptr<FabricRuntimeManager> FabricRuntimeManagerUPtr;

    class RuntimeRegistrationTable;
    typedef std::unique_ptr<RuntimeRegistrationTable> RuntimeRegistrationTableUPtr;

    class RuntimeRegistration;
    typedef std::shared_ptr<RuntimeRegistration> RuntimeRegistrationSPtr;

    class ServiceTypeStateManager;
    typedef std::unique_ptr<ServiceTypeStateManager> ServiceTypeStateManagerUPtr;

    class ServiceTypeRegistration;
    typedef std::shared_ptr<ServiceTypeRegistration> ServiceTypeRegistrationSPtr;

    class ApplicationManager;
    typedef std::unique_ptr<ApplicationManager> ApplicationManagerUPtr;

    class ApplicationMap;
    typedef std::unique_ptr<ApplicationMap> ApplicationMapUPtr;

    class Application2;
    typedef std::shared_ptr<Application2> Application2SPtr;
    typedef Common::RootedObjectHolder<Application2> Application2Holder;

    class VersionedApplication;
    typedef std::shared_ptr<VersionedApplication> VersionedApplicationSPtr;

    class ServicePackage2Map;
    typedef std::unique_ptr<ServicePackage2Map> ServicePackage2MapUPtr;

    class ServicePackage2;
    typedef std::shared_ptr<ServicePackage2> ServicePackage2SPtr;
    typedef Common::RootedObjectHolder<ServicePackage2> ServicePackage2Holder;

    class VersionedServicePackage;
    typedef std::shared_ptr<VersionedServicePackage> VersionedServicePackageSPtr;
    typedef Common::RootedObjectHolder<VersionedServicePackage> VersionedServicePackageHolder;

    class CodePackage;
    typedef std::shared_ptr<CodePackage> CodePackageSPtr;
    typedef Common::RootedObjectHolder<CodePackage> CodePackageHolder;
    typedef std::map<std::wstring, CodePackageSPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> CodePackageMap;

    class CodePackageInstance;
    typedef std::shared_ptr<CodePackageInstance> CodePackageInstanceSPtr;
    typedef std::unique_ptr<CodePackageInstance> CodePackageInstanceUPtr;

    class EnvironmentManager;
    typedef std::unique_ptr<EnvironmentManager> EnvironmentManagerUPtr;

    class ApplicationEnvironmentContext;
    typedef std::shared_ptr<ApplicationEnvironmentContext> ApplicationEnvironmentContextSPtr;

    class ServicePackageInstanceEnvironmentContext;
    typedef std::shared_ptr<ServicePackageInstanceEnvironmentContext> ServicePackageInstanceEnvironmentContextSPtr;

    class PrincipalsProvider;
    typedef std::unique_ptr<PrincipalsProvider> PrincipalsProviderUPtr;

    class EndpointProvider;
    typedef std::unique_ptr<EndpointProvider> EndpointProviderUPtr;

    class IPAddressProvider;
    typedef std::unique_ptr<IPAddressProvider> IPAddressProviderUPtr;

    class NatIPAddressProvider;
    typedef std::unique_ptr<NatIPAddressProvider> NatIPAddressProviderUPtr;

    class ILogCollectionProvider;
    typedef std::unique_ptr<ILogCollectionProvider> ILogCollectionProviderUPtr;

    class PrincipalsProviderContext;
    typedef std::unique_ptr<PrincipalsProviderContext> PrincipalsProviderContextUPtr;

    class ISecurityPrincipal;
    typedef std::shared_ptr<ISecurityPrincipal> ISecurityPrincipalSPtr;

    class SecurityUser;
    typedef std::shared_ptr<SecurityUser> SecurityUserSPtr;

    class SecurityGroup;
    typedef std::shared_ptr<SecurityGroup> SecurityGroupSPtr;

    class SecurityPrincipalsMap;
    typedef std::shared_ptr<SecurityPrincipalsMap> SecurityPrincipalsMapSPtr;

    class ApplicationPrincipals;
    typedef std::unique_ptr<ApplicationPrincipals> ApplicationPrincipalsUPtr;
    typedef std::shared_ptr<ApplicationPrincipals> ApplicationPrincipalsSPtr;

    class EndpointResource;
    typedef std::shared_ptr<EndpointResource> EndpointResourceSPtr;

    class EtwSessionProvider;
    typedef std::unique_ptr<EtwSessionProvider> EtwSessionProviderUPtr;

    class FirewallSecurityProvider;
    typedef std::unique_ptr<FirewallSecurityProvider> FirewallSecurityProviderUPtr;

    class CrashDumpProvider;
    typedef std::unique_ptr<CrashDumpProvider> CrashDumpProviderUPtr;

    class CrashDumpConfigurationManager;
    typedef std::unique_ptr<CrashDumpConfigurationManager> CrashDumpConfigurationManagerUPtr;

    class DownloadManager;
    typedef std::unique_ptr<DownloadManager> DownloadManagerUPtr;

    class ImageCacheManager;
    typedef std::unique_ptr<ImageCacheManager> ImageCacheManagerUPtr;

    class Activator;
    typedef std::unique_ptr<Activator> ActivatorUPtr;

    class Deactivator;
    typedef std::unique_ptr<Deactivator> DeactivatorUPtr;

    class ActivatorRequestor;
    typedef std::shared_ptr<ActivatorRequestor> ActivatorRequestorSPtr;
    typedef std::unique_ptr<ActivatorRequestor> ActivatorRequestorUPtr;

    class ApplicationHostManager;
    typedef std::unique_ptr<ApplicationHostManager> ApplicationHostManagerUPtr;

    class ApplicationHostRegistrationTable;
    typedef std::unique_ptr<ApplicationHostRegistrationTable> ApplicationHostRegistrationTableUPtr;

    class ApplicationHostRegistration;
    typedef std::shared_ptr<ApplicationHostRegistration> ApplicationHostRegistrationSPtr;

    class ApplicationHostActivationTable;
    typedef std::unique_ptr<ApplicationHostActivationTable> ApplicationHostActivationTableUPtr;

    class ProcessConsoleRedirector;
    typedef std::shared_ptr<ProcessConsoleRedirector> ProcessConsoleRedirectorSPtr;

    class ApplicationHostProxy;
    typedef std::shared_ptr<ApplicationHostProxy> ApplicationHostProxySPtr;

    class ProcessActivator;
    typedef std::unique_ptr<ProcessActivator> ProcessActivatorUPtr;

    class IProcessActivationContext;
    typedef std::shared_ptr<IProcessActivationContext> IProcessActivationContextSPtr;

    class ProcessActivationContext;
    typedef std::shared_ptr<ProcessActivationContext> ProcessActivationContextSPtr;

    class IContainerActivator;
    typedef std::unique_ptr<IContainerActivator> IContainerActivatorUPtr;

    class ApplicationHostCodePackageActivator;
    typedef std::unique_ptr<ApplicationHostCodePackageActivator> ApplicationHostCodePackageActivatorUPtr;
    typedef std::shared_ptr<ApplicationHostCodePackageActivator> ApplicationHostCodePackageActivatorSPtr;

    class ContainerNetworkOperations;
    typedef std::unique_ptr<ContainerNetworkOperations> ContainerNetworkOperationUPtr;

    class FabricRestartManager;
    typedef std::unique_ptr<FabricRestartManager> FabricRestartManagerUPtr;

    class NodeRestartManagerClient;
    typedef std::shared_ptr<NodeRestartManagerClient> NodeRestartManagerClientSPtr;

    class FabricRestartManager;
    typedef std::shared_ptr<FabricRestartManager> FabricRestartManagerSPtr;

    class ProcessDebugParameters;

    class ProcessDescription;
    typedef std::unique_ptr<ProcessDescription> ProcessDescriptionUPtr;

    class CodePackageActivationId;
    typedef std::unique_ptr<CodePackageActivationId> CodePackageActivationIdUPtr;

    class CodePackageRuntimeInformation;
    typedef std::shared_ptr<CodePackageRuntimeInformation> CodePackageRuntimeInformationSPtr;

    class OperationStatusMap;
    typedef std::shared_ptr<OperationStatusMap> OperationStatusMapSPtr;

    class PendingOperationMap;
    typedef std::unique_ptr<PendingOperationMap> PendingOperationMapUPtr;

    class ComCodePackageActivationContext;

    class EventDispatcher;
    typedef std::unique_ptr<EventDispatcher> EventDispatcherUPtr;

    typedef Common::JobQueue<std::unique_ptr<Common::JobItem<HostingSubsystem>>, HostingSubsystem> HostingJobQueue;
    typedef std::unique_ptr<HostingJobQueue> HostingJobQueueUPtr;

    class IFabricUpgradeImpl;
    typedef std::shared_ptr<IFabricUpgradeImpl> IFabricUpgradeImplSPtr;

    class FabricUpgradeManager;
    typedef std::unique_ptr<FabricUpgradeManager> FabricUpgradeManagerUPtr;

    class HostedServiceActivationManager;
    typedef std::shared_ptr<HostedServiceActivationManager> HostedServiceActivationManagerSPtr;
    typedef Common::RootedObjectHolder<HostedServiceActivationManager> HostedServiceActivationManagerHolder;

    class NetworkInventoryAgent;
    typedef std::unique_ptr<NetworkInventoryAgent> NetworkInventoryAgentUPtr;
    typedef std::shared_ptr<NetworkInventoryAgent> NetworkInventoryAgentSPtr;

    class TraceSessionManager;
    typedef std::unique_ptr<TraceSessionManager> TraceSessionManagerUPtr;

    typedef std::function<void(DWORD exitCode)> ServiceTerminationCallback;
    typedef std::function < Common::ErrorCode(std::wstring sddl, UINT port, bool isHttps, std::vector<std::wstring> prefixes)> HttpPortAclCallback;
    typedef std::function < Common::ErrorCode(
        UINT port,
        std::wstring const & x509FindValue,
        std::wstring const & x509StoreName,
        Common::X509FindType::Enum x509FindType,
        std::wstring const & principalSid)> PortCertificateBindingCallback;

    typedef std::function<void(std::wstring, std::wstring, DWORD)> ContainerTerminationCallback;
    typedef std::function<void(DWORD, ULONG, Common::TimeSpan)> ContainerEngineTerminationCallback;

    typedef std::function<void(DWORD, std::wstring , Common::ErrorCode, uint64)> FabricHostClientProcessTerminationCallback;

    class ContainerHealthStatusInfo;
    typedef std::function<void(std::wstring, std::vector<ContainerHealthStatusInfo>)> ContainerHealthCheckStatusCallback;

    class HostedService;
    typedef Common::RootedObjectHolder<HostedService> HostedServiceHolder;
    typedef std::shared_ptr<HostedService> HostedServiceSPtr;

    class HostedServiceMap;
    typedef std::shared_ptr<HostedServiceMap> HostedServiceMapSPtr;

    class FabricHostServiceListConfig;
    typedef std::shared_ptr<FabricHostServiceListConfig> FabricHostServiceListConfigSPtr;

    class FabricActivationManager;
    typedef std::shared_ptr<FabricActivationManager> FabricActivationManagerSPtr;

    class HostedServiceInstance;
    typedef std::shared_ptr<HostedServiceInstance> HostedServiceInstanceSPtr;

    class ProcessActivationManager;
    typedef Common::RootedObjectHolder<ProcessActivationManager> ProcessActivationManagerHolder;

    class ApplicationService;
    typedef std::shared_ptr<ApplicationService> ApplicationServiceSPtr;

    class ApplicationServiceMap;
    typedef std::shared_ptr<ApplicationServiceMap> ApplicationServiceMapSPtr;

    typedef std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> CaseInsensitiveStringSet;

    class HostingQueryManager;
    typedef std::unique_ptr<HostingQueryManager> HostingQueryManagerUPtr;

    class IFabricActivatorClient;
    typedef std::shared_ptr<IFabricActivatorClient> IFabricActivatorClientSPtr;

    class ISecretStoreClientPtr;

    class ActivateProcessRequest;
    class ConfigureSecurityPrincipalRequest;

    class ProcessActivationManager;
    typedef std::shared_ptr<ProcessActivationManager> ProcessActivationManagerSPtr;

    class NodeEnvironmentManager;
    typedef std::shared_ptr<NodeEnvironmentManager> NodeEnvironmentManagerSPtr;

    class SecurityPrincipalsProvider;
    typedef std::unique_ptr<SecurityPrincipalsProvider> SecurityPrincipalsProviderUPtr;

    class HttpEndpointSecurityProvider;
    typedef std::unique_ptr<HttpEndpointSecurityProvider> HttpEndpointSecurityProviderUPtr;

    class SecurityPrincipalInformation;
    typedef std::unique_ptr<SecurityPrincipalInformation> SecurityPrincipalInformationUPtr;
    typedef std::shared_ptr<SecurityPrincipalInformation> SecurityPrincipalInformationSPtr;

    class HostingHealthManager;
    typedef std::unique_ptr<HostingHealthManager> HostingHealthManagerUPtr;

    class FabricNodeContext;
    typedef std::shared_ptr<FabricNodeContext> FabricNodeContextSPtr;

    class CertificateAccessDescription;
    class ContainerImageDescription;
    class EndpointCertificateBinding;

    class DeletionManager;
    typedef std::shared_ptr<DeletionManager> DeletionManagerUPtr;

    class PortAclRef;
    typedef std::shared_ptr<PortAclRef> PortAclRefSPtr;

    class PortCertRef;
    typedef std::shared_ptr<PortCertRef> PortCertRefSPtr;

    class PortAclMap;
    typedef std::unique_ptr<PortAclMap> PortAclMapUPtr;

    class ContainerHostConfig;
    typedef std::shared_ptr<ContainerHostConfig> ContainerHostConfigSPtr;

    class ContainerConfig;
    typedef std::shared_ptr<ContainerConfig> ContainerConfigSPtr;

    class CreateContainerConfig;
   
    class DockerClient;
    typedef std::shared_ptr<DockerClient> DockerClientSPtr;

    class ContainerImageDownloader;
    typedef std::unique_ptr<ContainerImageDownloader> ContainerImageDownloaderUPtr;

    class ContainerDescription;
    typedef std::unique_ptr<ContainerDescription> ContainerDescriptionUPtr;

    class DockerProcessManager;
    typedef std::unique_ptr<DockerProcessManager> DockerProcessManagerUPtr;

    class LocalResourceManager;
    typedef std::unique_ptr<LocalResourceManager> LocalResourceManagerUPtr;

    class LocalSecretServiceManager;
    typedef std::unique_ptr<LocalSecretServiceManager> LocalSecretServiceManagerUPtr;

    class IIPAM;
    typedef std::shared_ptr<IIPAM> IIPAMSPtr;

    class INatIPAM;
    typedef std::shared_ptr<INatIPAM> INatIPAMSPtr;

    class GuestServiceTypeHost;
    typedef std::shared_ptr<GuestServiceTypeHost> GuestServiceTypeHostSPtr;

    class GuestServiceTypeHostManager;
    typedef std::unique_ptr<GuestServiceTypeHostManager> GuestServiceTypeHostManagerUPtr;

    class NetworkAllocationEntry;
    typedef std::shared_ptr<NetworkAllocationEntry> NetworkAllocationEntrySPtr;

    class DnsServiceEnvironmentManager;
    typedef std::unique_ptr<DnsServiceEnvironmentManager> DnsServiceEnvironmentManagerUPtr;

    class CertificateAclingManager;
    typedef std::shared_ptr<CertificateAclingManager> CertificateAclingManagerSPtr;
    typedef std::unique_ptr<CertificateAclingManager> CertificateAclingManagerUPtr;

    class ContainerActivatorServiceAgent;
    typedef std::shared_ptr<ContainerActivatorServiceAgent> ContainerActivatorServiceAgentSPtr;

    typedef std::function<void(Common::DateTime)> GhostChangeCallback;
    typedef std::function<void()> InternalReplenishNetworkResourcesCallback;
    typedef std::function<void(const std::wstring &)> ReplenishNetworkResourcesCallback;
    typedef std::function<void()> NetworkPluginProcessRestartedCallback;

    class OverlayNetworkManager;
    typedef std::unique_ptr<OverlayNetworkManager> OverlayNetworkManagerUPtr;

    class OverlayNetworkDriver;
    typedef std::shared_ptr<OverlayNetworkDriver> OverlayNetworkDriverSPtr;

    class OverlayNetworkPlugin;
    typedef std::shared_ptr<OverlayNetworkPlugin> OverlayNetworkPluginSPtr;

    class OverlayNetworkDefinition;
    typedef std::shared_ptr<OverlayNetworkDefinition> OverlayNetworkDefinitionSPtr;

    class OverlayNetworkRoutingInformation;
    typedef std::shared_ptr<OverlayNetworkRoutingInformation> OverlayNetworkRoutingInformationSPtr;

    class OverlayNetworkContainerParameters;
    typedef std::shared_ptr<OverlayNetworkContainerParameters> OverlayNetworkContainerParametersSPtr;

    class OverlayNetworkResource;
    typedef std::shared_ptr<OverlayNetworkResource> OverlayNetworkResourceSPtr;

    class IOverlayIPAM;
    typedef std::shared_ptr<IOverlayIPAM> IOverlayIPAMSPtr;

    class INatIPAM;
    typedef std::shared_ptr<INatIPAM> INatIPAMSPtr;

    class OverlayNetworkResourceProvider;
    typedef std::shared_ptr<OverlayNetworkResourceProvider> OverlayNetworkResourceProviderSPtr;

    class OverlayNetworkRoute;
    typedef std::shared_ptr<OverlayNetworkRoute> OverlayNetworkRouteSPtr;

    class OverlayNetworkPluginOperations;
    typedef std::unique_ptr<OverlayNetworkPluginOperations> OverlayNetworkPluginOperationsUPtr;

    class INetworkPluginProcessManager;
    typedef std::shared_ptr<INetworkPluginProcessManager> INetworkPluginProcessManagerSPtr;
}