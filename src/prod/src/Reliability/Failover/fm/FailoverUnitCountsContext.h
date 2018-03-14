// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverUnitCountsContext : public BackgroundThreadContext
        {
            DENY_COPY(FailoverUnitCountsContext);

        public:

            class TraceWriter
            {
                DENY_COPY_ASSIGNMENT(TraceWriter);

            public:
                TraceWriter(
                    FailoverUnitCountsContext const& performanceCounterContext,
                    size_t failoverUnitCount,
                    size_t inBuildFailoverUnitCount);

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

                void FillEventData(Common::TraceEventContext & context) const;
                void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            private:
                FailoverUnitCountsContext const& performanceCounterContext_;

                size_t failoverUnitCount_;
                size_t inBuildFailoverUnitCount_;

                uint64 commonQueueLength_;
                uint64 queryQueueLength_;
                uint64 failoverUnitQueueLength_;
                uint64 commitQueueLength_;

                bool isUpgradingFabric_;
            };

            FailoverUnitCountsContext();

            virtual BackgroundThreadContextUPtr CreateNewContext() const;

            virtual void Process(FailoverManager const& fm, FailoverUnit const& failoverUnit);

            virtual void Merge(BackgroundThreadContext const & context);

            virtual bool ReadyToComplete();

            virtual void Complete(FailoverManager & fm, bool isContextCompleted, bool isEnumerationAborted);

        private:
            int statelessFailoverUnitCount_;
            int volatileFailoverUnitCount_;
            int persistedFailoverUnitCount_;

            int quorumLossFailoverUnitCount_;
            int unhealthyFailoverUnitCount_;
            int deletingFailoverUnitCount_;
            int deletedFailoverUnitCount_;

            size_t replicaCount_;
            size_t inBuildReplicaCount_;
            size_t standByReplicaCount_;
            size_t offlineReplicaCount_;
            size_t droppedReplicaCount_;
        };
    }
}
