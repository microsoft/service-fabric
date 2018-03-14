// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class LoadCache
        {
            typedef std::vector<FailoverUnitId> PersistenceList;
            typedef std::unique_ptr<PersistenceList> PersistenceListUPtr;

            DENY_COPY(LoadCache);
        public:
            LoadCache(
                FailoverManager& fm,
                FailoverManagerStore& fmStore,
                LoadBalancingComponent::IPlacementAndLoadBalancing & plb,
                std::vector<LoadInfoSPtr> & loads);

            void GetAllFailoverUnitIds(std::vector<FailoverUnitId> &);
            void GetAllLoads(std::vector<LoadInfoSPtr> &);
            LoadInfoSPtr GetLoads(FailoverUnitId const&) const;
            void UpdateLoads(LoadBalancingComponent::LoadOrMoveCostDescription && loads);
            void DeleteLoads(FailoverUnitId const&);

            void PeriodicPersist();

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

            
        private:
            void Enqueue(FailoverUnitId const &);
            void StartPersist();
            Common::ErrorCode PersistBatchAsync(size_t listIndex);
            void OnPersistCompleted(size_t listIndex, bool isSuccess, int64 operationLSN);
            void PersistCallback(size_t listIndex, int64 operationLSN, Common::ErrorCode result);

            FailoverManager& fm_;
            FailoverManagerStore& fmStore_;
            LoadBalancingComponent::IPlacementAndLoadBalancing & plb_;
            std::map<FailoverUnitId, LoadInfoSPtr> loads_;

            std::queue<FailoverUnitId> pendingQueue_;
            std::vector<PersistenceListUPtr> activeLists_;
            size_t maxBatchSize_;

            mutable Common::ExclusiveLock lock_;
        };
    }
}
