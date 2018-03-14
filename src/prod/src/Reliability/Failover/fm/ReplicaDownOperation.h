// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ReplicaDownOperation;
        typedef std::shared_ptr<ReplicaDownOperation> ReplicaDownOperationSPtr;

        class ReplicaDownOperation
        {
            DENY_COPY(ReplicaDownOperation);

        public:

            ReplicaDownOperation(
                FailoverManager & fm,
                Federation::NodeInstance const& from);

            ~ReplicaDownOperation();

            void Start(
                ReplicaDownOperationSPtr const& thisSPtr,
                FailoverManager & fm,
                ReplicaListMessageBody && body);

            void AddResult(
                FailoverManager & fm,
                FailoverUnitId const& failoverUnitId,
                ReplicaDescription && replicaDescription);

        private:

            void OnTimer();

            void CompleteCallerHoldsLock(FailoverManager & fm);

            Federation::NodeInstance from_;
            std::set<FailoverUnitId> pending_;
            std::map<FailoverUnitId, ReplicaDescription> processed_;

            bool isCompleted_;
            Common::TimerSPtr timer_;

            Common::RootedObjectWeakHolder<FailoverManager> root_;

            RWLOCK(FM.ReplicaDownOperation, lock_);
        };
    }
}
