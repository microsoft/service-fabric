// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class NodeCounts
        {
        public:
            NodeCounts();

            __declspec (property(get = get_NodeCount, put = set_NodeCount)) size_t NodeCount;
            size_t get_NodeCount() const { return nodeCount_; }
            void set_NodeCount(size_t nodeCount) { nodeCount_ = nodeCount; }

            __declspec (property(get = get_DownNodeCount, put = set_DownNodeCount)) size_t DownNodeCount;
            size_t get_DownNodeCount() const { return downNodeCount_; }
            void set_DownNodeCount(size_t downNodeCount) { downNodeCount_ = downNodeCount; }

            __declspec (property(get = get_UpNodeCount, put = set_UpNodeCount)) size_t UpNodeCount;
            size_t get_UpNodeCount() const { return upNodeCount_; }
            void set_UpNodeCount(size_t upNodeCount) { upNodeCount_ = upNodeCount; }

            __declspec (property(get = get_DeactivatedCount, put = set_DeactivatedCount)) size_t DeactivatedCount;
            size_t get_DeactivatedCount() const { return deactivatedCount_; }
            void set_DeactivatedCount(size_t deactivatedCount) { deactivatedCount_ = deactivatedCount; }

            __declspec (property(get = get_PauseIntentCount, put = set_PauseIntentCount)) size_t PauseIntentCount;
            size_t get_PauseIntentCount() const { return pauseIntentCount_; }
            void set_PauseIntentCount(size_t pauseIntentCount) { pauseIntentCount_ = pauseIntentCount; }

            __declspec (property(get = get_RestartIntentCount, put = set_RestartIntentCount)) size_t RestartIntentCount;
            size_t get_RestartIntentCount() const { return restartIntentCount_; }
            void set_RestartIntentCount(size_t restartIntentCount) { restartIntentCount_ = restartIntentCount; }

            __declspec (property(get = get_RemoveDataIntentCount, put = set_RemoveDataIntentCount)) size_t RemoveDataIntentCount;
            size_t get_RemoveDataIntentCount() const { return removeDataIntentCount_; }
            void set_RemoveDataIntentCount(size_t removeDataIntentCount) { removeDataIntentCount_ = removeDataIntentCount; }

            __declspec (property(get = get_RemoveNodeIntentCount, put = set_RemoveNodeIntentCount)) size_t RemoveNodeIntentCount;
            size_t get_RemoveNodeIntentCount() const { return removeNodeIntentCount_; }
            void set_RemoveNodeIntentCount(size_t removeNodeIntentCount) { removeNodeIntentCount_ = removeNodeIntentCount; }

            __declspec (property(get = get_UnknownCount, put = set_UnknownCount)) size_t UnknownCount;
            size_t get_UnknownCount() const { return unknownCount_; }
            void set_UnknownCount(size_t unknownCount) { unknownCount_ = unknownCount; }

            __declspec (property(get = get_RemovedNodeCount, put = set_RemovedNodeCount)) size_t RemovedNodeCount;
            size_t get_RemovedNodeCount() const { return removedNodeCount_; }
            void set_RemovedNodeCount(size_t removedNodeCount) { removedNodeCount_ = removedNodeCount; }

            __declspec (property(get = get_PendingDeactivateNode, put = set_PendingDeactivateNode)) size_t PendingDeactivateNode;
            size_t get_PendingDeactivateNode() const { return pendingDeactivateNode_; }
            void set_PendingDeactivateNode(size_t pendingDeactivateNode) { pendingDeactivateNode_ = pendingDeactivateNode; }

            __declspec (property(get = get_PendingFabricUpgrade, put = set_PendingFabricUpgrade)) size_t PendingFabricUpgrade;
            size_t get_PendingFabricUpgrade() const { return pendingFabricUpgrade_; }
            void set_PendingFabricUpgrade(size_t pendingFabricUpgrade) { pendingFabricUpgrade_ = pendingFabricUpgrade; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            size_t nodeCount_;
            size_t upNodeCount_;
            size_t downNodeCount_;
            size_t deactivatedCount_;
            size_t pauseIntentCount_;
            size_t restartIntentCount_;
            size_t removeDataIntentCount_;
            size_t removeNodeIntentCount_;
            size_t unknownCount_;
            size_t removedNodeCount_;
            size_t pendingDeactivateNode_;
            size_t pendingFabricUpgrade_;
        };
    }
}
