// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerDescription : 
        public Serialization::FabricSerializable,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;

        DEFAULT_COPY_ASSIGNMENT(ContainerDescription)
        DEFAULT_COPY_CONSTRUCTOR(ContainerDescription)

        DEFAULT_MOVE_ASSIGNMENT(ContainerDescription)
        DEFAULT_MOVE_CONSTRUCTOR(ContainerDescription)

    public:
        ContainerDescription();
        ContainerDescription(
            std::wstring const & applicationName,
            std::wstring const & serviceName,
            std::wstring const & applicationId,
            std::wstring const & containerName,
            std::wstring const & entryPoint,
            ServiceModel::ContainerIsolationMode::Enum const & isolationMode,
            std::wstring const & hostName,
            std::wstring const & deploymentFolder,
            std::wstring const & nodeWorkFolder,
            std::map<std::wstring, std::wstring> const & portBindings,
            ServiceModel::LogConfigDescription const & logConfig,
            std::vector<ServiceModel::ContainerVolumeDescription> const & volumes,
            std::vector<ServiceModel::ContainerLabelDescription> const & labels,
            std::vector<std::wstring> const & dnsServers,
            ServiceModel::RepositoryCredentialsDescription const & repositoryCredentials,
            ServiceModel::ContainerHealthConfigDescription const & healthConfig,
            ContainerNetworkConfigDescription const & networkConfig,
            std::vector<std::wstring> const & securityOptions,
#if defined(PLATFORM_UNIX)
            ContainerPodDescription const & podDesc,
#endif
            bool removeServiceFabricRuntimeAccess = true,
            std::wstring const & groupContainerName = std::wstring(),
            bool useDefaultRepositoryCredentials = false,
            bool useTokenAuthenticationCredentials = false,
            bool autoRemove = true,
            bool runInteractive = false,
            bool isContainerRoot = false,
            std::wstring const & codePackageName = std::wstring(),
            std::wstring const & servicePackageActivationId = std::wstring(),
            std::wstring const & partitionId = std::wstring(),
            std::map<std::wstring, std::wstring> const & bindMounts = std::map<std::wstring, std::wstring>());

        virtual ~ContainerDescription();

        __declspec(property(get = get_ApplicationName)) std::wstring const & ApplicationName;
        inline std::wstring const & get_ApplicationName() const { return applicationName_; };

        __declspec(property(get = get_ServiceName)) std::wstring const & ServiceName;
        inline std::wstring const & get_ServiceName() const { return serviceName_; };

        __declspec(property(get = get_ApplicationId)) std::wstring const & ApplicationId;
        inline std::wstring const & get_ApplicationId() const { return applicationId_; };

        __declspec(property(get = get_ContainerName)) std::wstring const & ContainerName;
        inline std::wstring const & get_ContainerName() const { return containerName_; };

        __declspec(property(get = get_GroupContainerName)) std::wstring const & GroupContainerName;
        inline std::wstring const & get_GroupContainerName() const { return groupContainerName_; };

        __declspec(property(get = get_AssignedIP)) std::wstring const & AssignedIP;
        inline std::wstring const & get_AssignedIP() const { return assignedIp_; };

#if defined(PLATFORM_UNIX)
        __declspec(property(get = get_PodDescription)) ContainerPodDescription const & PodDescription;
        inline ContainerPodDescription const & get_PodDescription() const { return podDescription_; };
#endif

        __declspec(property(get = get_DeploymentFolder)) std::wstring const & DeploymentFolder;
        inline std::wstring const & get_DeploymentFolder() const { return deploymentFolder_; };

        __declspec(property(get = get_EntryPoint)) std::wstring const & EntryPoint;
        inline std::wstring const & get_EntryPoint() const { return entryPoint_; };

        __declspec(property(get = get_IsolationMode)) ServiceModel::ContainerIsolationMode::Enum const & IsolationMode;
        inline ServiceModel::ContainerIsolationMode::Enum const & get_IsolationMode() const { return isolationMode_; };

        __declspec(property(get = get_NodeWorkFolder)) std::wstring const & NodeWorkFolder;
        inline std::wstring const & get_NodeWorkFolder() const { return nodeWorkFolder_; };

        __declspec(property(get = get_Hostname)) std::wstring const & Hostname;
        inline std::wstring const & get_Hostname() const { return hostName_; };

        __declspec(property(get = get_AutoRemove)) bool const & AutoRemove;
        inline bool const & get_AutoRemove() const { return autoRemove_; };

        __declspec(property(get = get_RunInteractive)) bool const & RunInteractive;
        inline bool const & get_RunInteractive() const { return runInteractive_; };

        __declspec(property(get = get_IsContainerRoot)) bool const & IsContainerRoot;
        inline bool const & get_IsContainerRoot() const { return isContainerRoot_; };

        __declspec(property(get = get_PortBindings))  std::map<std::wstring, std::wstring> const & PortBindings;
        inline std::map<std::wstring, std::wstring> const & get_PortBindings() const { return portBindings_; };

        __declspec(property(get = get_LogConfig))  ServiceModel::LogConfigDescription const & LogConfig;
        inline  ServiceModel::LogConfigDescription const & get_LogConfig() const { return logConfig_; };

        __declspec(property(get = get_ContainerVolumes))  std::vector<ServiceModel::ContainerVolumeDescription> const & ContainerVolumes;
        inline std::vector<ServiceModel::ContainerVolumeDescription> const & get_ContainerVolumes() const { return volumes_; };

        __declspec(property(get = get_ContainerLabels))  std::vector<ServiceModel::ContainerLabelDescription> const & ContainerLabels;
        inline std::vector<ServiceModel::ContainerLabelDescription> const & get_ContainerLabels() const { return labels_; };

        __declspec(property(get = get_DnsServers))  std::vector<std::wstring> const & DnsServers;
        inline std::vector<std::wstring> const & get_DnsServers() const { return dnsServers_; };

        __declspec(property(get = get_SecurityOptions))  std::vector<std::wstring> const & SecurityOptions;
        inline std::vector<std::wstring> const & get_SecurityOptions() const { return securityOptions_; };

        __declspec(property(get = get_RepositoryCredentials))  ServiceModel::RepositoryCredentialsDescription const & RepositoryCredentials;
        inline ServiceModel::RepositoryCredentialsDescription const & get_RepositoryCredentials() const { return repositoryCredentials_; };

        __declspec(property(get = get_removeServiceFabricRuntimeAccess))  bool RemoveServiceFabricRuntimeAccess;
        inline bool get_removeServiceFabricRuntimeAccess() const { return removeServiceFabricRuntimeAccess_; };

        __declspec(property(get = get_HealthConfig))  ServiceModel::ContainerHealthConfigDescription const & HealthConfig;
        inline ServiceModel::ContainerHealthConfigDescription const & get_HealthConfig() const { return healthConfig_; }

        inline void SetIsolationMode(ServiceModel::ContainerIsolationMode::Enum isolationMode) { isolationMode_ = isolationMode; };

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        inline std::wstring const & get_CodePackageName() const { return codePackageName_; };

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        inline std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; };

        __declspec(property(get = get_PartitionId)) std::wstring const & PartitionId;
        inline std::wstring const & get_PartitionId() const { return partitionId_; };

        __declspec(property(get = get_BindMounts))  std::map<std::wstring, std::wstring> const & BindMounts;
        inline std::map<std::wstring, std::wstring> const & get_BindMounts() const { return bindMounts_; };

        __declspec(property(get = get_UseDefaultRepositoryCredentials)) bool UseDefaultRepositoryCredentials;
        inline bool get_UseDefaultRepositoryCredentials() const { return useDefaultRepositoryCredentials_; };

        __declspec(property(get = get_UseTokenAuthenticationCredentials)) bool UseTokenAuthenticationCredentials;
        inline bool get_UseTokenAuthenticationCredentials() const { return useTokenAuthenticationCredentials_; };

        __declspec(property(get = get_ContainerNetworkConfig)) ContainerNetworkConfigDescription const & NetworkConfig;
        inline ContainerNetworkConfigDescription const & get_ContainerNetworkConfig() const { return networkConfig_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_DESCRIPTION & fabricContainerDescription) const;

#if !defined(PLATFORM_UNIX)
        FABRIC_FIELDS_30(applicationName_, containerName_, deploymentFolder_, isolationMode_,
            nodeWorkFolder_, portBindings_, logConfig_, volumes_, labels_,
            dnsServers_, assignedIp_, repositoryCredentials_,
            securityOptions_, applicationId_, entryPoint_, hostName_, groupContainerName_,
            autoRemove_, runInteractive_, isContainerRoot_, healthConfig_, serviceName_,
            codePackageName_, servicePackageActivationId_, partitionId_, useDefaultRepositoryCredentials_, removeServiceFabricRuntimeAccess_, bindMounts_,
            useTokenAuthenticationCredentials_, networkConfig_);
#else
        FABRIC_FIELDS_31(applicationName_, containerName_, deploymentFolder_, isolationMode_,
            nodeWorkFolder_, portBindings_, logConfig_, volumes_, labels_,
            dnsServers_, assignedIp_, repositoryCredentials_,
            securityOptions_, applicationId_, entryPoint_, hostName_, groupContainerName_, podDescription_,
            autoRemove_, runInteractive_, isContainerRoot_, healthConfig_, serviceName_,
            codePackageName_, servicePackageActivationId_, partitionId_, useDefaultRepositoryCredentials_, removeServiceFabricRuntimeAccess_, bindMounts_,
            useTokenAuthenticationCredentials_, networkConfig_);
#endif

    private:
        std::wstring applicationName_;
        std::wstring serviceName_;
        std::wstring applicationId_;
        std::wstring containerName_;
        std::wstring deploymentFolder_;
        std::wstring nodeWorkFolder_;
        std::wstring assignedIp_;
        std::wstring entryPoint_;
        std::wstring hostName_;
        std::wstring groupContainerName_;
#if defined(PLATFORM_UNIX)
        ContainerPodDescription podDescription_;
#endif
        ServiceModel::ContainerIsolationMode::Enum isolationMode_;
        std::map<std::wstring, std::wstring> portBindings_;
        ServiceModel::LogConfigDescription logConfig_;
        std::vector<ServiceModel::ContainerVolumeDescription> volumes_;
        std::vector<ServiceModel::ContainerLabelDescription> labels_;
        std::vector<std::wstring> dnsServers_;
        std::vector<std::wstring> securityOptions_;
        ServiceModel::RepositoryCredentialsDescription repositoryCredentials_;
        bool removeServiceFabricRuntimeAccess_;
        ServiceModel::ContainerHealthConfigDescription healthConfig_;
        bool autoRemove_;
        bool runInteractive_;
        bool isContainerRoot_;
        std::wstring codePackageName_;
        std::wstring servicePackageActivationId_;
        std::wstring partitionId_;
        bool useDefaultRepositoryCredentials_;
        std::map<std::wstring, std::wstring> bindMounts_;
        bool useTokenAuthenticationCredentials_;
        ContainerNetworkConfigDescription networkConfig_;
    };
}