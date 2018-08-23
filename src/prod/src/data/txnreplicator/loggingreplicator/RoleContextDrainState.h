// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // State machine of the replicator's role, apply context and draining state
        //
        class RoleContextDrainState final
            : public KObject<RoleContextDrainState>
            , public KShared<RoleContextDrainState>
        {
            K_FORCE_SHARED(RoleContextDrainState)

        public:

            static RoleContextDrainState::SPtr Create(
                __in Data::Utilities::IStatefulPartition & partition,
                __in KAllocator & allocator);

            __declspec(property(get = get_ApplyRedoContext)) TxnReplicator::ApplyContext::Enum ApplyRedoContext;
            TxnReplicator::ApplyContext::Enum get_ApplyRedoContext() const
            {
                TxnReplicator::ApplyContext::Enum result;

                {
                    lock_.AcquireShared();
                    result = applyRedoContext_;
                    lock_.ReleaseShared();
                }

                return result;
            }

            __declspec(property(get = get_DrainStream)) DrainingStream::Enum DrainStream;
            DrainingStream::Enum get_DrainStream() const
            {
                DrainingStream::Enum result;

                {
                    lock_.AcquireShared();
                    result = drainingStream_;
                    lock_.ReleaseShared();
                }

                return result;
            }

            __declspec(property(get = get_IsClosing)) bool IsClosing;
            bool get_IsClosing() const
            {
                return isClosing_.load();
            }

            __declspec(property(get = get_ReplicaRole)) FABRIC_REPLICA_ROLE ReplicaRole;
            FABRIC_REPLICA_ROLE get_ReplicaRole() const
            {
                FABRIC_REPLICA_ROLE result;

                {
                    lock_.AcquireShared();
                    result = role_;
                    lock_.ReleaseShared();
                }

                return result;
            }

            void Reuse();

            void ReportFault();

            void OnClosing();

            void OnRecovery();

            void OnRecoveryCompleted();

            void OnDrainState();

            void OnDrainCopy();

            void OnDrainReplication();

            void ChangeRole(__in FABRIC_REPLICA_ROLE newRole);

        private:

            RoleContextDrainState(__in Data::Utilities::IStatefulPartition & partition);

            void BecomePrimaryCallerholdsLock();

            void BecomeActiveSecondaryCallerholdsLock();

            mutable KReaderWriterSpinLock lock_;
            Data::Utilities::IStatefulPartition::SPtr partition_;
            FABRIC_REPLICA_ROLE role_;
            DrainingStream::Enum drainingStream_;
            TxnReplicator::ApplyContext::Enum applyRedoContext_;
            Common::atomic_bool isClosing_;
        };
    }
}
