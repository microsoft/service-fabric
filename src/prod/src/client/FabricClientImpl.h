// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Management/FaultAnalysisService/FaultAnalysisServiceConfig.h"
namespace Client
{
    //
    // This class implements the fabric client.
    //

    class FabricClientImpl : public Common::TextTraceComponent<Common::TraceTaskCodes::Client>,
        public Common::ComponentRoot,
        public Common::FabricComponent,
        public Api::IQueryClient,
        public Api::IClientSettings,
        public Api::IServiceManagementClient,
        public Api::IApplicationManagementClient,
        public Api::IClusterManagementClient,
        public Api::IRepairManagementClient,
        public Api::IHealthClient,
        public Api::IPropertyManagementClient,
        public Api::IServiceGroupManagementClient,
        public Api::IInternalInfrastructureServiceClient,
        public Api::IImageStoreClient,
        public Api::IInfrastructureServiceClient,
        public Api::IFileStoreServiceClient,
        public Api::IInternalFileStoreServiceClient,
        public Api::ITestClient,
        public Api::IInternalTokenValidationServiceClient,
        public Api::IAccessControlClient,
        public Api::ITestManagementClient,
        public Api::ITestManagementClientInternal,
        public Api::IFaultManagementClient,
        public Api::IComposeManagementClient,
        public Api::ISecretStoreClient,
        public Api::IResourceManagerClient,
        public Api::IResourceManagementClient,
        public Api::INetworkManagementClient,
        public Api::IGatewayResourceManagerClient
        {
        DENY_COPY(FabricClientImpl);

    public:
        FabricClientImpl(
            std::vector<std::wstring> && gatewayAddresses,
            Api::IServiceNotificationEventHandlerPtr const & = Api::IServiceNotificationEventHandlerPtr(),
            Api::IClientConnectionEventHandlerPtr const & = Api::IClientConnectionEventHandlerPtr());

        FabricClientImpl(
            Common::FabricNodeConfigSPtr const &,
            Transport::RoleMask::Enum role = Transport::RoleMask::Enum::None,
            Api::IServiceNotificationEventHandlerPtr const & = Api::IServiceNotificationEventHandlerPtr(),
            Api::IClientConnectionEventHandlerPtr const & = Api::IClientConnectionEventHandlerPtr());

        FabricClientImpl(
            Naming::INamingMessageProcessorSPtr const &namingMessageProcessorSPtr,
            Api::IServiceNotificationEventHandlerPtr const & = Api::IServiceNotificationEventHandlerPtr(),
            Api::IClientConnectionEventHandlerPtr const & = Api::IClientConnectionEventHandlerPtr());

        // This constructor is used by Http gateway where it has already authenticated
        // the incoming connection and wants to communicate with TCP gateway without
        // requiring the client to be authenticated again.
        FabricClientImpl(
            Common::FabricNodeConfigSPtr const &,
            Transport::RoleMask::Enum role,
            bool isClientRoleAuthorized,
            Api::IServiceNotificationEventHandlerPtr const & = Api::IServiceNotificationEventHandlerPtr(),
            Api::IClientConnectionEventHandlerPtr const & = Api::IClientConnectionEventHandlerPtr());

        ~FabricClientImpl();

        __declspec(property(get=get_Settings)) FabricClientInternalSettingsSPtr const & Settings;
        FabricClientInternalSettingsSPtr const & get_Settings() const { Common::AcquireReadLock lock(stateLock_); return GetSettingsCallerHoldsLock(); }

        FabricClientInternalSettingsSPtr const & GetSettingsCallerHoldsLock() const { return settings_; }

        __declspec(property(get=get_TraceContext)) std::wstring const & TraceContext;
        std::wstring const & get_TraceContext() { return traceContext_; }

        __declspec(property(get=get_Connection)) ClientConnectionManagerSPtr const & Connection;
        ClientConnectionManagerSPtr const & get_Connection() { return clientConnectionManager_; }

        __declspec(property(get=get_Cache)) IClientCache & Cache;
        IClientCache & get_Cache();

        __declspec(property(get = get_IsClientRoleAuthorized)) bool IsClientRoleAuthorized;
        bool get_IsClientRoleAuthorized() { return isClientRoleAuthorized_; }

        static void UpdateSecurityFromConfig(std::weak_ptr<ComponentRoot> const &);

        Common::ErrorCode SetSecurityFromConfig();

        //
        // IClientSettings methods
        //
        Common::ErrorCode SetSecurity(Transport::SecuritySettings && securitySettings);

        Common::ErrorCode SetKeepAlive(ULONG keepAliveIntervalInSeconds);

        ServiceModel::FabricClientSettings GetSettings();

        Common::ErrorCode SetSettings(ServiceModel::FabricClientSettings && newSettings);

        Common::AsyncOperationSPtr BeginStopNode(
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            bool restart,
            bool createFabricDump,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndStopNode(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginAddUnreliableTransportBehavior(
            std::wstring const & nodeName,
            std::wstring const & behaviorString,
            std::wstring const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndAddUnreliableTransportBehavior(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginRemoveUnreliableTransportBehavior(
            std::wstring const & nodeName,
            std::wstring const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndRemoveUnreliableTransportBehavior(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetTransportBehaviorList(
            std::wstring const & nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetTransportBehaviorList(
            Common::AsyncOperationSPtr const &,
            std::vector<std::wstring>&);

        Common::AsyncOperationSPtr BeginStartNode(
            std::wstring const & nodeName,
            uint64 instanceId,
            std::wstring const & ipAddressOrFQDN,
            ULONG clusterConnectionPort,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndStartNode(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginRestartDeployedCodePackage(
            std::wstring const & nodeName,
            Common::NamingUri const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & codePackageName,
            std::wstring const & instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndRestartDeployedCodePackage(Common::AsyncOperationSPtr const &);

        //
        // IServiceMangementClient methods
        //
        Common::AsyncOperationSPtr BeginCreateService(
            ServiceModel::PartitionedServiceDescWrapper const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndCreateService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginInternalCreateService(
            Naming::PartitionedServiceDescriptor const &,
            ServiceModel::ServicePackageVersion const&,
            ULONGLONG packageInstance,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndInternalCreateService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCreateServiceFromTemplate(
            ServiceModel::ServiceFromTemplateDescription const & serviceFromTemplateDesc,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndCreateServiceFromTemplate(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginUpdateService(
            Common::NamingUri const &,
            Naming::ServiceUpdateDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndUpdateService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginDeleteService(
            ServiceModel::DeleteServiceDescription const &description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndDeleteService(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginInternalDeleteService(
            ServiceModel::DeleteServiceDescription const &description,
            Common::ActivityId const &activityId_,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndInternalDeleteService(
            Common::AsyncOperationSPtr const &operation,
            __inout bool & nameDeleted);

        Common::AsyncOperationSPtr BeginInternalDeleteSystemService(
            Common::NamingUri const& serviceName,
            Common::ActivityId const &activityId_,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndInternalDeleteSystemService(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginGetServiceDescription(
            Common::NamingUri const &name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetServiceDescription(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::PartitionedServiceDescriptor &description);

        Common::AsyncOperationSPtr BeginGetCachedServiceDescription(
            Common::NamingUri const &name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetCachedServiceDescription(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::PartitionedServiceDescriptor &description);

        Common::AsyncOperationSPtr BeginResolveServicePartition(
            Common::NamingUri const &serviceName,
            Naming::ServiceResolutionRequestData const &resolveRequest,
            Api::IResolvedServicePartitionResultPtr &previousResult,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::AsyncOperationSPtr BeginResolveServicePartition(
            Common::NamingUri const & serviceName,
            Naming::PartitionKey const &key,
            Naming::ResolvedServicePartitionMetadataSPtr const &metadata,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IResolvedServicePartitionResultPtr &result);
        Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::ResolvedServicePartitionSPtr &result);

        Common::AsyncOperationSPtr BeginResolveSystemServicePartition(
            Common::NamingUri const & serviceName,
            Naming::PartitionKey const &key,
            Common::Guid const & activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndResolveSystemServicePartition(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::ResolvedServicePartitionSPtr &result);

        Common::AsyncOperationSPtr BeginPrefixResolveServicePartition(
            Common::NamingUri const & serviceName,
            Naming::ServiceResolutionRequestData const & resolveRequest,
            Api::IResolvedServicePartitionResultPtr & previousResult,
            bool bypassCache,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndPrefixResolveServicePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::ResolvedServicePartitionSPtr & result);

        Common::ErrorCode RegisterServicePartitionResolutionChangeHandler(
            Common::NamingUri const &name,
            Naming::PartitionKey const &key,
            Api::ServicePartitionResolutionChangeCallback const &handler,
            __inout LONGLONG * callbackHandle);

        Common::ErrorCode UnRegisterServicePartitionResolutionChangeHandler(
            LONGLONG callbackHandle);

        Common::AsyncOperationSPtr BeginGetServiceManifest(
            std::wstring const& applicationTypeName,
            std::wstring const& applicationTypeVersion,
            std::wstring const& serviceManifestName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetServiceManifest(
            Common::AsyncOperationSPtr const &,
            __inout std::wstring &serviceManifest);

        Common::AsyncOperationSPtr BeginReportFault(
            std::wstring const & nodeName,
            Common::Guid const & partitionId,
            int64 replicaId,
            Reliability::FaultType::Enum faultType,
            bool isForce,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndReportFault(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginRegisterServiceNotificationFilter(
            Reliability::ServiceNotificationFilterSPtr const & filter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRegisterServiceNotificationFilter(
            Common::AsyncOperationSPtr const & operation,
            __out uint64 & filterId);

        Common::AsyncOperationSPtr BeginUnregisterServiceNotificationFilter(
            uint64 filterId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUnregisterServiceNotificationFilter(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginMovePrimary(
            std::wstring const & nodeName,
            Common::Guid const & partitionId,
            bool ignoreConstraints,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndMovePrimary(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginMoveSecondary(
            std::wstring const & currentNodeName,
            std::wstring const & newNodeName,
            Common::Guid const & partitionId,
            bool ignoreConstraints,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndMoveSecondary(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginPrefixResolveServicePartitionInternal(
            Common::NamingUri const & serviceName,
            Naming::ServiceResolutionRequestData const & resolveRequest,
            Api::IResolvedServicePartitionResultPtr & previousResult,
            bool bypassCache,
            Common::Guid const & activityId,
            bool suppressSuccessLogs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndPrefixResolveServicePartitionInternal(
            Common::AsyncOperationSPtr const & operation,
            bool suppressSuccessLogs,
            __inout Naming::ResolvedServicePartitionSPtr & result);

        //
        // IApplicationManagementClient methods.
        //
        Common::AsyncOperationSPtr BeginProvisionApplicationType(
            Management::ClusterManager::ProvisionApplicationTypeDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndProvisionApplicationType(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCreateApplication(
            ServiceModel::ApplicationDescriptionWrapper const &description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndCreateApplication(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginUpdateApplication(
            ServiceModel::ApplicationUpdateDescriptionWrapper const &updateDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback callback,
            Common::AsyncOperationSPtr const&);

        Common::ErrorCode EndUpdateApplication(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginUpgradeApplication(
            ServiceModel::ApplicationUpgradeDescriptionWrapper const &upgradeDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndUpgradeApplication(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetApplicationUpgradeProgress(
            Common::NamingUri const &applicationName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetApplicationUpgradeProgress(
            Common::AsyncOperationSPtr const &,
            __inout Api::IApplicationUpgradeProgressResultPtr &upgradeDescription);

        Common::AsyncOperationSPtr BeginMoveNextApplicationUpgradeDomain(
            Api::IApplicationUpgradeProgressResultPtr const& progress,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndMoveNextApplicationUpgradeDomain(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginMoveNextApplicationUpgradeDomain2(
            Common::NamingUri const &applicationName,
            std::wstring const &nextUpgradeDomain,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndMoveNextApplicationUpgradeDomain2(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginDeleteApplication(
            ServiceModel::DeleteApplicationDescription const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndDeleteApplication(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginUnprovisionApplicationType(
            Management::ClusterManager::UnprovisionApplicationTypeDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndUnprovisionApplicationType(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginStartInfrastructureTask(
            Management::ClusterManager::InfrastructureTaskDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndStartInfrastructureTask(
            Common::AsyncOperationSPtr const &,
            __out bool & isComplete);

        Common::AsyncOperationSPtr BeginFinishInfrastructureTask(
            Management::ClusterManager::TaskInstance const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndFinishInfrastructureTask(
            Common::AsyncOperationSPtr const &,
            __out bool & isComplete);

        Common::AsyncOperationSPtr BeginGetApplicationManifest(
            std::wstring const& applicationTypeName,
            std::wstring const& applicationTypeVersion,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetApplicationManifest(
            Common::AsyncOperationSPtr const &,
            __inout std::wstring &clusterManifest);

        Common::AsyncOperationSPtr BeginRollbackApplicationUpgrade(
            Common::NamingUri const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndRollbackApplicationUpgrade(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginUpdateApplicationUpgrade(
            Management::ClusterManager::ApplicationUpgradeUpdateDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndUpdateApplicationUpgrade(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginDeployServicePackageToNode(
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion,
            std::wstring const & serviceManifestName,
            std::wstring const & sharingPolicies,
            std::wstring const & nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndDeployServicePackageToNode(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetContainerInfo(
            std::wstring const & nodeName,
            Common::NamingUri const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & codePackageName,
            std::wstring const & containerInfoType,
            ServiceModel::ContainerInfoArgMap & containerInfoArgMap,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const &,
            __inout wstring &containerInfo);

        Common::AsyncOperationSPtr BeginInvokeContainerApi(
            std::wstring const & nodeName,
            Common::NamingUri const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & codePackageName,
            FABRIC_INSTANCE_ID codePackageInstance,
            ServiceModel::ContainerInfoArgMap & containerInfoArgMap,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndInvokeContainerApi(
            Common::AsyncOperationSPtr const &,
            __inout wstring & result) override;

        //
        // IComposeManagementClient methods
        //

        Common::AsyncOperationSPtr BeginCreateComposeDeployment(
            std::wstring const & deploymentName,
            std::wstring const& composeFilePath,
            std::wstring const& overridesFilePath,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool const isPasswordEncrypted,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCreateComposeDeployment(
            std::wstring const & deploymentName,
            Common::ByteBuffer && composeFile,
            Common::ByteBuffer && overridesFile,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool const isPasswordEncrypted,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndCreateComposeDeployment(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginInnerCreateComposeDeployment(
            std::wstring const &deploymentName,
            Common::ByteBuffer && composeFile,
            Common::ByteBuffer && overridesFile,
            std::wstring const &repositoryUserName,
            std::wstring const &repositoryPassword,
            bool const isPasswordEncrypted,
            Common::ActivityId const &activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndInnerCreateComposeDeployment(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginDeleteComposeDeployment(
            std::wstring const & deploymentName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndDeleteComposeDeployment(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetComposeDeploymentStatusList(
            ServiceModel::ComposeDeploymentStatusQueryDescription const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetComposeDeploymentStatusList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::ComposeDeploymentStatusQueryResult> & list,
            __inout ServiceModel::PagingStatusUPtr & pagingStatus);


        Common::AsyncOperationSPtr BeginUpgradeComposeDeployment(
            ServiceModel::ComposeDeploymentUpgradeDescription const &description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndUpgradeComposeDeployment(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetComposeDeploymentUpgradeProgress(
            std::wstring const & deploymentName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetComposeDeploymentUpgradeProgress(
            Common::AsyncOperationSPtr const &,
            __inout ServiceModel::ComposeDeploymentUpgradeProgress &result);

        Common::AsyncOperationSPtr BeginRollbackComposeDeployment(
            std::wstring const & deploymentName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;

        Common::ErrorCode EndRollbackComposeDeployment(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginDeleteSingleInstanceDeployment(
            ServiceModel::DeleteSingleInstanceDeploymentDescription const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndDeleteSingleInstanceDeployment(
            Common::AsyncOperationSPtr const &) override;

        //
        // IResourceManagementClient methods
        //

        // Volume
        Common::AsyncOperationSPtr BeginCreateVolume(
            Management::ClusterManager::VolumeDescriptionSPtr const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) override;

        Common::ErrorCode EndCreateVolume(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginGetVolumeResourceList(
            ServiceModel::ModelV2::VolumeQueryDescription const & volumeQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndGetVolumeResourceList(
            Common::AsyncOperationSPtr const & operation,
            std::vector<ServiceModel::ModelV2::VolumeQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) override;

        Common::AsyncOperationSPtr BeginDeleteVolume(
            std::wstring const & volumeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;

        Common::ErrorCode EndDeleteVolume(
            Common::AsyncOperationSPtr const &) override;

        // Application
        Common::AsyncOperationSPtr BeginCreateOrUpdateApplicationResource(
            ServiceModel::ModelV2::ApplicationDescription && description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndCreateOrUpdateApplicationResource(
            Common::AsyncOperationSPtr const &,
            __out ServiceModel::ModelV2::ApplicationDescription & description) override;

        Common::AsyncOperationSPtr BeginGetApplicationResourceList(
            ServiceModel::ApplicationQueryDescription && applicationQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetApplicationResourceList(
            Common::AsyncOperationSPtr const &,
            std::vector<ServiceModel::ModelV2::ApplicationDescriptionQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) override;

        // Service
        Common::AsyncOperationSPtr BeginGetServiceResourceList(
            ServiceModel::ServiceQueryDescription const & serviceQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndGetServiceResourceList(
            Common::AsyncOperationSPtr const &,
            std::vector<ServiceModel::ModelV2::ContainerServiceQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus);

        // Replica
        Common::AsyncOperationSPtr BeginGetReplicaResourceList(
            ServiceModel::ReplicasResourceQueryDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetReplicaResourceList(
            Common::AsyncOperationSPtr const & operation,
            std::vector<ServiceModel::ReplicaResourceQueryResult> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) override;

        Common::AsyncOperationSPtr BeginGetContainerCodePackageLogs(
            Common::NamingUri const & applicationName,
            Common::NamingUri const & serviceName,
            std::wstring replicaName,
            std::wstring const & codePackageName,
            ServiceModel::ContainerInfoArgMap & containerInfoArgMap,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndGetContainerCodePackageLogs(
            Common::AsyncOperationSPtr const & operation,
            __inout wstring &containerInfo);

        //
        // INetworkManagementClient methods
        //
        Common::AsyncOperationSPtr BeginCreateNetwork(
            ServiceModel::ModelV2::NetworkResourceDescription const &description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndCreateNetwork(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginDeleteNetwork(
            ServiceModel::DeleteNetworkDescription const &deleteDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndDeleteNetwork(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetNetworkList(
            ServiceModel::NetworkQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetNetworkList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::ModelV2::NetworkResourceDescriptionQueryResult> &networkList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetNetworkApplicationList(
            ServiceModel::NetworkApplicationQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetNetworkApplicationList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::NetworkApplicationQueryResult> &networkApplicationList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetNetworkNodeList(
            ServiceModel::NetworkNodeQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetNetworkNodeList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::NetworkNodeQueryResult> &networkNodeList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetApplicationNetworkList(
            ServiceModel::ApplicationNetworkQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetApplicationNetworkList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::ApplicationNetworkQueryResult> &applicationNetworkList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetDeployedNetworkList(
            ServiceModel::DeployedNetworkQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetDeployedNetworkList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::DeployedNetworkQueryResult> &deployedNetworkList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetDeployedNetworkCodePackageList(
            ServiceModel::DeployedNetworkCodePackageQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetDeployedNetworkCodePackageList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::DeployedNetworkCodePackageQueryResult> &deployedNetworkCodePackageList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        //
        // IQueryClient methods
        //
        Common::AsyncOperationSPtr BeginInternalQuery(
            std::wstring const &queryName,
            ServiceModel::QueryArgumentMap const &queryArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndInternalQuery(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::QueryResult &queryResult);

        Common::AsyncOperationSPtr BeginGetNodeList(
            ServiceModel::NodeQueryDescription const & queryDescription,
            bool excludeStoppedNodeInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) override;

        Common::AsyncOperationSPtr BeginGetNodeList(
            std::wstring const &nodeNameFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) override;

        Common::AsyncOperationSPtr BeginGetNodeList(
            std::wstring const &nodeNameFilter,
            DWORD nodeStatusFilter,
            bool excludeStoppedNodeInfo,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent) override;

        Common::AsyncOperationSPtr BeginGetFMNodeList(
            std::wstring const &nodeNameFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetNodeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::NodeQueryResult> &nodeList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) override;

        Common::AsyncOperationSPtr BeginGetApplicationTypeList(
            std::wstring const &applicationtypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndGetApplicationTypeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ApplicationTypeQueryResult> &applicationtypeList) override;

        Common::AsyncOperationSPtr BeginGetApplicationTypePagedList(
            ServiceModel::ApplicationTypeQueryDescription const &applicationTypeQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndGetApplicationTypePagedList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ApplicationTypeQueryResult> &applicationtypeList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) override;

        Common::AsyncOperationSPtr BeginGetServiceTypeList(
            std::wstring const &applicationTypeName,
            std::wstring const &applicationTypeVersion,
            std::wstring const &serviceTypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndGetServiceTypeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceTypeQueryResult> &servicetypeList);

        Common::AsyncOperationSPtr BeginGetServiceGroupMemberTypeList(
            std::wstring const &applicationTypeName,
            std::wstring const &applicationTypeVersion,
            std::wstring const &serviceGroupTypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetServiceGroupMemberTypeList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceGroupMemberTypeQueryResult> &servicegroupmembertypeList);

        Common::AsyncOperationSPtr BeginGetApplicationList(
            ServiceModel::ApplicationQueryDescription const & applicationQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &) override;

        Common::ErrorCode EndGetApplicationList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::ApplicationQueryResult> &applicationList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus)  override;

        Common::AsyncOperationSPtr BeginGetServiceList(
            ServiceModel::ServiceQueryDescription const & serviceQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndGetServiceList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceQueryResult> &serviceList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus) override;

        Common::AsyncOperationSPtr BeginGetServiceGroupMemberList(
            Common::NamingUri const &applicationName,
            Common::NamingUri const &serviceNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetServiceGroupMemberList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceGroupMemberQueryResult> &serviceGroupMemberList);

        Common::AsyncOperationSPtr BeginGetPartitionList(
            Common::NamingUri const &serviceName,
            Common::Guid const &partitionIdFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetPartitionList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServicePartitionQueryResult> &partitionList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetReplicaList(
            Common::Guid const &partitionId,
            int64 replicaOrInstanceIdFilter,
            DWORD replicaStatusFilter,
            std::wstring const &continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetReplicaList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::ServiceReplicaQueryResult> &replicaList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetDeployedApplicationList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetDeployedApplicationList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::DeployedApplicationQueryResult> &deployedApplicationList);

        Common::AsyncOperationSPtr BeginGetDeployedApplicationPagedList(
            ServiceModel::DeployedApplicationQueryDescription const &queryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetDeployedApplicationPagedList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::DeployedApplicationQueryResult> &deployedApplicationList,
            __inout ServiceModel::PagingStatusUPtr &pagingStatus);

        Common::AsyncOperationSPtr BeginGetDeployedServicePackageList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetDeployedServicePackageList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedServiceManifestQueryResult> &deployedServiceManifestList);

        Common::AsyncOperationSPtr BeginGetDeployedServiceTypeList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            std::wstring const &serviceTypeNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetDeployedServiceTypeList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedServiceTypeQueryResult> &deployedServiceTypeList);

        Common::AsyncOperationSPtr BeginGetDeployedCodePackageList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            std::wstring const &codePackageNameFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetDeployedCodePackageList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedCodePackageQueryResult> &deployedCodePackageList);

        Common::AsyncOperationSPtr BeginGetDeployedReplicaList(
            std::wstring const &nodeName,
            Common::NamingUri const &applicationName,
            std::wstring const &serviceManifestNameFilter,
            Common::Guid const &partitionIdFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetDeployedReplicaList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::DeployedServiceReplicaQueryResult> &deployedReplicaList);

        Common::AsyncOperationSPtr BeginGetDeployedServiceReplicaDetail(
            std::wstring const &nodeName,
            Common::Guid const &partitionId,
            FABRIC_REPLICA_ID replicaId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetDeployedServiceReplicaDetail(
            Common::AsyncOperationSPtr const &operation,
            __out ServiceModel::DeployedServiceReplicaDetailQueryResult & queryResult);

        Common::AsyncOperationSPtr BeginGetClusterLoadInformation(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndGetClusterLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ClusterLoadInformationQueryResult &clusterLoadInformation);

        Common::AsyncOperationSPtr BeginGetPartitionLoadInformation(
            Common::Guid const &partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetPartitionLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::PartitionLoadInformationQueryResult &partitionLoadInformation);

        Common::AsyncOperationSPtr BeginGetProvisionedFabricCodeVersionList(
            std::wstring const & codeVersionFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetProvisionedFabricCodeVersionList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::ProvisionedFabricCodeVersionQueryResultItem> &codeVersionList);

        Common::AsyncOperationSPtr BeginGetProvisionedFabricConfigVersionList(
            std::wstring const & configVersionFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetProvisionedFabricConfigVersionList(
            Common::AsyncOperationSPtr const &operation,
            __out std::vector<ServiceModel::ProvisionedFabricConfigVersionQueryResultItem> &configVersionList);

        Common::AsyncOperationSPtr BeginGetNodeLoadInformation(
            std::wstring const &nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndGetNodeLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::NodeLoadInformationQueryResult &nodeLoadInformation);

        Common::AsyncOperationSPtr BeginGetReplicaLoadInformation(
            Common::Guid const &partitionId,
            int64 replicaOrInstanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetReplicaLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ReplicaLoadInformationQueryResult &replicaLoadInformation);

        Common::AsyncOperationSPtr BeginGetUnplacedReplicaInformation(
            std::wstring const &serviceName,
            Common::Guid const &partitionId,
            bool onlyQueryPrimaries,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetUnplacedReplicaInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::UnplacedReplicaInformationQueryResult &unplacedReplicaInformation);

        Common::AsyncOperationSPtr BeginGetApplicationLoadInformation(
            std::wstring const &applicationName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode EndGetApplicationLoadInformation(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ApplicationLoadInformationQueryResult &applicationLoadInformation);

        Common::AsyncOperationSPtr BeginGetServiceName(
            Common::Guid const &partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetServiceName(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ServiceNameQueryResult &serviceName);

        Common::AsyncOperationSPtr BeginGetApplicationName(
            Common::NamingUri const &serviceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetApplicationName(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ApplicationNameQueryResult &applicationName);

        //
        // IClusterManagementClient methods
        //

        Common::AsyncOperationSPtr BeginDeactivateNode(
            std::wstring const &nodeName,
            FABRIC_NODE_DEACTIVATION_INTENT const deactivationIntent,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndDeactivateNode(
            Common::AsyncOperationSPtr const &parent) override;

        Common::AsyncOperationSPtr BeginActivateNode(
            std::wstring const &nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndActivateNode(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginDeactivateNodesBatch(
            std::map<Federation::NodeId, Reliability::NodeDeactivationIntent::Enum> const &,
            std::wstring const & batchId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndDeactivateNodesBatch(
            Common::AsyncOperationSPtr const &parent) override;

        Common::AsyncOperationSPtr BeginRemoveNodeDeactivations(
            std::wstring const & batchId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndRemoveNodeDeactivations(
            Common::AsyncOperationSPtr const &parent) override;

        Common::AsyncOperationSPtr BeginGetNodeDeactivationStatus(
            std::wstring const & batchId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndGetNodeDeactivationStatus(
            Common::AsyncOperationSPtr const &parent,
            __out Reliability::NodeDeactivationStatus::Enum &) override;

        Common::AsyncOperationSPtr BeginNodeStateRemoved(
            std::wstring const &nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndNodeStateRemoved(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginRecoverPartitions(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndRecoverPartitions(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginRecoverPartition(
            Common::Guid partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndRecoverPartition(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginRecoverServicePartitions(
            std::wstring const& serviceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndRecoverServicePartitions(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginRecoverSystemPartitions(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndRecoverSystemPartitions(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginProvisionFabric(
            std::wstring const &codeFilepath,
            std::wstring const &clusterManifestFilepath,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndProvisionFabric(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginUpgradeFabric(
            ServiceModel::FabricUpgradeDescriptionWrapper const &upgradeDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndUpgradeFabric(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginRollbackFabricUpgrade(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndRollbackFabricUpgrade(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginUpdateFabricUpgrade(
            Management::ClusterManager::FabricUpgradeUpdateDescription const &,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndUpdateFabricUpgrade(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginGetFabricUpgradeProgress(
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndGetFabricUpgradeProgress(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IUpgradeProgressResultPtr &result) override;

        Common::AsyncOperationSPtr BeginMoveNextFabricUpgradeDomain(
            Api::IUpgradeProgressResultPtr const &progress,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndMoveNextFabricUpgradeDomain(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginMoveNextFabricUpgradeDomain2(
            std::wstring const &nextDomain,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndMoveNextFabricUpgradeDomain2(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginUnprovisionFabric(
            Common::FabricCodeVersion const &codeVersion,
            Common::FabricConfigVersion const &configVersion,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndUnprovisionFabric(
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginGetClusterManifest(
            Management::ClusterManager::ClusterManifestQueryDescription const &,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndGetClusterManifest(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) override;

        Common::AsyncOperationSPtr BeginGetClusterVersion(
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndGetClusterVersion(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) override;

        Common::AsyncOperationSPtr BeginToggleVerboseServicePlacementHealthReporting(
            bool enabled,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndToggleVerboseServicePlacementHealthReporting (
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginResetPartitionLoad(
            Common::Guid partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndResetPartitionLoad (
            Common::AsyncOperationSPtr const &operation) override;

        Common::AsyncOperationSPtr BeginUpgradeConfiguration(
            Management::UpgradeOrchestrationService::StartUpgradeDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndUpgradeConfiguration(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginGetClusterConfigurationUpgradeStatus(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetClusterConfigurationUpgradeStatus(
            Common::AsyncOperationSPtr const &,
            __inout Api::IFabricOrchestrationUpgradeStatusResultPtr &) override;

        Common::AsyncOperationSPtr BeginGetUpgradesPendingApproval(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetUpgradesPendingApproval(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginStartApprovedUpgrades(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndStartApprovedUpgrades(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginGetClusterConfiguration(
            std::wstring const & apiVersion,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;
        Common::ErrorCode EndGetClusterConfiguration(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) override;

        Common::AsyncOperationSPtr BeginGetUpgradeOrchestrationServiceState(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetUpgradeOrchestrationServiceState(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) override;

        Common::AsyncOperationSPtr BeginSetUpgradeOrchestrationServiceState(
            std::wstring const & state,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndSetUpgradeOrchestrationServiceState(
            Common::AsyncOperationSPtr const & operation,
            __inout Api::IFabricUpgradeOrchestrationServiceStateResultPtr & result) override;

        //
        // IRepairManagementClient methods
        //

        Common::AsyncOperationSPtr BeginCreateRepairTask(
            Management::RepairManager::RepairTask const & repairTask,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCreateRepairTask(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion);

        Common::AsyncOperationSPtr BeginCancelRepairTask(
            Management::RepairManager::CancelRepairTaskMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCancelRepairTask(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion);

        Common::AsyncOperationSPtr BeginForceApproveRepairTask(
            Management::RepairManager::ApproveRepairTaskMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndForceApproveRepairTask(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion);

        Common::AsyncOperationSPtr BeginDeleteRepairTask(
            Management::RepairManager::DeleteRepairTaskMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteRepairTask(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginUpdateRepairExecutionState(
            Management::RepairManager::RepairTask const & repairTask,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateRepairExecutionState(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion);

        Common::AsyncOperationSPtr BeginGetRepairTaskList(
            Management::RepairManager::RepairScopeIdentifierBase & scope,
            std::wstring const & taskIdFilter,
            Management::RepairManager::RepairTaskState::Enum stateFilter,
            std::wstring const & executorFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetRepairTaskList(
            Common::AsyncOperationSPtr const & operation,
            __out std::vector<Management::RepairManager::RepairTask> & result);

        Common::AsyncOperationSPtr BeginUpdateRepairTaskHealthPolicy(
            Management::RepairManager::UpdateRepairTaskHealthPolicyMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateRepairTaskHealthPolicy(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion);

        //
        // IHealthClient methods
        //

        Common::ErrorCode ReportHealth(
            ServiceModel::HealthReport && healthReport,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions) override;

        Common::AsyncOperationSPtr BeginGetClusterHealth(
            ServiceModel::ClusterHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetClusterHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ClusterHealth & clusterHealth) override;

        Common::AsyncOperationSPtr BeginGetNodeHealth(
            ServiceModel::NodeHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetNodeHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::NodeHealth & nodeHealth) override;

        Common::AsyncOperationSPtr BeginGetApplicationHealth(
            ServiceModel::ApplicationHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetApplicationHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ApplicationHealth & applicationHealth) override;

        Common::AsyncOperationSPtr BeginGetServiceHealth(
            ServiceModel::ServiceHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetServiceHealth(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ServiceHealth & serviceHealth) override;

        Common::AsyncOperationSPtr BeginGetPartitionHealth(
            ServiceModel::PartitionHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetPartitionHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::PartitionHealth & partitionHealth) override;

        Common::AsyncOperationSPtr BeginGetReplicaHealth(
            ServiceModel::ReplicaHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetReplicaHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::ReplicaHealth & replicaHealth) override;

        Common::AsyncOperationSPtr BeginGetDeployedApplicationHealth(
            ServiceModel::DeployedApplicationHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetDeployedApplicationHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::DeployedApplicationHealth & deployedApplicationHealth) override;

        Common::AsyncOperationSPtr BeginGetDeployedServicePackageHealth(
            ServiceModel::DeployedServicePackageHealthQueryDescription const & queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetDeployedServicePackageHealth(
            Common::AsyncOperationSPtr const &operation,
            __inout ServiceModel::DeployedServicePackageHealth & deployedServicePackageHealth) override;

        Common::AsyncOperationSPtr BeginGetClusterHealthChunk(
            ServiceModel::ClusterHealthChunkQueryDescription && queryDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndGetClusterHealthChunk(
            Common::AsyncOperationSPtr const & operation,
            __inout ServiceModel::ClusterHealthChunk & clusterHealthChunk) override;

        // IInternalHealthClient methods
        Common::ErrorCode InternalReportHealth(
            std::vector<ServiceModel::HealthReport> && reports,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions) override;

        Common::ErrorCode HealthPreInitialize(
            std::wstring const & sourceId,
            FABRIC_HEALTH_REPORT_KIND kind,
            FABRIC_INSTANCE_ID instance) override;
        Common::ErrorCode HealthPostInitialize(
            std::wstring const & sourceId,
            FABRIC_HEALTH_REPORT_KIND kind,
            FABRIC_SEQUENCE_NUMBER sequence,
            FABRIC_SEQUENCE_NUMBER invalidateSequence) override;
        Common::ErrorCode HealthGetProgress(
            std::wstring const & sourceId,
            FABRIC_HEALTH_REPORT_KIND kind,
            __inout FABRIC_SEQUENCE_NUMBER & progress) override;
        Common::ErrorCode HealthSkipSequence(
            std::wstring const & sourceId,
            FABRIC_HEALTH_REPORT_KIND kind,
            FABRIC_SEQUENCE_NUMBER sequence) override;

        //
        // IPropertyManagementClient methods
        //
        Common::AsyncOperationSPtr BeginCreateName(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCreateName(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeleteName(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteName(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginInternalDeleteName(
            Common::NamingUri const & name,
            bool isExplicit,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndInternalDeleteName(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginNameExists(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndNameExists(
            Common::AsyncOperationSPtr const & operation,
            __out bool & doesExist);

        Common::AsyncOperationSPtr BeginEnumerateSubNames(
            Common::NamingUri const & name,
            bool doRecursive,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::AsyncOperationSPtr BeginEnumerateSubNames(
            Common::NamingUri const & name,
            Naming::EnumerateSubNamesToken const & continuationToken,
            bool doRecursive,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndEnumerateSubNames(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::EnumerateSubNamesResult & result);

        Common::AsyncOperationSPtr BeginEnumerateProperties(
            Common::NamingUri const & name,
            bool includeValues,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::AsyncOperationSPtr BeginEnumerateProperties(
            Common::NamingUri const & name,
            bool includeValues,
            Naming::EnumeratePropertiesToken const & continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndEnumerateProperties(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::EnumeratePropertiesResult & result);

        Common::AsyncOperationSPtr BeginGetPropertyMetadata(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetPropertyMetadata(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::NamePropertyMetadataResult & result);

        Common::AsyncOperationSPtr BeginGetProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetProperty(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::NamePropertyResult & result);

        Common::AsyncOperationSPtr BeginSubmitPropertyBatch(
            Naming::NamePropertyOperationBatch && batch,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSubmitPropertyBatch(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::NamePropertyOperationBatchResult & result);
        Common::ErrorCode EndSubmitPropertyBatch(
            Common::AsyncOperationSPtr const & operation,
            __inout Api::IPropertyBatchResultPtr & result);

        Common::AsyncOperationSPtr BeginDeleteProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteProperty(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginPutProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            FABRIC_PROPERTY_TYPE_ID storageTypeId,
            Common::ByteBuffer &&data,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndPutProperty(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginPutCustomProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            FABRIC_PROPERTY_TYPE_ID storageTypeId,
            Common::ByteBuffer &&data,
            std::wstring const & customTypeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndPutCustomProperty(Common::AsyncOperationSPtr const & operation);

        //
        // IServiceGroupManagementClient methods
        //
        Common::AsyncOperationSPtr BeginCreateServiceGroup(
            ServiceModel::PartitionedServiceDescWrapper const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateServiceGroup(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCreateServiceGroupFromTemplate(
            ServiceModel::ServiceGroupFromTemplateDescription const & serviceGroupFromTemplateDesc,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndCreateServiceGroupFromTemplate(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginDeleteServiceGroup(
            Common::NamingUri const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndDeleteServiceGroup(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetServiceGroupDescription(
            Common::NamingUri const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndGetServiceGroupDescription(
            Common::AsyncOperationSPtr const &,
            __inout Naming::ServiceGroupDescriptor &);

        Common::AsyncOperationSPtr BeginUpdateServiceGroup(
            Common::NamingUri const & name,
            Naming::ServiceUpdateDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndUpdateServiceGroup(Common::AsyncOperationSPtr const &);

        //
        // IInfrastructureServiceClient methods
        //
        Common::AsyncOperationSPtr BeginInvokeInfrastructureCommand(
            bool isAdminOperation,
            Common::NamingUri const & serviceName,
            std::wstring const & command,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndInvokeInfrastructureCommand(
            Common::AsyncOperationSPtr const &,
            __out std::wstring &);

        //
        // IFaultManagementClient methods
        //
        Common::AsyncOperationSPtr BeginRestartNode(
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            bool shouldCreateFabricDump,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRestartNode(
            Common::AsyncOperationSPtr const &,
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            __inout Api::IRestartNodeResultPtr &result);

        Common::AsyncOperationSPtr BeginStartNode(
            Management::FaultAnalysisService::StartNodeDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndStartNode(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::StartNodeDescriptionUsingNodeName const & description,
            __inout Api::IStartNodeResultPtr &result);

        Common::AsyncOperationSPtr BeginStopNode(
            Management::FaultAnalysisService::StopNodeDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndStopNode(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::StopNodeDescriptionUsingNodeName const & description,
            __inout Api::IStopNodeResultPtr &result);

        Common::AsyncOperationSPtr BeginStopNodeInternal(
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            DWORD durationInSeconds,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndStopNodeInternal(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginRestartDeployedCodePackage(
            Management::FaultAnalysisService::RestartDeployedCodePackageDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRestartDeployedCodePackage(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::RestartDeployedCodePackageDescriptionUsingNodeName const & description,
            __inout Api::IRestartDeployedCodePackageResultPtr &result);

        Common::AsyncOperationSPtr BeginMovePrimary(
            Management::FaultAnalysisService::MovePrimaryDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndMovePrimary(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::MovePrimaryDescriptionUsingNodeName const & description,
            __inout Api::IMovePrimaryResultPtr &result);

        Common::AsyncOperationSPtr BeginMoveSecondary(
            Management::FaultAnalysisService::MoveSecondaryDescriptionUsingNodeName const & description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndMoveSecondary(
            Common::AsyncOperationSPtr const &,
            Management::FaultAnalysisService::MoveSecondaryDescriptionUsingNodeName const & description,
            __inout Api::IMoveSecondaryResultPtr &result);

        //
        // ITestManagementClient methods
        //

        Common::AsyncOperationSPtr BeginInvokeDataLoss(
            Management::FaultAnalysisService::InvokeDataLossDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndInvokeDataLoss(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr FabricClientImpl::BeginGetInvokeDataLossProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode FabricClientImpl::EndGetInvokeDataLossProgress(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IInvokeDataLossProgressResultPtr &result);

        Common::AsyncOperationSPtr BeginInvokeQuorumLoss(
            Management::FaultAnalysisService::InvokeQuorumLossDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndInvokeQuorumLoss(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr FabricClientImpl::BeginGetInvokeQuorumLossProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode FabricClientImpl::EndGetInvokeQuorumLossProgress(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IInvokeQuorumLossProgressResultPtr &result);

        Common::AsyncOperationSPtr BeginRestartPartition(
            Management::FaultAnalysisService::RestartPartitionDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRestartPartition(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr FabricClientImpl::BeginGetRestartPartitionProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode FabricClientImpl::EndGetRestartPartitionProgress(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IRestartPartitionProgressResultPtr &result);

        Common::AsyncOperationSPtr BeginGetTestCommandStatusList(
            DWORD stateFilter,
            DWORD typeFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetTestCommandStatusList(
            Common::AsyncOperationSPtr const &operation,
            __inout std::vector<ServiceModel::TestCommandListQueryResult> &result);


        Common::AsyncOperationSPtr BeginCancelTestCommand(
            Management::FaultAnalysisService::CancelTestCommandDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCancelTestCommand(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginStartNodeTransition(
//            Management::FaultAnalysisService::StartNodeTransitionDescription const &,
            Management::FaultAnalysisService::StartNodeTransitionDescription const &,

            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndStartNodeTransition(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetNodeTransitionProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode FabricClientImpl::EndGetNodeTransitionProgress(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::INodeTransitionProgressResultPtr &result);

            // Chaos
        Common::AsyncOperationSPtr BeginStartChaos(
            Management::FaultAnalysisService::StartChaosDescription &&,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndStartChaos(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginStopChaos(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndStopChaos(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaos(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode FabricClientImpl::EndGetChaos(
            Common::AsyncOperationSPtr const &,
            __out Api::IChaosDescriptionResultPtr &result);
        Common::ErrorCode FabricClientImpl::EndGetChaos(
            Common::AsyncOperationSPtr const &,
            __out Api::ISystemServiceApiResultPtr &result);

        Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaosReport(
            Management::FaultAnalysisService::GetChaosReportDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode FabricClientImpl::EndGetChaosReport(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IChaosReportResultPtr &result);

        Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaosEvents(
            Management::FaultAnalysisService::GetChaosEventsDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode FabricClientImpl::EndGetChaosEvents(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IChaosEventsSegmentResultPtr &result);
        Common::ErrorCode FabricClientImpl::EndGetChaosEvents(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::ISystemServiceApiResultPtr &result);

        Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaosSchedule(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode FabricClientImpl::EndGetChaosSchedule(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::IChaosScheduleDescriptionResultPtr &result);
        Common::ErrorCode FabricClientImpl::EndGetChaosSchedule(
            Common::AsyncOperationSPtr const &operation,
            __inout Api::ISystemServiceApiResultPtr &result);

        Common::AsyncOperationSPtr FabricClientImpl::BeginSetChaosSchedule(
            Management::FaultAnalysisService::SetChaosScheduleDescription const & setChaosScheduleDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::AsyncOperationSPtr FabricClientImpl::BeginSetChaosSchedule(
            std::wstring const & state,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode FabricClientImpl::EndSetChaosSchedule(
            Common::AsyncOperationSPtr const &operation);

        //
        // ITestManagementClientInternal methods
        //
        Common::AsyncOperationSPtr BeginAddUnreliableLeaseBehavior(
            std::wstring const & nodeName,
            std::wstring const & alias,
            std::wstring const & behaviorString,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndAddUnreliableLeaseBehavior(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginRemoveUnreliableLeaseBehavior(
            std::wstring const & nodeName,
            std::wstring const & alias,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndRemoveUnreliableLeaseBehavior(
            Common::AsyncOperationSPtr const &);

        //
        // IInternalInfrastructureServiceClient methods
        //
        Common::AsyncOperationSPtr BeginRunCommand(
            std::wstring const &command,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRunCommand(
            Common::AsyncOperationSPtr const &,
            __out std::wstring & result);

        Common::AsyncOperationSPtr BeginReportStartTaskSuccess(
            std::wstring const &taskId,
            ULONGLONG instanceId,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndReportStartTaskSuccess(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginReportFinishTaskSuccess(
            std::wstring const &taskId,
            ULONGLONG instanceId,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndReportFinishTaskSuccess(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginReportTaskFailure(
            std::wstring const &taskId,
            ULONGLONG instanceId,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndReportTaskFailure(Common::AsyncOperationSPtr const &);

        //
        // IImageStoreClient methods.
        //

        Common::ErrorCode Upload(
            std::wstring const & imageStoreConnectionString,
            std::wstring const & sourceFullpath,
            std::wstring const & destinationRelativePath,
            bool const shouldOverwrite);

        Common::ErrorCode Delete(
            std::wstring const & imageStoreConnectionString,
            std::wstring const & relativePath);

        //
        // IFileStoreServiceClient methods.
        //
        Common::AsyncOperationSPtr BeginUploadFile(
            std::wstring const & serviceName,
            std::wstring const & sourceFullPath,
            std::wstring const & storeRelativePath,
            bool shouldOverwrite,
            Api::IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndUploadFile(
            Common::AsyncOperationSPtr const &operation);

        Common::AsyncOperationSPtr BeginDownloadFile(
            std::wstring const & serviceName,
            std::wstring const & storeRelativePath,
            std::wstring const & destinationFullPath,
            Management::FileStoreService::StoreFileVersion const & version,
            std::vector<std::wstring> const & availableShares,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndDownloadFile(
            Common::AsyncOperationSPtr const &operation);

        //
        // IInternalFileStoreServiceClient methods.
        //
        Common::AsyncOperationSPtr BeginGetStagingLocation(
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndGetStagingLocation(
            Common::AsyncOperationSPtr const &,
            __out std::wstring & stagingLocationShare);

        Common::AsyncOperationSPtr BeginUpload(
            Common::Guid const & targetPartitionId,
            std::wstring const & stagingRelativePath,
            std::wstring const & storeRelativePath,
            bool shouldOverwrite,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndUpload(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginCopy(
            Common::Guid const & targetPartitionId,
            std::wstring const & sourceStoreFileInfo,
            Management::FileStoreService::StoreFileVersion sourceVersion,
            std::wstring const & destinationStoreRelativePath,
            bool shouldOverwrite,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndCopy(
            Common::AsyncOperationSPtr const &operation);

        Common::AsyncOperationSPtr BeginCheckExistence(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndCheckExistence(
            Common::AsyncOperationSPtr const &,
            __out bool & result);

        Common::AsyncOperationSPtr BeginListFiles(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            std::wstring const & continuationToken,
            BOOLEAN const & shouldIncludeDetails,
            BOOLEAN const & isRecursive,
            bool isPaging,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndListFiles(
            Common::AsyncOperationSPtr const &,
            __out std::vector<Management::FileStoreService::StoreFileInfo> & availableFiles,
            __out std::vector<Management::FileStoreService::StoreFolderInfo> & availableFolders,
            __out std::vector<std::wstring> & availableShares,
            __out std::wstring & continuationToken);

        Common::AsyncOperationSPtr BeginDelete(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndDelete(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginInternalGetStoreLocation(
            Common::NamingUri const & serviceName,
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndInternalGetStoreLocation(
            Common::AsyncOperationSPtr const &,
            __out std::wstring & storeLocationShare);

        Common::AsyncOperationSPtr BeginInternalGetStoreLocations(
            Common::Guid const & targetPartitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndInternalGetStoreLocations(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> & secondaryShares);

        Common::AsyncOperationSPtr BeginInternalListFile(
            Common::NamingUri const & serviceName,
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndInternalListFile(
            Common::AsyncOperationSPtr const &parent,
            __out bool & isPresent,
            __out Management::FileStoreService::FileState::Enum & currentState,
            __out Management::FileStoreService::StoreFileVersion & currentVersion,
            __out std::wstring & storeShareLocation);

        Common::AsyncOperationSPtr BeginListUploadSession(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndListUploadSession(
            Common::AsyncOperationSPtr const & operation,
            __out Management::FileStoreService::UploadSession & uploadSessions);

        Common::AsyncOperationSPtr BeginCreateUploadSession(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::Guid const & sessionId,
            uint64 fileSize,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCreateUploadSession(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginUploadChunk(
            Common::Guid const & targetPartitionId,
            std::wstring const & localSource,
            Common::Guid const & sessionId,
            uint64 startPosition,
            uint64 endPosition,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUploadChunk(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginUploadChunkContent(
            Common::Guid const & targetPartitionId,
            __inout Transport::MessageUPtr & chunkContentMessage,
            __inout Management::FileStoreService::UploadChunkContentDescription & uploadChunkContentDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUploadChunkContent(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeleteUploadSession(
            Common::Guid const & targetPartitionId,
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteUploadSession(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginCommitUploadSession(
            Common::Guid const & targetPartitionId,
            std::wstring const & storeRelativePath,
            Common::Guid const & sessionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCommitUploadSession(
            Common::AsyncOperationSPtr const & operation);

        //
        // ITestClient methods.
        //
        Common::ErrorCode GetNthNamingPartitionId(
            ULONG n,
            __inout Reliability::ConsistencyUnitId &partitionId);

        Common::AsyncOperationSPtr BeginResolvePartition(
            Reliability::ConsistencyUnitId const& partitionId,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndResolvePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Api::IResolvedServicePartitionResultPtr &result);

        Common::ErrorCode EndResolvePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::ResolvedServicePartitionSPtr &result);

        Common::AsyncOperationSPtr BeginResolveNameOwner(
            Common::NamingUri const& name,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndResolveNameOwner(
            Common::AsyncOperationSPtr const & operation,
            __inout Api::IResolvedServicePartitionResultPtr &result);

        Common::ErrorCode NodeIdFromNameOwnerAddress(
            std::wstring const& address,
            Federation::NodeId & federationNodeId);

        Common::ErrorCode NodeIdFromFMAddress(
            std::wstring const& address,
            Federation::NodeId & federationNodeId);

        Common::ErrorCode GetCurrentGatewayAddress(
            __inout std::wstring &result);

        //
        // IInternalTokenValidationServiceClient methods.
        //

        Common::AsyncOperationSPtr BeginGetMetadata(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndGetMetadata(
            Common::AsyncOperationSPtr const &operation,
            __out ServiceModel::TokenValidationServiceMetadata &metadata);

        Common::AsyncOperationSPtr BeginValidateToken(
            std::wstring const &token,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndValidateToken(
            Common::AsyncOperationSPtr const &operation,
            __out Common::TimeSpan &expirationTime,
            __out std::vector<ServiceModel::Claim> &claimsList);

        static const ClientEventSource Trace;
        static const Common::StringLiteral CommunicationTraceComponent;
        static const Common::StringLiteral LifeCycleTraceComponent;

        Common::AsyncOperationSPtr BeginInternalForwardToService(
            ClientServerRequestMessageUPtr && message,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndInternalForwardToService(
            Common::AsyncOperationSPtr const &operation,
            __inout ClientServerReplyMessageUPtr &reply);

        Common::AsyncOperationSPtr BeginInternalLocationChangeNotificationRequest(
            std::vector<Naming::ServiceLocationNotificationRequestData> && requests,
            Transport::FabricActivityHeader && activityHeader,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndInternalLocationChangeNotificationRequest(
            Common::AsyncOperationSPtr const & operation,
            __inout std::vector<Naming::ResolvedServicePartitionSPtr> & addresses,
            __inout std::vector<Naming::AddressDetectionFailureSPtr> & failures,
            __out bool & updateNonProcessedRequest,
            __inout std::unique_ptr<Naming::NameRangeTuple> & firstNonProcessedRequest,
            __inout Transport::FabricActivityHeader & activityHeader);

        // ISecretStoreClient methods
        Common::AsyncOperationSPtr BeginGetSecrets(
            Management::CentralSecretService::GetSecretsDescription & getSecretsDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndGetSecrets(
            Common::AsyncOperationSPtr const & operation,
            __out Management::CentralSecretService::SecretsDescription & result) override;

        Common::AsyncOperationSPtr BeginSetSecrets(
            Management::CentralSecretService::SecretsDescription & secretsDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndSetSecrets(
            Common::AsyncOperationSPtr const & operation,
            __out Management::CentralSecretService::SecretsDescription & result) override;

        Common::AsyncOperationSPtr BeginRemoveSecrets(
            Management::CentralSecretService::SecretReferencesDescription & secretReferencesDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndRemoveSecrets(
            Common::AsyncOperationSPtr const & operation,
            __out Management::CentralSecretService::SecretReferencesDescription & result) override;

        Common::AsyncOperationSPtr BeginGetSecretVersions(
            Management::CentralSecretService::SecretReferencesDescription & secretReferencesDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndGetSecretVersions(
            Common::AsyncOperationSPtr const & operation,
            __out Management::CentralSecretService::SecretReferencesDescription & result) override;

        // IResourceManagerClient methods
        Common::AsyncOperationSPtr BeginClaimResource(
            Management::ResourceManager::Claim const & claim,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndClaimResource(
            Common::AsyncOperationSPtr const & operation,
            __out Management::ResourceManager::ResourceMetadata & result) override;

        Common::AsyncOperationSPtr BeginReleaseResource(
            Management::ResourceManager::Claim const & claim,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndReleaseResource(
            Common::AsyncOperationSPtr const & operation) override;

        // Access control
        Common::AsyncOperationSPtr BeginSetAcl(
            AccessControl::ResourceIdentifier const &resource,
            AccessControl::FabricAcl const & acl,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndSetAcl(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginGetAcl(
            AccessControl::ResourceIdentifier const &resource,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::ErrorCode EndGetAcl(
            Common::AsyncOperationSPtr const & operation,
            _Out_ AccessControl::FabricAcl & fabricAcl) override;

        //
        // IGatewayResourceManager client
        //
        virtual Common::AsyncOperationSPtr BeginCreateOrUpdateGatewayResource(
            std::wstring && description,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        virtual Common::ErrorCode EndCreateOrUpdateGatewayResource(
            Common::AsyncOperationSPtr const &,
            __out std::wstring & descriptionStr) override;

        virtual Common::AsyncOperationSPtr BeginGetGatewayResourceList(
            ServiceModel::ModelV2::GatewayResourceQueryDescription && gatewayQueryDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        virtual Common::ErrorCode EndGetGatewayResourceList(
            Common::AsyncOperationSPtr const &,
            std::vector<std::wstring> & list,
            ServiceModel::PagingStatusUPtr & pagingStatus) override;

        virtual Common::AsyncOperationSPtr BeginDeleteGatewayResource(
            std::wstring const & resourceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) override;
        virtual Common::ErrorCode EndDeleteGatewayResource(
            Common::AsyncOperationSPtr const &) override;

        // Public for tests only
        bool Test_TryGetCachedServiceResolution(
            Common::NamingUri const & name,
            Naming::PartitionKey const & key,
            __out Naming::ResolvedServicePartitionSPtr & result);

        HealthReportingComponent & Test_GetReportingComponent() const { return healthClient_->HealthReportingClient; }

        Common::AsyncOperationSPtr BeginInternalGetServiceDescription(
            Common::NamingUri  const &name,
            Common::ActivityId const & activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) override;

        Common::AsyncOperationSPtr BeginInternalGetServiceDescription(
            Common::NamingUri  const &name,
            Transport::FabricActivityHeader activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndInternalGetServiceDescription(
            Common::AsyncOperationSPtr const &operation,
            __inout Naming::PartitionedServiceDescriptor &description) override;

        Common::ErrorCode EndInternalGetServiceDescription(
            Common::AsyncOperationSPtr const & operation,
            __inout LruClientCacheEntrySPtr & cacheEntry);

        class RequestReplyAsyncOperation;

    public:

        //
        // Temporary for testing only
        //

        Common::AsyncOperationSPtr Test_BeginCreateNamespaceManager(
            std::wstring const & svcName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode Test_EndCreateNamespaceManager(
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr Test_BeginResolveNamespaceManager(
            std::wstring const & svcName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode Test_EndResolveNamespaceManager(
            Common::AsyncOperationSPtr const &,
            __out std::wstring &);

        Common::AsyncOperationSPtr Test_BeginTestNamespaceManager(
            std::wstring const & svcName,
            size_t byteCount,
            Transport::ISendTarget::SPtr const & directTarget,
            SystemServices::SystemServiceLocation const & primaryLocation,
            bool gatewayProxy,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode Test_EndTestNamespaceManager(
            Common::AsyncOperationSPtr const &,
            __out std::vector<byte> & result);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        class ClientAsyncOperationBase;
        class ForwardToServiceAsyncOperation;
        class DeleteServiceAsyncOperation;
        class GetServiceDescriptionAsyncOperation;
        class LocationChangeNotificationAsyncOperation;
        class EnsureOpenedAsyncOperation;
        class CreateContainerAppAsyncOperation;

        void InitializeConnectionManager(Naming::INamingMessageProcessorSPtr const & = Naming::INamingMessageProcessorSPtr());
        void InitializeTraceContextFromSettings();
        Common::ErrorCode InitializeSecurity();
        void InitializeInnerClients();
        Common::ErrorCode OpenInnerClients();

        Common::ErrorCode EnsureOpened();
        void CleanupOnClose();

        Common::ErrorCode GetImageStoreClient(std::wstring const & imageStoreConnectionString, __out Common::RootedObjectPointer<Management::ImageStore::IImageStore> & imageStore);

        bool GetFabricUri(std::wstring const& uriString, __out Common::NamingUri &uri, bool allowSystemApplication = true);

        std::wstring GetLocalGatewayAddress();

        Common::ErrorCode EndInternalGetServiceDescription(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::PartitionedServiceDescriptor & description,
            __inout LruClientCacheEntrySPtr & cacheEntry);

        void OnConnected(
            ClientConnectionManager::HandlerId,
            Transport::ISendTarget::SPtr const &,
            Naming::GatewayDescription const &);
        void OnDisconnected(
            ClientConnectionManager::HandlerId,
            Transport::ISendTarget::SPtr const &,
            Naming::GatewayDescription const &,
            Common::ErrorCode const &);
        Common::ErrorCode OnClaimsRetrieval(
            ClientConnectionManager::HandlerId,
            std::shared_ptr<Transport::ClaimsRetrievalMetadata> const &,
            __out std::wstring & token);
        Common::ErrorCode OnNotificationReceived(std::vector<ServiceNotificationResultSPtr> const &);

        static bool IsFASOrUOSReconfiguring(Common::ErrorCode const& error);

        std::wstring traceContext_;

        Common::RwLock securitySettingsUpdateLock_;
        Common::FabricNodeConfigSPtr config_;
        FabricClientInternalSettingsSPtr settings_;

        std::vector<std::wstring> gatewayAddresses_;
        ClientConnectionManagerSPtr clientConnectionManager_;

        ClientConnectionManager::HandlerId connectionHandlersId_;

        Api::IServiceNotificationEventHandlerPtr serviceNotificationEventHandler_;
        Common::RwLock notificationEventHandlerLock_;

        Api::IClientConnectionEventHandlerPtr connectionEventHandler_;
        Common::RwLock connectionEventHandlerLock_;

        LruClientCacheManagerUPtr lruCacheManager_;
        ServiceNotificationClientUPtr notificationClient_;
        FileTransferClientUPtr fileTransferClient_;
        ClientHealthReportingSPtr healthClient_;

        ServiceAddressTrackerManagerSPtr serviceAddressManager_;

        mutable Common::RwLock stateLock_;
        bool isOpened_;

        std::map<std::wstring, Management::ImageStore::ImageStoreUPtr> imageStoreClientsMap_;
        Common::ExclusiveLock mapLock_;
        Transport::RoleMask::Enum role_;
        bool isClientRoleAuthorized_;
    };
}
