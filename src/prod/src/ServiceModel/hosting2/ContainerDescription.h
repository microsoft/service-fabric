// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerDescription : public Serialization::FabricSerializable
    {

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
            std::wstring const & assignedIp,
            std::map<std::wstring, std::wstring> const & portBindings,
            ServiceModel::LogConfigDescription const & logConfig,
            std::vector<ServiceModel::ContainerVolumeDescription> const & volumes,
            std::vector<std::wstring> const & dnsServers,
            ServiceModel::RepositoryCredentialsDescription const & repositoryCredentials,
            ServiceModel::ContainerHealthConfigDescription const & healthConfig,
            std::vector<std::wstring> const & securityOptions,
            std::wstring const & groupContainerName = std::wstring(),
            bool autoRemove = true,
            bool runInteractive = false,
            bool isContainerRoot = false,
            std::wstring const & codePackageName = std::wstring(),
            std::wstring const & servicePackageActivationId = std::wstring(),
            std::wstring const & partitionId = std::wstring());

        ContainerDescription(ContainerDescription const & other);

        ContainerDescription(ContainerDescription && other);
        virtual ~ContainerDescription();
        ContainerDescription const & operator = (ContainerDescription const & other);
        ContainerDescription const & operator = (ContainerDescription && other);
        
        __declspec(property(get = get_ApplicationName)) std::wstring const & ApplicationName;
        inline std::wstring const & get_ApplicationName() const { return applicationName_; };

        __declspec(property(get = get_ServiceName)) std::wstring const & ServiceName;
        inline std::wstring const & get_ServiceName() const { return serviceName_; };

        __declspec(property(get = get_ApplicationId)) std::wstring const & ApplicationId;
        inline std::wstring const & get_ApplicationId() const { return applicationId_; };

        __declspec(property(get=get_ContainerName)) std::wstring const & ContainerName;
        inline std::wstring const & get_ContainerName() const { return containerName_; };

        __declspec(property(get = get_GroupContainerName)) std::wstring const & GroupContainerName;
        inline std::wstring const & get_GroupContainerName() const { return groupContainerName_; };

        __declspec(property(get = get_AssignedIP)) std::wstring const & AssignedIP;
        inline std::wstring const & get_AssignedIP() const { return assignedIp_; };

        __declspec(property(get=get_DeploymentFolder)) std::wstring const & DeploymentFolder;
        inline std::wstring const & get_DeploymentFolder() const { return deploymentFolder_; };

        __declspec(property(get = get_EntryPoint)) std::wstring const & EntryPoint;
        inline std::wstring const & get_EntryPoint() const { return entryPoint_; };

        __declspec(property(get = get_IsolationMode)) ServiceModel::ContainerIsolationMode::Enum const & IsolationMode;
        inline ServiceModel::ContainerIsolationMode::Enum const & get_IsolationMode() const { return isolationMode_; };

        __declspec(property(get=get_NodeWorkFolder)) std::wstring const & NodeWorkFolder;
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

        __declspec(property(get = get_DnsServers))  std::vector<std::wstring> const & DnsServers;
        inline std::vector<std::wstring> const & get_DnsServers() const { return dnsServers_; };

        __declspec(property(get = get_SecurityOptions))  std::vector<std::wstring> const & SecurityOptions;
        inline std::vector<std::wstring> const & get_SecurityOptions() const { return securityOptions_; };

        __declspec(property(get = get_RepositoryCredentials))  ServiceModel::RepositoryCredentialsDescription const & RepositoryCredentials;
        inline ServiceModel::RepositoryCredentialsDescription const & get_RepositoryCredentials() const { return repositoryCredentials_; };

        __declspec(property(get = get_HealthConfig))  ServiceModel::ContainerHealthConfigDescription const & HealthConfig;
        inline ServiceModel::ContainerHealthConfigDescription const & get_HealthConfig() const { return healthConfig_; }

        inline void SetIsolationMode(ServiceModel::ContainerIsolationMode::Enum isolationMode) { isolationMode_ = isolationMode; };

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        inline std::wstring const & get_CodePackageName() const { return codePackageName_; };

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        inline std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; };

        __declspec(property(get = get_PartitionId)) std::wstring const & PartitionId;
        inline std::wstring const & get_PartitionId() const { return partitionId_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_DESCRIPTION & fabricContainerDescription) const;

        FABRIC_FIELDS_24(applicationName_, containerName_, deploymentFolder_,
            isolationMode_, nodeWorkFolder_, portBindings_,
            logConfig_, volumes_, dnsServers_,
            assignedIp_, repositoryCredentials_, securityOptions_,
            applicationId_, entryPoint_, hostName_,
            groupContainerName_, autoRemove_, runInteractive_,
           isContainerRoot_, healthConfig_, serviceName_,
            codePackageName_, servicePackageActivationId_, partitionId_);

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
        ServiceModel::ContainerIsolationMode::Enum isolationMode_;
        std::map<std::wstring, std::wstring> portBindings_;
        ServiceModel::LogConfigDescription logConfig_;
        std::vector<ServiceModel::ContainerVolumeDescription> volumes_;
        std::vector<std::wstring> dnsServers_;
        std::vector<std::wstring> securityOptions_;
        ServiceModel::RepositoryCredentialsDescription repositoryCredentials_;
        ServiceModel::ContainerHealthConfigDescription healthConfig_;
        bool autoRemove_;
        bool runInteractive_;
        bool isContainerRoot_;
        std::wstring codePackageName_;
        std::wstring servicePackageActivationId_;
        std::wstring partitionId_;
    };
}
