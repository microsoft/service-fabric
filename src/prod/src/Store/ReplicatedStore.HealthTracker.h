// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::HealthTracker : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(HealthTracker);

    private:
        HealthTracker(__in ReplicatedStore &, Common::ComPointer<IFabricStatefulServicePartition3> &&);

    public:

        static Common::ErrorCode Create(
            __in ReplicatedStore &,
            Common::ComPointer<IFabricStatefulServicePartition> const &,
            __out std::shared_ptr<HealthTracker> &);

        void OnSlowCommit();
        Common::TimeSpan ComputeRefreshTime(Common::TimeSpan const threshold);

        void ReportErrorDetails(Common::ErrorCode const &);
        void ReportHealth(Common::SystemHealthReportCode::Enum, std::wstring const & description, Common::TimeSpan const ttl);

        virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        void InitializeTimestamps(int targetCountThreshold);
        Common::ErrorCode CreateHealthInformation(
            Common::SystemHealthReportCode::Enum, 
            std::wstring const & description, 
            Common::TimeSpan const ttl,
            __in Common::ScopedHeap &,
            __out FABRIC_HEALTH_INFORMATION const **);

        Common::SystemHealthReportCode::Enum ToHealthReportCode(Common::ErrorCode const &);

        ReplicatedStore & replicatedStore_;
        Common::ComPointer<IFabricStatefulServicePartition3> healthReporter_;

        Common::RwLock slowCommitLock_;
        std::vector<Common::DateTime> slowCommitTimestamps_;
        size_t currentSlowCommitIndex_;
        Common::DateTime lastSlowCommitReportTimestamp_;
    };
}

