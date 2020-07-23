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

        void UpgradeFabric(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpgradeFabricComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void UpdateFabricUpgrade(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateFabricUpgradeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void RollbackFabricUpgrade(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRollbackFabricUpgradeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ToggleVerboseServicePlacementHealthReporting(Common::AsyncOperationSPtr const& thisSptr);
        void OnToggleVerboseServicePlacementHealthReportingComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetUpgradeProgress(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetUpgradeProgressComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void MoveNextUpgradeDomain(Common::AsyncOperationSPtr const& thisSPtr);
        void OnMoveNextUpgradeDomainComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetClusterManifest(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterManifestComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetClusterVersion(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterVersionComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void EvaluateClusterHealth(Common::AsyncOperationSPtr const& thisSPtr);
        void OnEvaluateClusterHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void StartClusterConfigurationUpgrade(Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartClusterConfigurationUpgradeComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetClusterConfigurationUpgradeStatus(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterConfigurationUpgradeStatusComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetClusterConfiguration(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterConfigurationComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetUpgradesPendingApproval(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetUpgradesPendingApprovalComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void StartApprovedUpgrades(Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartApprovedUpgradesComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetUpgradeOrchestrationServiceState(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetUpgradeOrchestrationServiceStateComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

		void SetUpgradeOrchestrationServiceState(Common::AsyncOperationSPtr const& thisSPtr);
		void OnSetUpgradeOrchestrationServiceStateComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void EvaluateClusterHealthChunk(Common::AsyncOperationSPtr const& thisSPtr);
        void OnEvaluateClusterHealthChunkComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ProvisionFabric(Common::AsyncOperationSPtr const& thisSPtr);
        void OnProvisionComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void UnprovisionFabric(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUnprovisionComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetProvisionedFabricCodeVersions(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetProvisionedFabricCodeVersionsComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetProvisionedFabricConfigVersions(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetProvisionedFabricConfigVersionsComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetTokenValidationServiceMetadata(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetMetadataComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetAzureActiveDirectoryMetadata(Common::AsyncOperationSPtr const& thisSPtr);

        void RecoverPartitions(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRecoverPartitionsComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void RecoverSystemServicePartitions(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRecoverSystemServicePartitionsComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetClusterLoadInformation(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetClusterLoadInformationComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ReportClusterHealth(Common::AsyncOperationSPtr const& thisSPtr);

        void InvokeInfrastructureCommand(Common::AsyncOperationSPtr const& thisSPtr);
        void InvokeInfrastructureQuery(Common::AsyncOperationSPtr const& thisSPtr);
        void InnerInvokeInfrastructureService(bool isAdminCommand, Common::AsyncOperationSPtr const& thisSPtr);
        void OnInvokeInfrastructureServiceComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void CreateRepairTask(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateRepairTaskComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void CancelRepairTask(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancelRepairTaskComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void ForceApproveRepairTask(Common::AsyncOperationSPtr const& thisSPtr);
        void OnForceApproveRepairTaskComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void DeleteRepairTask(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteRepairTaskComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void UpdateRepairExecutionState(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateRepairExecutionStateComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void GetRepairTaskList(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetRepairTaskListComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void UpdateRepairTaskHealthPolicy(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateRepairTaskHealthPolicyComplete(Common::AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously);

        void GetGenericStaticFile(Common::AsyncOperationSPtr const& operation);

        void RedirectToExplorerIndexPage(Common::AsyncOperationSPtr const& operation);

        void GetRootPage(Common::AsyncOperationSPtr const& operation);

        void RespondWithContentFromFile(std::wstring const& path, Common::AsyncOperationSPtr const& operation);
        Common::ErrorCode ValidateFileName(std::wstring const& path, std::wstring const& fileExtension);

//#ifdef PLATFORM_UNIX
        Common::ErrorCode ReadFileToBufferSyncForLinux(std::wstring const& path, __out Common::ByteBufferUPtr& bufferUPtr);
//#else
        void OnRespondWithContentFromFileComplete(Common::AsyncOperationSPtr const& operation, std::wstring const &contentTypeValue);
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

        class ClusterVersion : public Common::IFabricJsonSerializable
        {
        public:
            ClusterVersion(std::wstring&& version)
                :version_(move(version))
            {
            }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Version, version_)
                END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring version_;
        };

        class ClusterConfiguration : public Common::IFabricJsonSerializable
        {
        public:
            ClusterConfiguration(std::wstring&& clusterConfiguration)
                :clusterConfiguration_(std::move(clusterConfiguration))
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

    };
}
