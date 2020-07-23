// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessApplicationUpgradeContextAsyncOperation : 
            public ProcessUpgradeContextAsyncOperationBase<ApplicationUpgradeContext, Reliability::UpgradeApplicationRequestMessageBody>
        {
        public:

            // ***************************************************************
            // Upgrades are processed slightly differently from other requests
            // (CreateApplication, CreateService, etc.). An upgrade operation
            // will complete the client request once the operation has been
            // accepted (i.e. will failover correctly) by the CM.
            //
            // The upgrade operation will then be processed in the background
            // by the CM. The client is expected to poll the CM for the upgrade
            // status.
            // ***************************************************************
            //
            ProcessApplicationUpgradeContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationUpgradeContext &,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            
            ProcessApplicationUpgradeContextAsyncOperation(
                __in RolloutManager &,
                __in ApplicationUpgradeContext &,
                string const & traceComponent,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            void OnStart(Common::AsyncOperationSPtr const &) override;

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

        protected:

            void OnCompleted() override;
            virtual Reliability::RSMessage const & GetUpgradeMessageTemplate();
            virtual Common::TimeSpan GetUpgradeStatusPollInterval();
            virtual Common::TimeSpan GetHealthCheckInterval();

            virtual Common::ErrorCode LoadRollforwardMessageBody(__out Reliability::UpgradeApplicationRequestMessageBody &);
            virtual Common::ErrorCode LoadRollbackMessageBody(__out Reliability::UpgradeApplicationRequestMessageBody &);

            virtual void TraceQueryableUpgradeStart();
            virtual void TraceQueryableUpgradeDomainComplete(std::vector<std::wstring> & recentlyCompletedUDs);
            virtual std::wstring GetTraceAnalysisContext();
            virtual bool Get_SkipRollbackUpdateDefaultService() { return ManagementConfig::GetConfig().SkipRollbackUpdateDefaultService; }
            virtual bool Get_EnableDefaultServicesUpgrade() { return ManagementConfig::GetConfig().EnableDefaultServicesUpgrade; }
            
            virtual Common::AsyncOperationSPtr BeginInitializeUpgrade(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndInitializeUpgrade(Common::AsyncOperationSPtr const &);

            virtual Common::AsyncOperationSPtr BeginOnValidationError(Common::AsyncCallback const &, Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndOnValidationError(Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode OnFinishStartUpgrade(Store::StoreTransaction const &) { return Common::ErrorCodeValue::Success; }
            virtual Common::ErrorCode OnProcessFMReply(Store::StoreTransaction const &) { return Common::ErrorCodeValue::Success; }
            virtual Common::ErrorCode OnRefreshUpgradeContext(Store::StoreTransaction const &) override;

            virtual void FinalizeUpgrade(Common::AsyncOperationSPtr const &) override;
            virtual void FinalizeRollback(Common::AsyncOperationSPtr const &) override;
            virtual Common::ErrorCode OnFinishUpgrade(Store::StoreTransaction const &) { return Common::ErrorCodeValue::Success; }
            virtual Common::ErrorCode OnStartRollback(Store::StoreTransaction const &) { return Common::ErrorCodeValue::Success; }
            virtual Common::ErrorCode OnFinishRollback(Store::StoreTransaction const &) { return Common::ErrorCodeValue::Success; }

            virtual Common::AsyncOperationSPtr BeginUpdateHealthPolicyForHealthCheck(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndUpdateHealthPolicy(Common::AsyncOperationSPtr const &);

            virtual Common::ErrorCode EvaluateHealth(
                Common::AsyncOperationSPtr const &,
                __out bool & isHealthy, 
                __out std::vector<ServiceModel::HealthEvaluation> &);

            virtual Common::ErrorCode LoadVerifiedUpgradeDomains(
                __in Store::StoreTransaction &,
                __out std::vector<std::wstring> &);
            virtual Common::ErrorCode UpdateVerifiedUpgradeDomains(
                __in Store::StoreTransaction &,
                std::vector<std::wstring> &&);

        private:
            Common::AsyncOperationSPtr BeginUpdateHealthPolicy(
                ServiceModel::ApplicationHealthPolicy const &, 
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        private:

            //
            // Upgrade success
            //

            void StartSendRequest(Common::AsyncOperationSPtr const &) override;
            void OnPrepareUpdateDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void UpdateDefaultServices(Common::AsyncOperationSPtr const &);
            void OnUpdateDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            Common::ErrorCode ScheduleUpdateDefaultService(Common::AsyncOperationSPtr const &, size_t operationIndex, bool isRollback, ParallelOperationsCompletedCallback const &);
            Common::AsyncOperationSPtr BeginUpdateDefaultService(Common::AsyncOperationSPtr const &, size_t operationIndex, bool isRollback, Common::AsyncCallback const &);
            void EndUpdateDefaultService(Common::AsyncOperationSPtr const &, ParallelOperationsCompletedCallback const &);

            void PrepareRequestToFM(Common::AsyncOperationSPtr const &);
            void OnPrepareRequestToFMComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void OnUpdateDefaultServicesFailureComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void OnPrepareCreateDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void CreateDefaultServices(Common::AsyncOperationSPtr const &);
            void OnCreateDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            Common::ErrorCode ScheduleCreateService(Common::AsyncOperationSPtr const &, size_t operationIndex, ParallelOperationsCompletedCallback const &);
            Common::AsyncOperationSPtr BeginCreateService(Common::AsyncOperationSPtr const &, size_t operationIndex, Common::AsyncCallback const &);
            void EndCreateService(Common::AsyncOperationSPtr const &, ParallelOperationsCompletedCallback const &);

            void PrepareDeleteDefaultServices(Common::AsyncOperationSPtr const &);
            Common::ErrorCode ScheduleDeleteService(Common::AsyncOperationSPtr const &, size_t operationIndex, bool isRollback, ParallelOperationsCompletedCallback const &);
            Common::AsyncOperationSPtr BeginDeleteService(Common::AsyncOperationSPtr const &, size_t operationIndex, bool isRollback, Common::AsyncCallback const &);
            void EndDeleteService(Common::AsyncOperationSPtr const &, ParallelOperationsCompletedCallback const &);
            void OnPrepareDeleteDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void DeleteDefaultServices(Common::AsyncOperationSPtr const &);
            void OnDeleteDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void CommitDeleteDefaultServices(Common::AsyncOperationSPtr const &);
            void OnCommitDeleteDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void PrepareFinishUpgrade(Common::AsyncOperationSPtr const &);
            void OnPrepareFinishUpgradeComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void FinishUpgrade(Common::AsyncOperationSPtr const &);
            void OnFinishUpgradeCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void TrySchedulePrepareFinishUpgrade(Common::ErrorCode const &, Common::AsyncOperationSPtr const &);
            Common::ErrorCode FinishUpgradeServiceTemplates(Store::StoreTransaction const &);
            Common::ErrorCode FinishUpgradeServicePackages(Store::StoreTransaction const &);
            Common::ErrorCode FinishDeleteGoalState(Store::StoreTransaction const &);
            Common::ErrorCode FinishCreateDefaultServices(Store::StoreTransaction const &);
            Common::ErrorCode FinishDeleteDefaultServices(Store::StoreTransaction const &);

        private:

            //
            // Upgrade rollback
            //

            void PrepareStartRollback(Common::AsyncOperationSPtr const &);
            void OnPrepareStartRollbackComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void StartRollback(Common::AsyncOperationSPtr const &);
            void OnStartRollbackCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void StartImageBuilderRollback(Common::AsyncOperationSPtr const &);
            void OnImageBuilderRollbackComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void RemoveCreatedDefaultServices(Common::AsyncOperationSPtr const &);
            void OnRemoveCreatedDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void CommitRemoveCreatedDefaultServices(Common::AsyncOperationSPtr const &);
            void OnCommitRemoveCreatedDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void OnPrepareRollbackUpdatedDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void RollbackUpdatedDefaultServices(Common::AsyncOperationSPtr const &);
            void OnRollbackUpdatedDefaultServicesComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void OnRollbackUpdatedDefaultServicesFailureComplete(Common::ErrorCode const &, Common::AsyncOperationSPtr const &, bool);
            void PrepareFinishRollback(Common::AsyncOperationSPtr const &);
            void OnPrepareFinishRollbackComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void FinishRollback(Common::AsyncOperationSPtr const &);
            void OnFinishRollbackCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void TryScheduleFinishRollback(Common::ErrorCode const &, Common::AsyncOperationSPtr const &);
            Common::ErrorCode FinishRollbackServicePackages(Store::StoreTransaction const &);
            bool TryAcceptGoalStateUpgrade(Common::AsyncOperationSPtr const &, __in Store::StoreTransaction &);
            void OnFinishAcceptGoalStateUpgrade(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        protected:
            class InitializeUpgradeAsyncOperation : public AsyncOperation
            {
            public:
                InitializeUpgradeAsyncOperation(
                    __in ProcessApplicationUpgradeContextAsyncOperation & owner,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);

                void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
                static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

            protected:
                void ValidateServices(AsyncOperationSPtr const & thisSPtr);
                ProcessApplicationUpgradeContextAsyncOperation & owner_;

            private:
                void InitializeUpgrade(Common::AsyncOperationSPtr const & thisSPtr);
                virtual void OnLoadApplicationDescriptionsComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

                void OnValidateServicesComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
                void ValidateNetworks(Common::AsyncOperationSPtr const & thisSPtr);
                void OnValidateNetworksComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
                void ValidateActiveDefaultServices(Common::AsyncOperationSPtr const & thisSPtr);

                void LoadActiveDefaultServiceUpdates(Common::AsyncOperationSPtr const & thisSPtr);
                ErrorCode ScheduleLoadActiveDefaultService(
                    Common::AsyncOperationSPtr const & parallelAsyncOperation,
                    size_t operationIndex,
                    ParallelOperationsCompletedCallback const & callback);
                Common::AsyncOperationSPtr BeginLoadActiveDefaultService(
                    Common::AsyncOperationSPtr const & parallelAsyncOperation,
                    size_t operationIndex,
                    Common::AsyncCallback const & jobQueueCallback);
                void EndLoadActiveDefaultService(
                    Common::AsyncOperationSPtr const & operation,
                    size_t operationIndex,
                    ParallelOperationsCompletedCallback const & callback);
                void OnLoadActiveDefaultServiceUpdatesComplete(
                    Common::AsyncOperationSPtr const & operation,
                    bool expectedCompletedSynchronously);
                void InitializeHealthPolicies(Common::AsyncOperationSPtr const & thisSPtr);
                void OnUpdateHealthPolicyComplete(
                    Common::AsyncOperationSPtr const & operation,
                    bool expectedCompletedSynchronously);

                RwLock upgradeContextLock_;
            };

        private:
            class ApplicationUpgradeParallelRetryableAsyncOperation;
            class ImageBuilderRollbackAsyncOperation;

            //
            // Misc. helpers
            //

            Common::ErrorCode CompleteApplicationContext(Store::StoreTransaction const &);

            Common::ErrorCode LoadApplicationContext();
            virtual Common::AsyncOperationSPtr BeginLoadApplicationDescriptions(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            virtual Common::ErrorCode EndLoadApplicationDescriptions(
                Common::AsyncOperationSPtr const &);
            bool IsNetworkValidationNeeded();
            Common::AsyncOperationSPtr BeginValidateNetworks(
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndValidateNetworks(
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode LoadUpgradePolicies();
            Common::ErrorCode LoadHealthPolicy(ServiceModelVersion const &, __out ServiceModel::ApplicationHealthPolicy &);
            Common::ErrorCode LoadActiveServices();
            Common::ErrorCode ValidateRemovedServiceTypes();
            Common::ErrorCode ValidateMovedServiceTypes(std::vector<StoreDataServicePackage> const &);
            void ComputeUnusedServiceTypes(
                std::set<ServiceModelTypeName> const & activeTypes, 
                std::vector<StoreDataServicePackage> const & upgradingTypes);
            Common::ErrorCode LoadDefaultServices();
            
            Common::ErrorCode LoadMessageBody(
                ServiceModel::ApplicationInstanceDescription const &,
                bool isMonitored,
                bool isManual,
                bool isRollback,
                std::vector<StoreDataServicePackage> const & packageUpgrades,
                std::vector<StoreDataServicePackage> const & removedTypes,
                DigestedApplicationDescription::CodePackageNameMap const &,
                Common::TimeSpan const replicaSetCheckTimeout,
                uint64 instanceId,
                std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageResourceGovernanceDescription> &&,
                std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::CodePackageContainersImagesDescription> &&,
                std::vector<std::wstring> &&,
                __out Reliability::UpgradeApplicationRequestMessageBody &);

            void AppendDynamicUpgradeStatusDetails(std::wstring const &);
            void SetDynamicUpgradeStatusDetails(std::wstring const &);

            static Common::NamingUri StringToNamingUri(std::wstring const & name);

        protected:
            std::unique_ptr<ApplicationContext> appContextUPtr_;
            DigestedApplicationDescription currentApplication_;
            DigestedApplicationDescription targetApplication_;
            std::vector<StoreDataServicePackage> removedTypes_;
            std::vector<ServiceModel::ServicePackageReference> addedPackages_;

            // Descriptions of active services in target manifest
            std::vector<Naming::PartitionedServiceDescriptor> activeDefaultServiceDescriptions_;

            // Only used for validation during upgrade preparation
            std::set<ServiceModelTypeName> activeServiceTypes_;
            std::map<Common::NamingUri, Naming::PartitionedServiceDescriptor> allActiveServices_;

            // Stores pre-upgrade health policies
            ServiceModel::ApplicationHealthPolicy manifestHealthPolicy_;

            // Used for saving the the currentManifestId for tracing  the completed upgrade
            std::wstring currentManifestId_;
        };
    }
}