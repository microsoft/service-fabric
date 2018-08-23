// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestDispatcher;
    class TestHealthTable;

    class CheckpointTruncationTimestampValidator
    {
        DENY_COPY(CheckpointTruncationTimestampValidator);

    public:
        CheckpointTruncationTimestampValidator(FabricTestDispatcher & dispatcher);

        std::vector<std::wstring> const & ServiceNames() { return serviceNames_; }

        void AddService(std::wstring & serviceName);
        void RemoveService(std::wstring & serviceName);
        void Stop();

    private:
        void Run();
        std::wstring GetTraceViewerQuery(Common::Guid partitionId, int64 replicaId);

        std::vector<std::wstring> serviceNames_;

        // Key = <PartitionId><ReplicaId>
        // Value = tuple<LastPeriodicCheckpointTimeInTicks, LastPeriodicTruncationTimeInTicks>
        std::map<std::wstring, tuple<LONG64, LONG64>> periodicCheckpointAndLogTruncationTimestampMap_;

        // Key = <PartitionId><ReplicaId>
        // Tracks any failures to provide a buffer time prior to failing the test
        // Ensures that API fault injection (i.e. failing sp2.*checkpoint* APIs) can occur safely
        std::map<std::wstring, LONG64> failureTimestampMap_;

        Common::ExclusiveLock validationLock_;
        bool isActive_;

        FabricTestDispatcher & dispatcher_;
        Random random_;
    };

    typedef std::shared_ptr<CheckpointTruncationTimestampValidator> CheckpointTruncationTimestampValidatorSPtr;
}
