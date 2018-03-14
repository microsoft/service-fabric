// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class QueueCounts
        {
        public:
            QueueCounts(
                bool isFabricUpgrading,
                uint64 commonQueueLength,
                uint64 queryQueueLength,
                uint64 failoverUnitQueueLength,
                uint64 commitQueueLength,
                int commonQueueCompleted,
                int queryQueueCompleted,
                int failoverUnitQueueCompleted,
                int commitQueueCompleted);

            __declspec (property(get = get_CommonQueueLength, put = set_CommonQueueLength)) uint64 CommonQueueLength;
            uint64 get_CommonQueueLength() const { return commonQueueLength_; }
            void set_CommonQueueLength(uint64 commonQueueLength) { commonQueueLength_ = commonQueueLength; }

            __declspec (property(get = get_QueryQueueLength, put = set_QueryQueueLength)) uint64 QueryQueueLength;
            uint64 get_QueryQueueLength() const { return queryQueueLength_; }
            void set_QueryQueueLength(uint64 queryQueueLength) { queryQueueLength_ = queryQueueLength; }

            __declspec (property(get = get_FailoverUnitQueueLength, put = set_FailoverUnitQueueLength)) uint64 FailoverUnitQueueLength;
            uint64 get_FailoverUnitQueueLength() const { return failoverUnitQueueLength_; }
            void set_FailoverUnitQueueLength(uint64 failoverUnitQueueLength) { failoverUnitQueueLength_ = failoverUnitQueueLength; }

            __declspec (property(get = get_CommitQueueLength, put = set_CommitQueueLength)) uint64 CommitQueueLength;
            uint64 get_CommitQueueLength() const { return commitQueueLength_; }
            void set_CommitQueueLength(uint64 commitQueueLength) { commitQueueLength_ = commitQueueLength; }
            
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            uint64 commonQueueLength_;
            uint64 queryQueueLength_;
            uint64 failoverUnitQueueLength_;
            uint64 commitQueueLength_;
            int commonQueueCompleted_;
            int queryQueueCompleted_;
            int failoverUnitQueueCompleted_;
            int commitQueueCompleted_;

            bool isUpgradingFabric_;
        };
    }
}
