// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class TestStoreServiceProxy : public ITestStoreService, public Common::ComponentRoot
    {
        DENY_COPY(TestStoreServiceProxy);

    public:
        TestStoreServiceProxy(
            std::wstring const& clientId,
            std::wstring const& serviceLocation,
            std::wstring const& partitionId,
            std::wstring const& serviceName,
            Federation::NodeId const& nodeId);
        virtual ~TestStoreServiceProxy();

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
        
        virtual std::wstring const & GetPartitionId() const { return partitionId_; }
        virtual std::wstring const & GetServiceLocation() const { return serviceLocation_; }
        virtual std::wstring const & GetServiceName() const { return serviceName_; }
        virtual Federation::NodeId const & GetNodeId() const { return nodeId_; }
        
        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus() const
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::GetReadStatus not implemented");
        }

        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus() const
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::GetWriteStatus not implemented");
        }

        virtual Common::ComPointer<IFabricStatefulServicePartition> const& GetPartition() const 
        { 
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::GetPartition not implemented");
        }

        virtual void ReportFault(::FABRIC_FAULT_TYPE) const 
        { 
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::ReportFault not implemented");
        }

        virtual void ReportLoad(ULONG, ::FABRIC_LOAD_METRIC const *) const 
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::ReportLoad not implemented");
        }

        virtual void ReportMoveCost(::FABRIC_MOVE_COST) const
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::ReportMoveCost not implemented");
        }

        virtual Common::ErrorCode ReportPartitionHealth(
            ::FABRIC_HEALTH_INFORMATION const *,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * ) const
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::ReportPartitionHealth not implemented");
        }

        virtual Common::ErrorCode ReportReplicaHealth(
            ::FABRIC_HEALTH_INFORMATION const *,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const *) const
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::ReportInstanceHealth not implemented");
        }

        virtual void SetSecondaryPumpEnabled(bool) 
        { 
           TestCommon:: TestSession::FailTest("TestStoreServiceProxy::SetSecondaryPumpEnabled not implemented");
        }

        virtual bool IsStandby() const 
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::IsStandby not implemented");
        }

        virtual Common::ErrorCode Backup(std::wstring const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode Restore(std::wstring const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::AsyncOperationSPtr BeginBackup(
            __in std::wstring const &,
            __in StoreBackupOption::Enum,
            __in Api::IStorePostBackupHandlerPtr const &,
            __in Common::AsyncCallback const &,
            __in Common::AsyncOperationSPtr const &)
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::BeginBackup not implemented");
        }

        virtual Common::ErrorCode EndBackup(
            __in Common::AsyncOperationSPtr const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::AsyncOperationSPtr BeginRestore(
            __in std::wstring const &,
            __in Common::AsyncCallback const &,
            __in Common::AsyncOperationSPtr const &)
        {
            TestCommon::TestSession::FailTest("TestStoreServiceProxy::BeginRestore not implemented");
        }

        virtual Common::ErrorCode EndRestore(
            __in Common::AsyncOperationSPtr const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode CompressionTest(std::vector<std::wstring> const &)
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode getCurrentStoreTime(int64 & currentStoreTime) 
        {
            currentStoreTime = -1;
            return Common::ErrorCode(Common::ErrorCodeValue::NotImplemented); 
        }

        virtual Common::ErrorCode GetReplicationQueueCounters(
            __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
        {
            UNREFERENCED_PARAMETER(counters);
            return Common::ErrorCodeValue::NotImplemented;
        }

    private:
        std::wstring clientId_;
        std::wstring serviceLocation_;
        std::wstring partitionId_;
        std::wstring serviceName_;
        Federation::NodeId nodeId_;
    };

    typedef std::shared_ptr<TestStoreServiceProxy> TestStoreServiceProxySPtr;
};
