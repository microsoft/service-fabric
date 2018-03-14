// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // REST Handler for FabricClient API's 
    // *IFabricClusterManagementCLient
    //
    class ClusterManagementHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ClusterManagementHandler)

    public:

        ClusterManagementHandler(HttpGatewayImpl & server);
        virtual ~ClusterManagementHandler();

        Common::ErrorCode Initialize();

    private:

        void UpgradeFabric(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpgradeFabricComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void UpdateFabricUpgrade(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateFabricUpgradeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void RollbackFabricUpgrade(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnRollbackFabricUpgradeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ToggleVerboseServicePlacementHealthReporting(__in Common::AsyncOperationSPtr const& thisSptr);
        void OnToggleVerboseServicePlacementHealthReportingComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetUpgradeProgress(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetUpgradeProgressComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void MoveNextUpgradeDomain(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnMoveNextUpgradeDomainComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetClusterManifest(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterManifestComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void EvaluateClusterHealth(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnEvaluateClusterHealthComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void StartClusterConfigurationUpgrade(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartClusterConfigurationUpgradeComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetClusterConfigurationUpgradeStatus(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterConfigurationUpgradeStatusComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetClusterConfiguration(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterConfigurationComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetUpgradesPendingApproval(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetUpgradesPendingApprovalComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void StartApprovedUpgrades(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartApprovedUpgradesComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetUpgradeOrchestrationServiceState(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetUpgradeOrchestrationServiceStateComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

		void SetUpgradeOrchestrationServiceState(__in Common::AsyncOperationSPtr const& thisSPtr);
		void OnSetUpgradeOrchestrationServiceStateComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void EvaluateClusterHealthChunk(Common::AsyncOperationSPtr const& thisSPtr);
        void OnEvaluateClusterHealthChunkComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ProvisionFabric(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnProvisionComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void UnprovisionFabric(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnUnprovisionComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetProvisionedFabricCodeVersions(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetProvisionedFabricCodeVersionsComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetProvisionedFabricConfigVersions(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetProvisionedFabricConfigVersionsComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetTokenValidationServiceMetadata(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetMetadataComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetAzureActiveDirectoryMetadata(__in Common::AsyncOperationSPtr const& thisSPtr);
      
        void RecoverPartitions(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnRecoverPartitionsComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void RecoverSystemServicePartitions(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnRecoverSystemServicePartitionsComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetClusterLoadInformation(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterLoadInformationComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ReportClusterHealth(Common::AsyncOperationSPtr const& thisSPtr);

        void InvokeInfrastructureCommand(__in Common::AsyncOperationSPtr const& thisSPtr);
        void InvokeInfrastructureQuery(__in Common::AsyncOperationSPtr const& thisSPtr);
        void InnerInvokeInfrastructureService(bool isAdminCommand, __in Common::AsyncOperationSPtr const& thisSPtr);
        void OnInvokeInfrastructureServiceComplete(__in Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void CreateRepairTask(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateRepairTaskComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void CancelRepairTask(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancelRepairTaskComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void ForceApproveRepairTask(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnForceApproveRepairTaskComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void DeleteRepairTask(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteRepairTaskComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void UpdateRepairExecutionState(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateRepairExecutionStateComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void GetRepairTaskList(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetRepairTaskListComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void UpdateRepairTaskHealthPolicy(__in Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateRepairTaskHealthPolicyComplete(__in Common::AsyncOperationSPtr const &operation, __in bool expectedCompletedSynchronously);

        void GetGenericStaticFile(__in Common::AsyncOperationSPtr const& operation);

        void RedirectToExplorerIndexPage(__in Common::AsyncOperationSPtr const& operation);

        void GetRootPage(__in Common::AsyncOperationSPtr const& operation);

        void RespondWithContentFromFile(__in std::wstring const& path, __in Common::AsyncOperationSPtr const& operation);
        Common::ErrorCode ValidateFileName(__in std::wstring const& path, __in std::wstring const& fileExtension);

//#ifdef PLATFORM_UNIX
        Common::ErrorCode ReadFileToBufferSyncForLinux(__in std::wstring const& path, __out Common::ByteBufferUPtr& bufferUPtr);
//#else
        void OnRespondWithContentFromFileComplete(__in Common::AsyncOperationSPtr const& operation, __in std::wstring const &contentTypeValue);
//#endif

        void InitializeContentTypes();
        typedef std::map<std::wstring, Common::GlobalWString> ContentTypeMap; // Maps extensions to MIME content types
        ContentTypeMap contentTypes;

        //
        // Gives the next upgrade domain the user wants to upgrade
        //
        class MoveNextUpgradeDomainData : public Common::IFabricJsonSerializable
        {
        public:
            MoveNextUpgradeDomainData() {};

            __declspec(property(get = get_NextUpgradeDomain)) std::wstring &NextUpgradeDomain;
            std::wstring const& get_NextUpgradeDomain() const { return nextUpgradeDomain_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDomain, nextUpgradeDomain_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring nextUpgradeDomain_;
        };

        class Manifest : public Common::IFabricJsonSerializable
        {
        public:
            Manifest(std::wstring&& manifest)
                :manifest_(move(manifest))
            {}

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Manifest, manifest_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring manifest_;
        };

        class ClusterConfiguration : public Common::IFabricJsonSerializable
        {
        public:
            ClusterConfiguration(std::wstring&& clusterConfiguration)
                :clusterConfiguration_(move(clusterConfiguration))
            {}

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ClusterConfiguration, clusterConfiguration_)
                END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring clusterConfiguration_;
        };

        class UpgradeOrchestrationServiceStateString : public Common::IFabricJsonSerializable
        {
        public:
			UpgradeOrchestrationServiceStateString()
				: serviceState_()
			{}

			UpgradeOrchestrationServiceStateString(std::wstring&& serviceState)
                :serviceState_(move(serviceState))
            {}

			__declspec(property(get = get_ServiceState)) std::wstring & ServiceState;
			std::wstring const & get_ServiceState() const { return serviceState_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceState, serviceState_)
                END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring serviceState_;
        };

        class ProvisionFabricData : public Common::IFabricJsonSerializable
        {
        public:
            ProvisionFabricData() {};

            __declspec(property(get = get_CodeFilePath)) std::wstring &CodeFilePath;
            std::wstring const& get_CodeFilePath() const { return codeFilePath_; }

            __declspec(property(get = get_ClusterManifestFilePath)) std::wstring &ClusterManifestFilePath;
            std::wstring const& get_ClusterManifestFilePath() const { return clusterManifestFilePath_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CodeFilePath, codeFilePath_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ClusterManifestFilePath, clusterManifestFilePath_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring codeFilePath_;
            std::wstring clusterManifestFilePath_;
        };

        class UnprovisionFabricData : public Common::IFabricJsonSerializable
        {
        public:
            UnprovisionFabricData() {};

            __declspec(property(get = get_CodeVersion)) std::wstring &CodeVersion;
            std::wstring const& get_CodeVersion() const { return codeVersion_; }

            __declspec(property(get = get_ConfigVersion)) std::wstring &ConfigVersion;
            std::wstring const& get_ConfigVersion() const { return configVersion_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::CodeVersion, codeVersion_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ConfigVersion, configVersion_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring codeVersion_;
            std::wstring configVersion_;
        };

        class CommitRepairTaskResponse : public Common::IFabricJsonSerializable
        {
        public:
            CommitRepairTaskResponse(int64 commitVersion)
                : commitVersion_(commitVersion)
            {
            }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Version, commitVersion_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            int64 commitVersion_;
        };

        class StartUpgradeData : public Common::IFabricJsonSerializable
        {
        public:
            StartUpgradeData()
                : maxPercentUnhealthyApplications_(0),
                maxPercentUnhealthyNodes_(0),
                maxPercentDeltaUnhealthyNodes_(0),
                maxPercentUpgradeDomainDeltaUnhealthyNodes_(0)
            {};

            __declspec(property(get = get_ClusterConfig)) std::wstring const& ClusterConfig;
            std::wstring const& get_ClusterConfig() const { return clusterConfig_; }

            __declspec(property(get = get_HealthCheckRetryTimeout)) Common::TimeSpan const HealthCheckRetryTimeout;
            Common::TimeSpan const get_HealthCheckRetryTimeout() const { return healthCheckRetryTimeout_; }

            __declspec(property(get = get_HealthCheckWaitDurationInSeconds)) Common::TimeSpan const HealthCheckWaitDurationInSeconds;
            Common::TimeSpan const get_HealthCheckWaitDurationInSeconds() const { return healthCheckWaitDuration_; }

            __declspec(property(get = get_HealthCheckStableDurationInSeconds)) Common::TimeSpan const HealthCheckStableDurationInSeconds;
            Common::TimeSpan const get_HealthCheckStableDurationInSeconds() const { return healthCheckStableDuration_; }

            __declspec(property(get = get_UpgradeDomainTimeoutInSeconds)) Common::TimeSpan const UpgradeDomainTimeoutInSeconds;
            Common::TimeSpan const get_UpgradeDomainTimeoutInSeconds() const { return upgradeDomainTimeout_; }

            __declspec(property(get = get_UpgradeTimeoutInSeconds)) Common::TimeSpan const UpgradeTimeoutInSeconds;
            Common::TimeSpan const get_UpgradeTimeoutInSeconds() const { return upgradeTimeout_; }

            __declspec(property(get = get_MaxPercentUnhealthyApplications)) BYTE MaxPercentUnhealthyApplications;
            inline BYTE get_MaxPercentUnhealthyApplications() const { return maxPercentUnhealthyApplications_; }

            __declspec(property(get = get_MaxPercentUnhealthyNodes)) BYTE MaxPercentUnhealthyNodes;
            inline BYTE get_MaxPercentUnhealthyNodes() const { return maxPercentUnhealthyNodes_; }

            __declspec(property(get = get_MaxPercentDeltaUnhealthyNodes)) BYTE MaxPercentDeltaUnhealthyNodes;
            inline BYTE get_MaxPercentDeltaUnhealthyNodes() const { return maxPercentDeltaUnhealthyNodes_; }

            __declspec(property(get = get_MaxPercentUpgradeDomainDeltaUnhealthyNodes)) BYTE MaxPercentUpgradeDomainDeltaUnhealthyNodes;
            inline BYTE get_MaxPercentUpgradeDomainDeltaUnhealthyNodes() const { return maxPercentUpgradeDomainDeltaUnhealthyNodes_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ClusterConfig, clusterConfig_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthCheckRetryTimeout, healthCheckRetryTimeout_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthCheckWaitDurationInSeconds, healthCheckWaitDuration_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::HealthCheckStableDurationInSeconds, healthCheckStableDuration_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDomainTimeoutInSeconds, upgradeDomainTimeout_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeTimeoutInSeconds, upgradeTimeout_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxPercentUnhealthyApplications, maxPercentUnhealthyApplications_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxPercentUnhealthyNodes, maxPercentUnhealthyNodes_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxPercentDeltaUnhealthyNodes, maxPercentDeltaUnhealthyNodes_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxPercentUpgradeDomainDeltaUnhealthyNodes, maxPercentUpgradeDomainDeltaUnhealthyNodes_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring  clusterConfig_;
            Common::TimeSpan healthCheckRetryTimeout_;
            Common::TimeSpan healthCheckWaitDuration_;
            Common::TimeSpan healthCheckStableDuration_;
            Common::TimeSpan upgradeDomainTimeout_;
            Common::TimeSpan upgradeTimeout_;
            BYTE maxPercentUnhealthyApplications_;
            BYTE maxPercentUnhealthyNodes_;
            BYTE maxPercentDeltaUnhealthyNodes_;
            BYTE maxPercentUpgradeDomainDeltaUnhealthyNodes_;
        };
    };
}
