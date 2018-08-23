// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace Store;

namespace FabricTest
{
    struct ITestStoreService
    {
    public:
        virtual ~ITestStoreService() { };

        virtual Common::ErrorCode Put(
            __int64 key,
            std::wstring const& value,
            FABRIC_SEQUENCE_NUMBER currentVersion,
            std::wstring serviceName,
            FABRIC_SEQUENCE_NUMBER & newVersion,
            ULONGLONG & dataLossVersion) = 0;

        virtual Common::ErrorCode Get(
            __int64 key,
            std::wstring serviceName,
            std::wstring & value,
            FABRIC_SEQUENCE_NUMBER & version,
            ULONGLONG & dataLossVersion) = 0;

        virtual Common::ErrorCode GetAll(
            std::wstring serviceName,
            vector<std::pair<__int64,std::wstring>> & kvpairs)
        {
            UNREFERENCED_PARAMETER(serviceName);
            UNREFERENCED_PARAMETER(kvpairs);
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode GetKeys(
            std::wstring serviceName,
            __int64 first,
            __int64 last,
            vector<__int64> & keys)
        {
            UNREFERENCED_PARAMETER(serviceName);
            UNREFERENCED_PARAMETER(first);
            UNREFERENCED_PARAMETER(last);
            UNREFERENCED_PARAMETER(keys);
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode Delete(
            __int64,
            FABRIC_SEQUENCE_NUMBER,
            std::wstring,
            FABRIC_SEQUENCE_NUMBER &,
            ULONGLONG &) 
        { 
            return Common::ErrorCodeValue::NotImplemented; 
        }

        virtual Common::ComPointer<IFabricStatefulServicePartition> const& GetPartition() const = 0;
        virtual std::wstring const & GetPartitionId() const = 0;
        virtual std::wstring const & GetServiceLocation() const = 0;
        virtual bool IsStandby() const = 0;
        virtual std::wstring const & GetServiceName() const = 0;
        virtual Federation::NodeId const & GetNodeId() const = 0;
        
        virtual void SetSecondaryPumpEnabled(bool value) = 0;

        virtual void ReportFault(::FABRIC_FAULT_TYPE faultType) const = 0;
        virtual void ReportLoad(ULONG metricCount, ::FABRIC_LOAD_METRIC const *metrics) const = 0;
        virtual void ReportMoveCost(::FABRIC_MOVE_COST moveCost) const = 0;
        virtual Common::ErrorCode ReportPartitionHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const = 0;
        virtual Common::ErrorCode ReportReplicaHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const = 0;
        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus() const = 0;
        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus() const = 0;

        virtual Common::ErrorCode Backup(std::wstring const & dir) = 0;
        virtual Common::ErrorCode Restore(std::wstring const & dir) = 0;

        virtual Common::AsyncOperationSPtr BeginBackup(
            __in std::wstring const & backupDir,
            __in StoreBackupOption::Enum backupOption,
            __in Api::IStorePostBackupHandlerPtr const & postBackupHandler,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndBackup(
            __in Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginRestore(
            __in std::wstring const & backupDir,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndRestore(
            __in Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::ErrorCode CompressionTest(std::vector<std::wstring> const & params) = 0;
        virtual Common::ErrorCode getCurrentStoreTime(int64 & currentStoreTime) = 0;

        virtual Common::ErrorCode RequestCheckpoint()
        {
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode GetReplicationQueueCounters(
            __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counter)
        {
            UNREFERENCED_PARAMETER(counter);
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual Common::ErrorCode GetReplicatorQueryResult(__out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
        {
            UNREFERENCED_PARAMETER(result);
            return Common::ErrorCodeValue::NotImplemented;
        }

        virtual NTSTATUS GetPeriodicCheckpointAndTruncationTimestamps(
            __out LONG64 & lastPeriodicCheckpoint,
            __out LONG64 & lastPeriodicTruncation)
        {
            lastPeriodicCheckpoint = 0;
            lastPeriodicTruncation = 0;
            return STATUS_NOT_IMPLEMENTED;
        }
    };

    typedef std::shared_ptr<ITestStoreService> ITestStoreServiceSPtr;
    typedef std::weak_ptr<ITestStoreService> ITestStoreServiceWPtr;
};
