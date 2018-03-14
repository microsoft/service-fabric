// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class FabricTimeController
        : public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;
        DENY_COPY(FabricTimeController);
    public:
        FabricTimeController(__in ReplicatedStore & replicatedStore);
        virtual ~FabricTimeController();
        Common::ErrorCode GetCurrentStoreTime(__out int64 & currentStoreTime);
        void Start(bool setCancelWait = false);
        void Cancel();

    private:
        void TryScheduleTimer(Common::TimeSpan delay);
        void RefreshAndPersist();
        void RefreshAndPersistCompletedCallback(Common::AsyncOperationSPtr const & operation, bool setReady, bool completedSynchronously, Common::ActivityId activityId);
        ReplicatedStore & replicatedStore_;
        FabricTimeData fabricTimeData_;
        Common::TimerSPtr refreshTimer_;
        bool ready_;
		Common::atomic_bool cancelled_;
        Common::atomic_bool refreshActive_;
        Common::ComponentRootSPtr root_;
    };
}
