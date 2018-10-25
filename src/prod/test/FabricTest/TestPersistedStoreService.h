// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
	using namespace Common;
	using namespace std;

    class FabricTestSession;

    class TestPersistedStoreService : public ITestStoreService, public Store::KeyValueStoreReplica
    {
        DENY_COPY(TestPersistedStoreService);

    private:
        static std::wstring const StoreDataType;
        static int const DestructorWaitDelayInSeconds;

    public:
        static std::wstring const DefaultServiceType;
        static std::wstring const TestPersistedStoreDirectory;
        static std::wstring const SharedLogFilename;
        static std::wstring const EseDatabaseFilename;

    public:
        TestPersistedStoreService(
            Common::Guid const &,
            FABRIC_REPLICA_ID,
            std::wstring const& serviceName,
            Federation::NodeId nodeId,
            bool changeServiceLocationOnChangeRole,
            TestCommon::TestSession & testSession,            
            std::wstring const& initDataStr = L"",
            std::wstring const& serviceType = TestPersistedStoreService::DefaultServiceType,
            std::wstring const& appId = L"");
        virtual ~TestPersistedStoreService();

        __declspec(property(get = get_DisableReplicatorCatchupSpecificReplica)) bool DisableReplicatorCatchupSpecificReplica;
        bool get_DisableReplicatorCatchupSpecificReplica() const { return disableReplicatorCatchupSpecificReplica_; }

        __declspec(property(get = get_ReplicatedStorePrivate)) Store::ReplicatedStore * ReplicatedStorePrivate;
        Store::ReplicatedStore * get_ReplicatedStorePrivate() const;

        virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const &);
        virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation);
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();
        
        virtual Common::ErrorCode OnCopyComplete(Api::IStoreEnumeratorPtr const &);
        virtual Common::ErrorCode OnReplicationOperation(Api::IStoreNotificationEnumeratorPtr const &);

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

        virtual Common::ErrorCode Delete(
             __int64 key,
            FABRIC_SEQUENCE_NUMBER currentVersion,
            std::wstring serviceName,
            FABRIC_SEQUENCE_NUMBER & newVersion,
            ULONGLONG & dataLossVersion);

        OpenCallback OnOpenCallback;
        CloseCallback OnCloseCallback;
        ChangeRoleCallback OnChangeRoleCallback;

        virtual std::wstring const & GetPartitionId() const { return partitionId_; }
        virtual std::wstring const & GetServiceLocation() const { return testServiceLocation_; }
        virtual Common::ComPointer<IFabricStatefulServicePartition> const& GetPartition() const { return oldPartition_; }
        
        virtual bool IsStandby() const { return (state_ == State::Opened && !changeRoleCalled_); }
        virtual std::wstring const & GetServiceName() const { return serviceName_; }
        virtual Federation::NodeId const & GetNodeId() const { return nodeId_; }
        
        virtual void ReportFault(::FABRIC_FAULT_TYPE faultType) const 
        { 
            partitionWrapper_.ReportFault(faultType);
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

        virtual void ReportMoveCost(::FABRIC_MOVE_COST moveCost) const
        {
            UNREFERENCED_PARAMETER(moveCost);
            TestCommon::TestSession::FailTest("TestPersistedStoreService::ReportMoveCost not implemented");
        }

        virtual void SetSecondaryPumpEnabled(bool value);
        void ReportStreamFault(FABRIC_FAULT_TYPE);

        virtual Common::ErrorCode Backup(std::wstring const & dir);
        virtual Common::ErrorCode Restore(std::wstring const & dir);

		Common::AsyncOperationSPtr BeginBackup(
			__in std::wstring const & backupDir,
			__in StoreBackupOption::Enum backupOption,
			__in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
			__in Common::AsyncCallback const & callback,
			__in Common::AsyncOperationSPtr const & parent);

		ErrorCode EndBackup(
			__in Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRestore(
            __in std::wstring const & backupDir,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent);

        ErrorCode EndRestore(
            __in Common::AsyncOperationSPtr const & operation);

        virtual Common::ErrorCode CompressionTest(std::vector<std::wstring> const & params);
        virtual Common::ErrorCode CompressionWriteTest(std::vector<std::wstring> const & params);
        virtual Common::ErrorCode CompressionReadTest(std::vector<std::wstring> const & params);

        __declspec (property(get=getServiceName)) std::wstring const& ServiceName;
        std::wstring const& getServiceName() { return serviceName_; }

        __declspec (property(get=getNodeId)) Federation::NodeId const& NodeId;
        Federation::NodeId const& getNodeId() { return nodeId_; }

        bool ShouldFailOn(ApiFaultHelper::ComponentName compName, std::wstring operationName, ApiFaultHelper::FaultType faultType=ApiFaultHelper::Failure)
        {
            return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, compName, operationName, faultType);
        }

        bool IsSignalSet(std::wstring const& strSignal)
        {
            return TestHooks::ApiSignalHelper::IsSignalSet(nodeId_, serviceName_, strSignal);
        }

        void ResetSignal(std::wstring const& strSignal)
        {
            TestHooks::ApiSignalHelper::ResetSignal(nodeId_.ToString(), serviceName_, strSignal);
        }

        void WaitForSignalReset(std::wstring const& strSignal)
        {
            if (IsSignalSet(strSignal))
            {
                TestHooks::ApiSignalHelper::WaitForSignalReset(nodeId_, serviceName_, strSignal);
            }
        }

        static void DumpPending();
                
        virtual Common::ErrorCode getCurrentStoreTime(int64 & currentStoreTime)
        {
            return ReplicatedStore->GetCurrentStoreTime(currentStoreTime);
        }

        void SetStateProvider(__in IFabricStateProvider *);

    protected:
        //
        // IStoreEventHandler
        //

        virtual Common::AsyncOperationSPtr BeginOnDataLoss(
            __in Common::AsyncCallback const &,
            __in Common::AsyncOperationSPtr const &) override;

        virtual Common::ErrorCode EndOnDataLoss(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isStateChanged) override;

        virtual Common::ErrorCode GetReplicationQueueCounters(
            __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters) override;

    private:

        class ComProxyDataLossAsyncOperation;

        enum State { Initialized, Opened, Closed };
        std::wstring ToString(State const & e);

        bool VerifyServiceLocation(__int64 key, std::wstring const& serviceName);
        void Cleanup();

        std::shared_ptr<Common::RwLock> GetLock(__int64 key);

        Common::ErrorCode ReadCurrentData(Store::IStoreBase::EnumerationSPtr const & enumSPtr, __out std::wstring & value);
        Common::ErrorCode GetCurrentOperationLSN(
            Store::IStoreBase::TransactionSPtr const & transactionSPtr, 
            std::wstring const & key, 
            __out _int64 & currentOperationLSN, 
            __out Store::IStoreBase::EnumerationSPtr & enumSPtr);
        Common::ErrorCode SeekToDataItem(std::wstring const & key, Store::IStoreBase::EnumerationSPtr const & enumSPtr);

		Common::ErrorCode GetBackupOption(__in FABRIC_STORE_BACKUP_OPTION backupOption, __out StoreBackupOption::Enum & storeBackupOption);

        std::wstring GetPendingItemId() const;

        Common::ComPointer<IFabricStateProvider> GetStateProvider();

        Store::ReplicatedStore * TryGetReplicatedStorePrivate() const;

        StatefulPartitionWrapper<IFabricStatefulServicePartition3> partitionWrapper_;

        std::wstring serviceName_;
        std::wstring testServiceLocation_;
        Common::ComPointer<IFabricStatefulServicePartition> oldPartition_;
        Common::ComPointer<IFabricStatefulServicePartition3> partition_;
        std::wstring partitionId_;
        ::FABRIC_SERVICE_PARTITION_KIND scheme_;
        __int64 highKey_;
        __int64 lowKey_;
        std::wstring partitionName_;
        Federation::NodeId nodeId_;
        State state_;
        Common::ExclusiveLock lockMapLock_;
        std::map<__int64, std::shared_ptr<Common::RwLock>> lockMap_;
        bool changeRoleCalled_;
        std::wstring serviceType_;
        std::wstring appId_;
        TestCommon::TestSession & testSession_;
        bool changeServiceLocationOnChangeRole_;
        std::wstring initDataStr_;

        Common::TimeSpan onCopyCompletionDelay_;
        bool faultCopyCompletion_;
        bool disableReplicatorCatchupSpecificReplica_;

        Common::ComPointer<IFabricStateProvider> stateProvider_;
        Common::RwLock stateProviderLock_;
    };

    typedef std::shared_ptr<TestPersistedStoreService> TestPersistedStoreServiceSPtr;
};
