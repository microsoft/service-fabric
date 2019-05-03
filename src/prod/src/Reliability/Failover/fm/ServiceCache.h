// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        typedef std::shared_ptr<CacheEntry<ApplicationInfo>> ApplicationEntrySPtr;

        typedef std::function<bool(ServiceInfoSPtr const &)> const ServiceCacheFilter;

        class ServiceCache
        {
            DENY_COPY(ServiceCache);
        public:
            ServiceCache(
                FailoverManager& fm,
                FailoverManagerStore& fmStore,
                InstrumentedPLB & plb,
                std::vector<ApplicationInfoSPtr> const & applications,
                std::vector<ServiceTypeSPtr> const & serviceTypes,
                std::vector<ServiceInfoSPtr> const & services);

            void Close();

            void StartPeriodicTask();

            Common::ErrorCode GetServiceDescription(std::wstring const& serviceName, __out ServiceDescription & serviceDescription) const;

            Common::ErrorCode GetFMServiceForGFUMTransfer(__out ServiceInfoSPtr & serviceInfo) const;

            void GetServices(std::vector<ServiceInfoSPtr> & services) const;
            ServiceInfoSPtr GetService(std::wstring const& serviceName) const;
            bool ServiceExists(std::wstring const& serviceName) const;
            void GetServiceQueryResults(ServiceCacheFilter const & filter, __out std::vector<ServiceModel::ServiceQueryResult> & serviceResults) const;
            void GetServiceGroupMemberQueryResults(ServiceCacheFilter const & filter, __out std::vector<ServiceModel::ServiceGroupMemberQueryResult> & serviceGroupResults) const;

            void GetServiceTypes(std::vector<ServiceTypeSPtr> & serviceTypes) const;
            ServiceTypeSPtr GetServiceType(ServiceModel::ServiceTypeIdentifier const& serviceTypeId) const;

            void CleanupServiceTypesAndApplications();

            void CompleteServiceUpdateAsync(std::wstring const& serviceName, uint64 serviceInstance, uint64 updateVersion);

            // Removes this as well all previous instances of the node from block lists.
            Common::ErrorCode RemoveFromDisabledNodes(uint64 sequenceNumber, Federation::NodeId const& nodeId);

            void GetApplications(std::vector<ApplicationInfoSPtr> & applications) const;
            ApplicationInfoSPtr GetApplication(ServiceModel::ApplicationIdentifier const& applicationId) const;

            Common::ErrorCode CreateService(
                ServiceDescription && serviceDescription,
                std::vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
                bool isCreateFromRebuild,
                bool isServiceUpdateNeeded = false);

            Common::AsyncOperationSPtr BeginCreateService(
                ServiceDescription && serviceDescription,
                std::vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
                bool isCreateFromRebuild,
                bool isServiceUpdateNeeded,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);
            Common::ErrorCode EndCreateService(Common::AsyncOperationSPtr const& operation);

            Common::ErrorCode CreateServiceDuringRebuild(
                ServiceDescription && serviceDescription,
                std::vector<ConsistencyUnitDescription> && consistencyUnitDescriptions,
                bool isServiceUpdateNeeded = false);

            Common::ErrorCode CreateTombstoneService();

            Common::ErrorCode UpdateService(ServiceDescription && serviceDescription);
            void ProcessNodeUpdateServiceReply(NodeUpdateServiceReplyMessageBody const& body, Federation::NodeInstance const& from);

            Common::AsyncOperationSPtr BeginDeleteService(
                std::wstring const& serviceName,
                bool const isForce,
                uint64 serviceInstance,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);
            Common::ErrorCode EndDeleteService(Common::AsyncOperationSPtr const& operation);

            void MarkServiceAsToBeDeletedAsync(std::wstring const& serviceName, uint64 serviceInstance);
            void MarkServiceAsDeletedAsync(std::wstring const& serviceName, uint64 serviceInstance);

            Common::AsyncOperationSPtr BeginCreateApplication(
                ServiceModel::ApplicationIdentifier const& applicationId,
                uint64 instanceId,
                Common::NamingUri const & applicationName,
                uint64 updateId,
                ApplicationCapacityDescription const& applicationCapacity,
                ServiceModel::ServicePackageResourceGovernanceMap const& rgDescription,
                ServiceModel::CodePackageContainersImagesMap const& cpContainersImages,
                Common::StringCollection networks,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);

            Common::ErrorCode EndCreateApplication(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginDeleteApplication(
                ServiceModel::ApplicationIdentifier const& applicationId,
                uint64 instanceId,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);
            Common::ErrorCode EndDeleteApplication(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginProcessServiceTypeNotification(
                std::vector<ServiceTypeInfo> && incomingInfos,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndProcessServiceTypeNotification(
                Common::AsyncOperationSPtr const& operation,
                __out std::vector<ServiceTypeInfo> & processedInfos);

            Common::AsyncOperationSPtr BeginUpdateService(
                ServiceDescription && serviceDescription,
                std::vector<ConsistencyUnitDescription> && added,
                std::vector<ConsistencyUnitDescription> && removed,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndUpdateService(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginUpdateSystemService(
                std::wstring && serviceName,
                Naming::ServiceUpdateDescription && serviceUpdateDescription,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndUpdateSystemService(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginUpdateApplication(
                ServiceModel::ApplicationIdentifier const & applicationId,
                uint64 updateId,
                ApplicationCapacityDescription const& capacityDescription,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::AsyncOperationSPtr BeginUpdateApplication(
                LockedApplicationInfo && lockedAppInfo,
                ApplicationInfoSPtr && newAppInfo,
                bool isPlbUpdateNeeded,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndUpdateApplication(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginUpdateServiceTypes(
                std::vector<ServiceModel::ServiceTypeIdentifier> && serviceTypes,
                uint64 sequenceNumber,
                Federation::NodeId const& nodeId,
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndUpdateServiceTypes(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginRemoveFromDisabledNodes(
                uint64 sequenceNumber,
                Federation::NodeId const& nodeId,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndRemoveFromDisabledNodes(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginProcessPLBSafetyCheck(
                ServiceModel::ApplicationIdentifier && appId,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndProcessPLBSafetyCheck(
                Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginProcessUpgrade(
                UpgradeApplicationRequestMessageBody && requestBody,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndProcessUpgrade(
                Common::AsyncOperationSPtr const& operation,
                __out UpgradeApplicationReplyMessageBody & replyBody);

            void UpdateUpgradeProgressAsync(
                ApplicationInfo const& appInfo,
                std::map<Federation::NodeId, NodeUpgradeProgress> && pendingNodes,
                std::map<Federation::NodeId, std::pair<NodeUpgradeProgress, NodeInfoSPtr>> && readyNodes,
                std::map<Federation::NodeId, NodeUpgradeProgress> && waitingNodes,
                bool isUpgradeNeededOnCurrentUD,
                bool isCurrentDomainComplete);

            void CompleteUpgradeAsync(LockedApplicationInfo & lockedAppInfo);

            bool UpdateServicePackageVersionInstanceAsync(LockedApplicationInfo & lockedAppInfo);

            Common::ErrorCode ProcessUpgradeReply(ServiceModel::ApplicationIdentifier const & applicationId, uint64 instanceId, Federation::NodeInstance const & from);

            bool GetTargetVersion(
                ServiceModel::ServicePackageIdentifier const & packageId,
                std::wstring const & upgradeDomain,
                ServiceModel::ServicePackageVersionInstance & versionInstance) const;

            void AddServiceContexts() const;
            void AddApplicationUpgradeContexts(std::vector<BackgroundThreadContextUPtr> & contexts) const;

            Common::AsyncOperationSPtr BeginAddFailoverUnitId(
                std::wstring const& serviceName,
                FailoverUnitId const& failoverUnitId,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndAddFailoverUnitId(Common::AsyncOperationSPtr const& operation);

            void SetFailoverUnitIdsAsync(std::wstring const& serviceName, uint64 serviceInstance, std::set<FailoverUnitId> && failoverUnitIds);

            Common::ErrorCode IterateOverFailoverUnits(std::wstring const& serviceName, std::function<Common::ErrorCode(LockedFailoverUnitPtr &)> const& func) const;
            Common::ErrorCode IterateOverFailoverUnits(std::wstring const& serviceName, std::function<bool(FailoverUnitId const&)> const& filter, std::function<Common::ErrorCode(LockedFailoverUnitPtr &)> const& func) const;

            FABRIC_SEQUENCE_NUMBER GetNextHealthSequence();

            static ServiceDescription GetFMServiceDescription();
            static ServiceDescription GetTombstoneServiceDescription();

            void FinishRemoveService(LockedServiceInfo & lockedServiceInfo, int64 commitDuration);
            void FinishRemoveServiceType(LockedServiceType & lockedServiceType, int64 commitDuration);
            void FinishRemoveApplication(LockedApplicationInfo & lockedApplicationInfo, int64 commitDuration, int64 plbDuration);

            class CreateServiceAsyncOperation;
            class DeleteServiceAsyncOperation;
            class ProcessUpgradeAsyncOperation;
            class ProcessPLBSafetyCheckAsyncOperation;
            class RepartitionServiceAsyncOperation;

        private:

            class RemoveFromDisabledNodesAsyncOperation;
            class ServiceTypeNotificationAsyncOperation;
            class UpdateServiceTypesAsyncOperation;

            typedef std::shared_ptr<CacheEntry<ServiceInfo>> ServiceEntrySPtr;
            typedef std::shared_ptr<CacheEntry<ServiceType>> ServiceTypeEntrySPtr;
            ServiceInfoSPtr GetServiceCallerHoldsLock(std::wstring const& serviceName) const;
            ServiceTypeSPtr GetServiceTypeCallerHoldsLock(ServiceModel::ServiceTypeIdentifier const& serviceTypeId) const;
            ApplicationInfoSPtr GetApplicationCallerHoldsLock(ServiceModel::ApplicationIdentifier const& applicationId) const;

            void InternalUpdateServiceInfoCache(LockedServiceInfo & lockedServiceInfo, ServiceInfoSPtr && newServiceInfo, int64 commitDuration, int64 plbDuration);

            void PostUpdateApplication(
                LockedApplicationInfo & lockedApplication,
                ApplicationInfoSPtr && newApplication,
                int64 commitDuration,
                int64 plbDuration,
                Common::ErrorCode error,
                bool isPlbUpdated);

            void UpdateApplicationAsync(LockedApplicationInfo && lockedAppInfo, ApplicationInfoSPtr && newAppInfo, bool isPlbUpdateNeeded);

            Common::AsyncOperationSPtr BeginRepartitionService(
                ServiceDescription && serviceDescription,
                std::vector<ConsistencyUnitDescription> && added,
                std::vector<ConsistencyUnitDescription> && removed,
                LockedServiceInfo && lockedServiceInfo,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);
            Common::ErrorCode EndRepartitionService(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr BeginInternalUpdateServiceInfo(
                LockedServiceInfo && lockedServiceInfo,
                ServiceInfoSPtr && newServiceInfo,
                bool isPlbUpdateNeeded,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndInternalUpdateServiceInfo(Common::AsyncOperationSPtr const& operation);

            Common::AsyncOperationSPtr ServiceCache::BeginUpdateServiceType(
                ServiceTypeEntrySPtr const& serviceTypeEntry,
                uint64 sequenceNumber,
                Federation::NodeId const& nodeId,
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state);
            Common::ErrorCode EndUpdateServiceType(Common::AsyncOperationSPtr const& operation);

            Common::ErrorCode GetLockedService(std::wstring const& serviceName, __out LockedServiceInfo & lockedServiceInfo) const;
            Common::ErrorCode CreateOrGetLockedService(
                ServiceDescription const& serviceDescription,
                ServiceTypeSPtr const& serviceType,
                FABRIC_SEQUENCE_NUMBER healthSequence,
                bool isServiceUpdateNeeded,
                __out LockedServiceInfo & lockedServiceInfo,
                __out bool & isNewService);

            Common::ErrorCode GetLockedServiceType(ServiceModel::ServiceTypeIdentifier const& serviceTypeId, __out LockedServiceType & lockedServiceType) const;
            Common::ErrorCode CreateOrGetLockedServiceType(
                ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                ApplicationEntrySPtr const& applicationEntry,
                __out LockedServiceType & lockedServiceType,
                __out bool & isNewServiceType);

            Common::ErrorCode GetLockedApplication(ServiceModel::ApplicationIdentifier const& applicationId, __out LockedApplicationInfo & lockedApplication) const;
            Common::ErrorCode CreateOrGetLockedApplication(
                ServiceModel::ApplicationIdentifier const& applicationId,
                Common::NamingUri const& applicationName,
                uint64 instanceId,
                uint64 updateId,
                ApplicationCapacityDescription const& capacityDescription,
                ServiceModel::ServicePackageResourceGovernanceMap const& rgDescription,
                ServiceModel::CodePackageContainersImagesMap const& cpContainersImages,
                __out LockedApplicationInfo & lockedApplication,
                __out bool & isNewApplication);

            Common::ErrorCode PLBUpdateService(ServiceInfo const& serviceInfo, __out int64 & plbElapsedMilliseconds, bool forceUpdate = false);
            void PLBDeleteService(std::wstring const& serviceName, __out int64 & plbElapsedMilliseconds);

            void PLBUpdateServiceType(ServiceType const& serviceType, __out int64 & plbElapsedMilliseconds);
            void PLBDeleteServiceType(std::wstring const& serviceTypeName, __out int64 & plbElapsedMilliseconds);

            Common::ErrorCode PLBUpdateApplication(ApplicationInfo & applicationInfo, __out int64 & plbElapsedMilliseconds, bool forceUpdate = false);
            void PLBDeleteApplication(Common::NamingUri const& applicationName, __out int64 & plbElapsedMilliseconds, bool forceUpdate = false);

            void InitializeHealthState();
            void ReportServiceHealth(ServiceInfoSPtr const & serviceInfo, Common::ErrorCode error, FABRIC_SEQUENCE_NUMBER healthSequence = FABRIC_INVALID_SEQUENCE_NUMBER);

            void Trace(ServiceCounts counts) const;

            FailoverManager& fm_;
            FailoverManagerStore& fmStore_;
            InstrumentedPLB & plb_;

            std::map<std::wstring, ServiceEntrySPtr> services_;
            std::map<ServiceModel::ServiceTypeIdentifier, ServiceTypeEntrySPtr> serviceTypes_;
            std::map<ServiceModel::ApplicationIdentifier, ApplicationEntrySPtr> applications_;

            FABRIC_SEQUENCE_NUMBER ackedSequence_;
            FABRIC_SEQUENCE_NUMBER healthSequence_;
            FABRIC_SEQUENCE_NUMBER initialSequence_;
            FABRIC_SEQUENCE_NUMBER invalidateSequence_;
            bool healthInitialized_;

            bool closed_;

            MUTABLE_RWLOCK(FM.ServiceCache, lock_);

            size_t nextServiceTraceIndex_;
            void TraceServices();
        };
    }
}