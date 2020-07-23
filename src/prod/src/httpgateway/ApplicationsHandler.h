// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class ApplicationsHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ApplicationsHandler)

    public:

        ApplicationsHandler(HttpGatewayImpl & server);
        virtual ~ApplicationsHandler();

        Common::ErrorCode Initialize();

        static Common::ErrorCode GetApplicationQueryDescription(
            Common::AsyncOperationSPtr const& thisSPtr,
            bool includeApplicationName,
            __out ServiceModel::ApplicationQueryDescription & queryDescription);

    private:

        //
        // Note: GetHealth is supported as a GET as well as a POST. When the request issued as a GET the default health
        // policy will be used.
        //

        void GetAllApplications(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllApplicationsComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetApplicationById(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetApplicationByIdComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetPartitionByPartitionId(Common::AsyncOperationSPtr const& thisSPtr);

        void CreateApplication(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateApplicationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void DeleteApplication(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteApplicationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void UpgradeApplication(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpgradeApplicationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void UpdateApplicationUpgrade(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateApplicationUpgradeComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void RollbackApplicationUpgrade(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRollbackApplicationUpgradeComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void EvaluateApplicationHealth(Common::AsyncOperationSPtr const& thisSptr);
        void EvaluateApplicationHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetUpgradeProgress(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetUpgradeProgressComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void MoveNextUpgradeDomain(Common::AsyncOperationSPtr const& thisSPtr);
        void OnMoveNextUpgradeDomainComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetAllPartitions(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllPartitionsComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetPartitionByIdAndServiceName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetPartitionByIdAndServiceNameComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void EvaluatePartitionHealth(Common::AsyncOperationSPtr const& thisSptr);
        void EvaluatePartitionHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void RecoverAllPartitions(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRecoverAllPartitionsComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void RecoverPartition(Common::AsyncOperationSPtr const& thisSPtr);
        void OnRecoverPartitionComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ResetPartitionLoad(Common::AsyncOperationSPtr const& thisSPtr);
        void OnResetPartitionLoadComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetAllReplicas(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllReplicasComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetReplicaById(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetReplicaByIdComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void EvaluateReplicaHealth(Common::AsyncOperationSPtr const& thisSptr);
        void EvaluateReplicaHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void GetAllServices(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllServicesComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceById(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetServiceByIdComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void CreateServiceFromTemplate(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateServiceFromTemplateComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);
        void CreateServiceGroupFromTemplate(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateServiceGroupFromTemplateComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void CreateService(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateServiceComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void CreateServiceGroup(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCreateServiceGroupComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void UpdateService(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateServiceComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void UpdateServiceGroup(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUpdateServiceGroupComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void DeleteService(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteServiceComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void DeleteServiceGroup(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteServiceGroupComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceDescription(Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceDescriptionComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceGroupDescription(Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceGroupDescriptionComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceGroupMembers(Common::AsyncOperationSPtr const& thisSPtr);
        void OnServiceGroupMembersComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void EvaluateServiceHealth(Common::AsyncOperationSPtr const& thisSptr);
        void EvaluateServiceHealthComplete(Common::AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously);

        void ResolveServicePartition(Common::AsyncOperationSPtr const& thisSPtr);
        void OnResolveServicePartitionComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void ReportApplicationHealth(Common::AsyncOperationSPtr const& thisSptr);
        void ReportServiceHealth(Common::AsyncOperationSPtr const& thisSptr);
        void ReportServicePartitionHealth(Common::AsyncOperationSPtr const& thisSptr);
        void ReportServicePartitionReplicaHealth(Common::AsyncOperationSPtr const& thisSptr);

        void GetApplicationApiHandlers(std::vector<HandlerUriTemplate> &handlerUris);
        void GetServiceApiHandlers(std::vector<HandlerUriTemplate> &handlerUris);
        void GetPartitionApiHandlers(std::vector<HandlerUriTemplate> &handlerUris);

        void GetPartitionLoadInformation(Common::AsyncOperationSPtr const& thisSPtr);
        void GetPartitionLoadInformationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetReplicaLoadInformation(Common::AsyncOperationSPtr const& thisSPtr);
        void GetReplicaLoadInformationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetUnplacedReplicaInformation(Common::AsyncOperationSPtr const& thisSPtr);
        void GetUnplacedReplicaInformationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void StartPartitionDataLoss(Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartPartitionDataLossComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);
        void StartPartitionQuorumLoss(Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartPartitionQuorumLossComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);
        void StartPartitionRestart(Common::AsyncOperationSPtr const& thisSPtr);
        void OnStartPartitionRestartComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetServiceName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetServiceNameComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetApplicationName(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetApplicationNameComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        void GetApplicationLoadInformation(Common::AsyncOperationSPtr const& thisSPtr);
        void GetApplicationLoadInformationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

#if !defined(PLATFORM_UNIX)
        void PerformBackupRestoreOperation(Common::AsyncOperationSPtr const& thisSPtr);
        void OnPerformBackupRestoreOperationComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);
#endif

        void GetAllApplicationNetworks(Common::AsyncOperationSPtr const& thisSPtr);
        void OnGetAllApplicationNetworksComplete(Common::AsyncOperationSPtr const& operation, __in bool expectedCompletedSynchronously);

        class CreateServiceFromTemplateData : public Common::IFabricJsonSerializable
        {
        public:
            CreateServiceFromTemplateData(std::wstring const& appName)
                : appName_(appName)
                , servicePackageActivationMode_(ServiceModel::ServicePackageActivationMode::SharedProcess)
            {
            };

            __declspec (property(get=get_ApplicationName)) std::wstring const& ApplicationName;
            std::wstring const& get_ApplicationName() const { return appName_; }

            __declspec (property(get=get_ServiceName)) std::wstring const& ServiceName;
            std::wstring const& get_ServiceName() const { return serviceName_; }

            __declspec (property(get = get_ServiceDnsName)) std::wstring const& ServiceDnsName;
            std::wstring const& get_ServiceDnsName() const { return serviceDnsName_; }

            __declspec (property(get=get_ServiceType)) std::wstring const& ServiceType;
            std::wstring const& get_ServiceType() const { return serviceType_; }

            __declspec (property(get = get_PackageActivationMode)) ServiceModel::ServicePackageActivationMode::Enum PackageActivationMode;
            ServiceModel::ServicePackageActivationMode::Enum get_PackageActivationMode() const { return servicePackageActivationMode_; }

            __declspec (property(get=get_InitData)) Common::ByteBuffer const& InitData;
            Common::ByteBuffer const& get_InitData() const { return initData_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationName, appName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, serviceName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceTypeName, serviceType_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::InitializationData, initData_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ServicePackageActivationMode, servicePackageActivationMode_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceDnsName, serviceDnsName_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring appName_;
            std::wstring serviceName_;
            std::wstring serviceDnsName_;
            std::wstring serviceType_;
            Common::ByteBuffer initData_;
            ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode_;
        };

        class CreateServiceGroupFromTemplateData : public Common::IFabricJsonSerializable
        {
        public:
            CreateServiceGroupFromTemplateData(std::wstring const& appName)
                : appName_(appName)
                , servicePackageActivationMode_(ServiceModel::ServicePackageActivationMode::SharedProcess)
            {
            };

            __declspec (property(get = get_ApplicationName)) std::wstring const& ApplicationName;
            std::wstring const& get_ApplicationName() const { return appName_; }

            __declspec (property(get = get_ServiceName)) std::wstring const& ServiceName;
            std::wstring const& get_ServiceName() const { return serviceName_; }

            __declspec (property(get = get_ServiceType)) std::wstring const& ServiceType;
            std::wstring const& get_ServiceType() const { return serviceType_; }

            __declspec (property(get = get_PackageActivationMode)) ServiceModel::ServicePackageActivationMode::Enum PackageActivationMode;
            ServiceModel::ServicePackageActivationMode::Enum get_PackageActivationMode() const { return servicePackageActivationMode_; }

            __declspec (property(get = get_InitData)) Common::ByteBuffer const& InitData;
            Common::ByteBuffer const& get_InitData() const { return initData_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationName, appName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, serviceName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceTypeName, serviceType_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::InitializationData, initData_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ServicePackageActivationMode, servicePackageActivationMode_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring appName_;
            std::wstring serviceName_;
            std::wstring serviceType_;
            Common::ByteBuffer initData_;
            ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode_;
        };

        //
        // Gives the next upgrade domain the user wants to upgrade
        //
        class MoveNextUpgradeDomainData : public Common::IFabricJsonSerializable
        {
        public:
            MoveNextUpgradeDomainData() {};

            __declspec(property(get=get_NextUpgradeDomain)) std::wstring &NextUpgradeDomain;
            std::wstring const& get_NextUpgradeDomain() const { return nextUpgradeDomain_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UpgradeDomainName, nextUpgradeDomain_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring nextUpgradeDomain_;
        };

        class PreviousRspVersionData : public Common::IFabricJsonSerializable
        {
        public:
            PreviousRspVersionData() {};

            __declspec(property(get=get_Version)) std::wstring &Version;
            std::wstring const& get_Version() const { return previousRspVersion_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PreviousRspVersion, previousRspVersion_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring previousRspVersion_;
        };

        class StartPartitionRestartData : public Common::IFabricJsonSerializable
        {
        public:
            StartPartitionRestartData() {};

            __declspec(property(get = get_OperationId)) std::wstring const& OperationId;
            std::wstring const& get_OperationId() const { return operationId_; }

            __declspec(property(get = get_PartitionSelectorType)) FABRIC_PARTITION_SELECTOR_TYPE const& PartitionSelectorType;
            FABRIC_PARTITION_SELECTOR_TYPE const& get_PartitionSelectorType() const { return partitionSelectorType_; }

            __declspec(property(get = get_PartitionKey)) std::wstring const& PartitionKey;
            std::wstring const& get_PartitionKey() const { return partitionKey_; }

            __declspec(property(get = get_RestartPartitionMode)) FABRIC_RESTART_PARTITION_MODE const& RestartPartitionMode;
            FABRIC_RESTART_PARTITION_MODE const& get_RestartPartitionMode() const { return restartPartitionMode_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::OperationId, operationId_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::PartitionSelectorType, partitionSelectorType_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionKey, partitionKey_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::RestartPartitionMode, restartPartitionMode_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring operationId_;

            //Management::FaultAnalysisService::PartitionSelectorType::Enum partitionSelectorType_;
            FABRIC_PARTITION_SELECTOR_TYPE partitionSelectorType_;

            // partition key
            std::wstring partitionKey_;

            //Management::FaultAnalysisService::RestartPartitionMode::Enum restartPartitionMode_;
            FABRIC_RESTART_PARTITION_MODE restartPartitionMode_;
        };

        class StartPartitionQuorumLossData : public Common::IFabricJsonSerializable
        {
        public:
            StartPartitionQuorumLossData() {};

            __declspec(property(get = get_OperationId)) std::wstring const& OperationId;
            std::wstring const& get_OperationId() const { return operationId_; }

            __declspec(property(get = get_PartitionSelectorType)) FABRIC_PARTITION_SELECTOR_TYPE & PartitionSelectorType;
            FABRIC_PARTITION_SELECTOR_TYPE get_PartitionSelectorType() const { return partitionSelectorType_; }

            __declspec(property(get = get_PartitionKey)) std::wstring const& PartitionKey;
            std::wstring const& get_PartitionKey() const { return partitionKey_; }

            __declspec(property(get = get_QuorumLossMode)) FABRIC_QUORUM_LOSS_MODE const& QuorumLossMode;
            FABRIC_QUORUM_LOSS_MODE const& get_QuorumLossMode() const { return quorumLossMode_; }

            __declspec(property(get = get_QuorumLossDurationInSeconds)) DWORD QuorumLossDurationInSeconds;
            DWORD get_QuorumLossDurationInSeconds() const { return quorumLossDurationInSeconds_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::OperationId, operationId_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::PartitionSelectorType, partitionSelectorType_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionKey, partitionKey_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::QuorumLossMode, quorumLossMode_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::QuorumLossDurationInSeconds, quorumLossDurationInSeconds_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring operationId_;

            //Management::FaultAnalysisService::PartitionSelectorType::Enum partitionSelectorType_;
            FABRIC_PARTITION_SELECTOR_TYPE partitionSelectorType_;

            // partition key
            std::wstring partitionKey_;

            //Management::FaultAnalysisService::QuorumLossMode::Enum quorumLossMode_;
            FABRIC_QUORUM_LOSS_MODE quorumLossMode_;

            DWORD quorumLossDurationInSeconds_;
        };

        class StartPartitionDataLossData : public Common::IFabricJsonSerializable
        {
        public:
            StartPartitionDataLossData() {};


            __declspec(property(get = get_OperationId)) std::wstring const& OperationId;
            std::wstring const& get_OperationId() const { return operationId_; }

#if 0
            __declspec(property(get = get_PartitionSelectorType)) Management::FaultAnalysisService::PartitionSelectorType::Enum const& PartitionSelectorType;
            Management::FaultAnalysisService::PartitionSelectorType::Enum const& get_PartitionSelectorType() const { return partitionSelectorType_; }
#endif
            __declspec(property(get = get_PartitionSelectorType)) FABRIC_PARTITION_SELECTOR_TYPE & PartitionSelectorType;
            FABRIC_PARTITION_SELECTOR_TYPE get_PartitionSelectorType() const { return partitionSelectorType_; }

            __declspec(property(get = get_PartitionKey)) std::wstring const& PartitionKey;
            std::wstring const& get_PartitionKey() const { return partitionKey_; }

#if 0
            __declspec(property(get = get_DataLossMode)) Management::FaultAnalysisService::DataLossMode::Enum const& DataLossMode;
            Management::FaultAnalysisService::DataLossMode::Enum const& get_DataLossMode() const { return dataLossMode_; }
#endif
            __declspec(property(get = get_DataLossMode)) FABRIC_DATA_LOSS_MODE const& DataLossMode;
            FABRIC_DATA_LOSS_MODE const& get_DataLossMode() const { return dataLossMode_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::OperationId, operationId_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::PartitionSelectorType, partitionSelectorType_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PartitionKey, partitionKey_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::DataLossMode, dataLossMode_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring operationId_;

            //Management::FaultAnalysisService::PartitionSelectorType::Enum partitionSelectorType_;
            FABRIC_PARTITION_SELECTOR_TYPE partitionSelectorType_;

            // partition key
            std::wstring partitionKey_;

        //    Management::FaultAnalysisService::DataLossMode::Enum dataLossMode_;
            FABRIC_DATA_LOSS_MODE dataLossMode_;
        };
    };
}
