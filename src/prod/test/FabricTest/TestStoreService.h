// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    typedef std::function<void(std::wstring const&, std::wstring const&, FABRIC_REPLICA_ROLE)> ChangeRoleCallback;

    class FabricTestSession;

    class TestStoreService : public ITestStoreService, public Common::ComponentRoot
    {
        DENY_COPY(TestStoreService);

    public:
        TestStoreService(
            FABRIC_PARTITION_ID partitionId,
            std::wstring const& serviceName,
            Federation::NodeId nodeId,
            bool changeServiceLocationOnChangeRole,
            std::wstring const& initDataStr,
            std::wstring const& serviceType = TestStoreService::DefaultServiceType,
            std::wstring const& appId = L"");
        virtual ~TestStoreService();

        Common::ErrorCode OnOpen(
            Common::ComPointer<IFabricStatefulServicePartition3> const& partition,
            Common::ComPointer<IFabricStateReplicator> const& replicator,
            bool operationStream2Enabled);

        Common::ErrorCode OnChangeRole(FABRIC_REPLICA_ROLE newRole);

        Common::ErrorCode OnClose();

        void OnAbort();

        HRESULT STDMETHODCALLTYPE UpdateEpoch(
            FABRIC_EPOCH const * epoch,
            FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber);

        HRESULT STDMETHODCALLTYPE GetCurrentProgress(
            FABRIC_SEQUENCE_NUMBER *sequenceNumber);

        HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            IFabricAsyncOperationCallback *callback,
            IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            IFabricAsyncOperationContext *context,
            BOOLEAN * isStateChanged);

        Common::AsyncOperationSPtr BeginBackup(
            __in std::wstring const & backupDir,
            __in StoreBackupOption::Enum backupOption,
            __in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent)
        {
            UNREFERENCED_PARAMETER(backupDir);
            UNREFERENCED_PARAMETER(backupOption);
            UNREFERENCED_PARAMETER(postBackupHandler);
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(parent);

            return nullptr;
        }

        Common::ErrorCode EndBackup(
            __in Common::AsyncOperationSPtr const & operation)
        {
            UNREFERENCED_PARAMETER(operation);

            return Common::ErrorCodeValue::NotImplemented;
        }

        HRESULT STDMETHODCALLTYPE GetCopyContext(IFabricOperationDataStream ** copyContextEnumerator);

        HRESULT STDMETHODCALLTYPE GetCopyState(
            FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            IFabricOperationDataStream * copyContextEnumerator,
            IFabricOperationDataStream ** copyStateEnumerator);

        static std::wstring const DefaultServiceType;

        virtual Common::ErrorCode Put(
            __int64 key,
            std::wstring const& value,
            FABRIC_SEQUENCE_NUMBER currentVersion,
            std::wstring serviceName,
            FABRIC_SEQUENCE_NUMBER & newVersion,
            ULONGLONG & dataLossVersion);

        virtual Common::ErrorCode Get(
            __int64 key,
            std::wstring serviceName,
            std::wstring & value,
            FABRIC_SEQUENCE_NUMBER & version,
            ULONGLONG & dataLossVersion);

        virtual Common::ComPointer<IFabricStatefulServicePartition> const& GetPartition() const { return oldPartition_; }
        virtual std::wstring const & GetPartitionId() const { return partitionId_; }
        virtual std::wstring const & GetServiceLocation() const { return serviceLocation_; }
        virtual std::wstring const & GetServiceName() const { return serviceName_; }
        virtual Federation::NodeId const & GetNodeId() const { return nodeId_; }

        virtual void ReportFault(::FABRIC_FAULT_TYPE faultType) const
        {
            partitionWrapper_.ReportFault(faultType);
        }

        virtual Common::ErrorCode ReportPartitionHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
        {
            return  partitionWrapper_.ReportPartitionHealth(healthInfo, sendOptions);

        }
        virtual Common::ErrorCode ReportReplicaHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
        {
            return partitionWrapper_.ReportReplicaHealth(healthInfo, sendOptions);
        }

        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus() const
        {
            return partitionWrapper_.GetWriteStatus();
        }

        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus() const
        {
            return partitionWrapper_.GetReadStatus();
        }

        virtual void ReportLoad(ULONG metricCount, ::FABRIC_LOAD_METRIC const *metrics) const
        {
            return partitionWrapper_.ReportLoad(metricCount, metrics);
        }

        virtual void ReportMoveCost(::FABRIC_MOVE_COST moveCost) const
        {
            return partitionWrapper_.ReportMoveCost(moveCost);
        }

        virtual void SetSecondaryPumpEnabled(bool value) { secondaryPumpEnabled_ = value; }

        virtual Common::ErrorCode Backup(std::wstring const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode Restore(std::wstring const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        Common::AsyncOperationSPtr BeginRestore(
            __in std::wstring const & backupDir,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent)
        {
            UNREFERENCED_PARAMETER(backupDir);
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(parent);

            return nullptr;
        }

        Common::ErrorCode EndRestore(
            __in Common::AsyncOperationSPtr const & operation)
        {
            UNREFERENCED_PARAMETER(operation);

            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode CompressionTest(std::vector<std::wstring> const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        OpenCallback OnOpenCallback;
        CloseCallback OnCloseCallback;
        ChangeRoleCallback OnChangeRoleCallback;

        __declspec (property(get = getServiceLocation)) std::wstring const& ServiceLocation;
        std::wstring const& getServiceLocation()
        {
            Common::AcquireReadLock grab(stateLock_);
            return serviceLocation_;
        }

        __declspec (property(get = getServiceName)) std::wstring const& ServiceName;
        std::wstring const& getServiceName() { return serviceName_; }

        __declspec (property(get = getNodeId)) Federation::NodeId const& NodeId;
        Federation::NodeId const& getNodeId() { return nodeId_; }

        __declspec (property(get = getInitDataString)) std::wstring const& InitDataString;
        std::wstring const& getInitDataString() { return initDataStr_; }

        bool ShouldFailOn(ApiFaultHelper::ComponentName compName, std::wstring operationName, ApiFaultHelper::FaultType faultType = ApiFaultHelper::Failure)
        {
            return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, compName, operationName, faultType);
        }

        bool IsSignalSet(std::wstring strFlag)
        {
            return TestHooks::ApiSignalHelper::IsSignalSet(nodeId_, serviceName_, strFlag);
        }

        void WaitForSignalReset(std::wstring strFlag)
        {
            return TestHooks::ApiSignalHelper::WaitForSignalReset(nodeId_, serviceName_, strFlag);
        }

        virtual bool IsStandby() const { return false;/*Is non persisted service so cannot be in Standby*/ }

        virtual Common::ErrorCode getCurrentStoreTime(int64 & currentStoreTime)
        {
            currentStoreTime = -1;
            return Common::ErrorCode(Common::ErrorCodeValue::NotImplemented);
        }

        virtual Common::ErrorCode GetReplicationQueueCounters(
            __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters);

        virtual Common::ErrorCode GetReplicatorQueryResult(__out ServiceModel::ReplicatorStatusQueryResultSPtr & result);

    private:
        struct TestServiceEntry;
        class TestFabricOperationData;
        class ComTestAsyncEnumOperationData;
        class ComTestGetNextOperation;

        enum PumpState { PumpNotStarted, PumpCopy, PumpReplication };

        std::wstring serviceName_;
        std::wstring partitionId_;
        std::wstring serviceLocation_;
        bool isOpen_;
        bool secondaryPumpEnabled_;
        FABRIC_REPLICA_ROLE replicaRole_;
        StatefulPartitionWrapper<IFabricStatefulServicePartition3> partitionWrapper_;
        Common::ComPointer<IFabricStatefulServicePartition3> partition_;
        Common::ComPointer<IFabricStatefulServicePartition> oldPartition_;
        Common::ComPointer<IFabricStateReplicator> replicator_;
        Common::RwLock stateLock_;
        Common::ExclusiveLock entryMapLock_;
        std::map<__int64, std::shared_ptr<TestServiceEntry>> entryMap_;
        ::FABRIC_SERVICE_PARTITION_KIND scheme_;
        __int64 highKey_;
        __int64 lowKey_;
        std::wstring partitionName_;
        LONGLONG dataLossNumber_;
        LONGLONG configurationNumber_;
        Federation::NodeId nodeId_;
        Common::ComPointer<IFabricStateProvider> stateProviderCPtr_;
        // The service doesn't support parallel pumps, 
        // so keep track of the pump we are at
        PumpState pumpState_;
        bool changeRoleCalled_;
        std::wstring serviceType_;
        std::wstring appId_;
        bool changeServiceLocationOnChangeRole_;
        std::wstring initDataStr_;
        bool streamFaultsAndRequireServiceAckEnabled_;

        Common::ManualResetEvent pumpingCompleteEvent_;

        void StartReplicationOperationPump(Common::ComPointer<IFabricStateReplicator> const & replicator);
        void StartCopyOperationPump(Common::ComPointer<IFabricStateReplicator> const & replicator);

        void BeginGetOperations(Common::ComPointer<IFabricStateReplicator> const & replicator, Common::ComPointer<IFabricOperationStream> const & stream);
        bool FinishProcessingOperation(IFabricAsyncOperationContext * context, Common::ComPointer<IFabricStateReplicator> const & replicator, Common::ComPointer<IFabricOperationStream> const & stream);

        bool VerifyServiceLocation(__int64 key, std::wstring const& serviceName);
        std::shared_ptr<TestServiceEntry> & GetEntry(__int64 key);
        bool TryGetEntry(__int64 key, std::shared_ptr<TestServiceEntry> & entry);
        void Cleanup();

        BOOLEAN GetCompletedSynchronously(IFabricAsyncOperationContext * context);
    };

    typedef std::shared_ptr<TestStoreService> TestStoreServiceSPtr;
};
