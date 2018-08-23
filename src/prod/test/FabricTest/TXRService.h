// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;
    
    //
    // Transactional Replicator Based service
    //
    class TXRService 
        : public ITestStoreService
        , public Common::ComponentRoot
    {
        DENY_COPY(TXRService);

    public:
        static std::wstring const DefaultServiceType;

        TXRService(
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId,
            std::wstring const& serviceName,
            Federation::NodeId nodeId,
            std::wstring const& workingDir,
            TestCommon::TestSession & testSession,        
            std::wstring const& initDataStr,
            std::wstring const& serviceType = TXRService::DefaultServiceType,
            std::wstring const& appId = L"");

        virtual ~TXRService();

        Common::ErrorCode OnOpen(
            __in Common::ComPointer<IFabricStatefulServicePartition> const &,
            __in TxnReplicator::ITransactionalReplicator & transactionalReplicator,
            __in Common::ComPointer<IFabricPrimaryReplicator> & primaryReplicator,
            __in KAllocator & allocator);

        Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole);
        Common::ErrorCode OnClose();
        void OnAbort();
        void Cleanup();

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

        __declspec (property(get = getServiceName)) std::wstring const& ServiceName;
        std::wstring const& getServiceName() { return serviceName_; }

        __declspec (property(get = getWorkDir)) std::wstring const& WorkDirectory;
        std::wstring const& getWorkDir() { return workDir_; }

        __declspec (property(get = getNodeId)) Federation::NodeId const& NodeId;
        Federation::NodeId const& getNodeId() { return nodeId_; }

        __declspec (property(get = getInitDataString)) std::wstring const& InitDataString;
        std::wstring const& getInitDataString() { return initDataStr_; }

        OpenCallback OnOpenCallback;
        CloseCallback OnCloseCallback;
        ChangeRoleCallback OnChangeRoleCallback;

        virtual Common::ErrorCode Put(
            __int64 key,
            std::wstring const& value,
            FABRIC_SEQUENCE_NUMBER currentVersion,
            std::wstring serviceName,
            FABRIC_SEQUENCE_NUMBER & newVersion,
            ULONGLONG & dataLossVersion) override;

        virtual Common::ErrorCode Get(
            __int64 key,
            std::wstring serviceName,
            std::wstring & value,
            FABRIC_SEQUENCE_NUMBER & version,
            ULONGLONG & dataLossVersion) override;

        virtual Common::ErrorCode Delete(
            __int64,
            FABRIC_SEQUENCE_NUMBER,
            std::wstring,
            FABRIC_SEQUENCE_NUMBER &,
            ULONGLONG &) override;

        Common::AsyncOperationSPtr BeginBackup(
            __in std::wstring const & backupDir,
            __in StoreBackupOption::Enum backupOption,
            __in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) override;
        
        Common::ErrorCode EndBackup(
            __in Common::AsyncOperationSPtr const & operation) override;

        Common::ComPointer<IFabricStatefulServicePartition> const& GetPartition() const override;
        std::wstring const & GetPartitionId() const override;
        std::wstring const & GetServiceLocation() const override;
        std::wstring const & GetServiceName() const override;
        Federation::NodeId const & GetNodeId() const override;

        void ReportFault(::FABRIC_FAULT_TYPE faultType) const override;
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus() const override;
        FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus() const override;
        Common::ErrorCode ReportPartitionHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const override;
        Common::ErrorCode ReportReplicaHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const override;
        void ReportLoad(ULONG metricCount, ::FABRIC_LOAD_METRIC const *metrics) const override;
        void ReportMoveCost(::FABRIC_MOVE_COST moveCost) const override;
        void SetSecondaryPumpEnabled(bool value) override;
        Common::ErrorCode Backup(std::wstring const &) override;
        Common::ErrorCode Restore(std::wstring const &) override;
        Common::AsyncOperationSPtr BeginRestore(
            __in std::wstring const & backupDir,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndRestore(__in Common::AsyncOperationSPtr const & operation) override;
        Common::ErrorCode CompressionTest(std::vector<std::wstring> const &) override;
        Common::ErrorCode getCurrentStoreTime(int64 & currentStoreTime) override;
        bool IsStandby() const override;

        Common::ErrorCode RequestCheckpoint() override;

        Common::ErrorCode GetReplicationQueueCounters(
            __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters) override;

        Common::ErrorCode GetReplicatorQueryResult(
            __out ServiceModel::ReplicatorStatusQueryResultSPtr & result) override;

        NTSTATUS GetPeriodicCheckpointAndTruncationTimestamps(
            __out LONG64 & lastPeriodicCheckpoint,
            __out LONG64 & lastPeriodicTruncation) override;

    protected:
        TxnReplicator::ITransactionalReplicator::SPtr GetTxnReplicator();

        KAllocator& GetAllocator();

        ::FABRIC_SERVICE_PARTITION_KIND GetScheme();

        bool VerifyServiceLocation(
            __int64 key,
            std::wstring const& serviceName);

        // Wraps KTL based invocations and converts the exceptions to error codes
        Common::ErrorCode InvokeKtlApi(std::function<void()> func);

        Common::ErrorCode InvokeKtlApiNoExcept(std::function<NTSTATUS()> func) noexcept;

        //
        // Commit timeout in the transaction api only applies to the time it takes to accept the begin replicate
        // However, if the beginreplicate is already accepted, the commit waits forever to return
        // This wrapper returns timeout when this is the case
        //
        Common::ErrorCode CommitTransactionWrapper(
            __in TxnReplicator::Transaction & transaction,
            __in Common::TimeSpan const & timeout) noexcept;

        Common::ErrorCode FromNTStatus(
            __in NTSTATUS status) noexcept;

    private:

        static std::wstring GetSPNameFromKey(__int64 key);

        enum State { Initialized, Opened, Closed };
        StatefulPartitionWrapper<IFabricStatefulServicePartition3> partitionWrapper_;
        TxnReplicator::ITransactionalReplicator::SPtr transactionalReplicator_;
        Common::ComPointer<IFabricPrimaryReplicator> primaryReplicator_;

        KAllocator * allocator_;

        std::wstring partitionId_;
        FABRIC_REPLICA_ID replicaId_;
        std::wstring serviceName_;
        std::wstring testServiceLocation_;
        Common::ComPointer<IFabricStatefulServicePartition> oldPartition_;
        Common::ComPointer<IFabricStatefulServicePartition3> partition_;
        ::FABRIC_SERVICE_PARTITION_KIND scheme_;
        __int64 highKey_;
        __int64 lowKey_;
        std::wstring partitionName_;
        Federation::NodeId nodeId_;
        std::wstring workDir_;
        State state_;
        std::wstring serviceType_;
        std::wstring appId_;
        TestCommon::TestSession & testSession_;
        std::wstring initDataStr_;
    };

    typedef std::shared_ptr<TXRService> TXRServiceSPtr;
}
