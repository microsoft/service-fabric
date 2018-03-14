// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace TestCommon;

namespace FabricTest
{
    class SGComProxyReplicateAsyncOperation : 
        public Common::ComProxyAsyncOperation
    {
        DENY_COPY(SGComProxyReplicateAsyncOperation);

    public:
        SGComProxyReplicateAsyncOperation(
            Common::ComPointer<IFabricStateReplicator> const & stateReplicator,
            IFabricOperationData* replicationData,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::ComProxyAsyncOperation(callback, parent)
            , stateReplicator_(stateReplicator)
            , replicationData_(replicationData)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAsyncOperation::SGComProxyReplicateAsyncOperation ({0}) - ctor",
                this);
        }

        virtual ~SGComProxyReplicateAsyncOperation()
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAsyncOperation::~SGComProxyReplicateAsyncOperation ({0}) - dtor",
                this);
        }

        virtual HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAsyncOperation::BeginComAsyncOperation ({0}) - BeginReplicate",
                this);

            return stateReplicator_->BeginReplicate(
                replicationData_, 
                callback,
                &sequenceNumber_, 
                context);
        }

        virtual HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAsyncOperation::EndComAsyncOperation ({0}) - EndReplicate",
                this);

            return stateReplicator_->EndReplicate(context, nullptr);
        }

        __declspec (property(get=get_ReplicationData)) IFabricOperationData* ReplicationData;
        IFabricOperationData* get_ReplicationData() { return replicationData_; }

        __declspec (property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
        FABRIC_SEQUENCE_NUMBER get_SequenceNumber() { return sequenceNumber_; }

    private:
        Common::ComPointer<IFabricStateReplicator> stateReplicator_;

        IFabricOperationData* replicationData_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;

        static Common::StringLiteral const TraceSource;
    };

    class SGComProxyReplicateAtomicGroupAsyncOperation : 
        public Common::ComProxyAsyncOperation
    {
        DENY_COPY(SGComProxyReplicateAtomicGroupAsyncOperation);

    public:
        SGComProxyReplicateAtomicGroupAsyncOperation(
            Common::ComPointer<IFabricAtomicGroupStateReplicator> const & stateReplicator,
            FABRIC_ATOMIC_GROUP_ID atomicGroup,
            IFabricOperationData* replicationData,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::ComProxyAsyncOperation(callback, parent)
            , stateReplicator_(stateReplicator)
            , atomicGroup_(atomicGroup)
            , replicationData_(replicationData)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAtomicGroupAsyncOperation::SGComProxyReplicateAtomicGroupAsyncOperation ({0}) - ctor",
                this);
        }

        virtual ~SGComProxyReplicateAtomicGroupAsyncOperation()
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAtomicGroupAsyncOperation::~SGComProxyReplicateAtomicGroupAsyncOperation ({0}) - dtor",
                this);
        }

        virtual HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAtomicGroupAsyncOperation::BeginComAsyncOperation ({0}) - BeginReplicateAtomicGroupOperation",
                this);

            return stateReplicator_->BeginReplicateAtomicGroupOperation(
                atomicGroup_,
                replicationData_, 
                callback,
                &sequenceNumber_, 
                context);
        }

        virtual HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyReplicateAtomicGroupAsyncOperation::EndComAsyncOperation ({0}) - EndReplicateAtomicGroupOperation",
                this);

            return stateReplicator_->EndReplicateAtomicGroupOperation(context, nullptr);
        }

        __declspec (property(get=get_ReplicationData)) IFabricOperationData* ReplicationData;
        IFabricOperationData* get_ReplicationData() { return replicationData_; }

        __declspec (property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
        FABRIC_SEQUENCE_NUMBER get_SequenceNumber() { return sequenceNumber_; }

         __declspec (property(get=get_AtomicGroupId)) FABRIC_ATOMIC_GROUP_ID AtomicGroupId;
        FABRIC_ATOMIC_GROUP_ID get_AtomicGroupId() { return atomicGroup_; }

    private:
        Common::ComPointer<IFabricAtomicGroupStateReplicator> stateReplicator_;

        IFabricOperationData* replicationData_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
        FABRIC_ATOMIC_GROUP_ID atomicGroup_;

        static Common::StringLiteral const TraceSource;
    };

    class SGComProxyCommitOrRollbackAtomicGroupAsyncOperation : 
        public Common::ComProxyAsyncOperation
    {
        DENY_COPY(SGComProxyCommitOrRollbackAtomicGroupAsyncOperation);

    public:
        SGComProxyCommitOrRollbackAtomicGroupAsyncOperation(
            Common::ComPointer<IFabricAtomicGroupStateReplicator> const & stateReplicator,
            FABRIC_ATOMIC_GROUP_ID atomicGroup,
            IFabricOperationData* replicationData,
            bool commit,
            int endDelay,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::ComProxyAsyncOperation(callback, parent)
            , stateReplicator_(stateReplicator)
            , atomicGroup_(atomicGroup)
            , replicationData_(replicationData)
            , commit_(commit)
            , endDelay_(endDelay)
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::SGComProxyCommitOrRollbackAtomicGroupAsyncOperation ({0}) - ctor",
                this);
        }

        virtual ~SGComProxyCommitOrRollbackAtomicGroupAsyncOperation()
        {
            TestSession::WriteNoise(
                TraceSource, 
                "SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::~SGComProxyCommitOrRollbackAtomicGroupAsyncOperation ({0}) - dtor",
                this);
        }

        virtual HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
        {
            if (commit_)
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    "SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::BeginReplicateAtomicGroupCommit ({0}) - BeginReplicateAtomicGroupCommit",
                    this);

                return stateReplicator_->BeginReplicateAtomicGroupCommit(
                    atomicGroup_,
                    callback,
                    &sequenceNumber_, 
                    context);
            }
            else
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    "SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::BeginReplicateAtomicGroupCommit ({0}) - BeginReplicateAtomicGroupRollback",
                    this);

                return stateReplicator_->BeginReplicateAtomicGroupRollback(
                    atomicGroup_,
                    callback,
                    &sequenceNumber_, 
                    context);
            }
        }

        virtual HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
        {
            if (endDelay_ > 0)
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    "SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::EndComAsyncOperation ({0}) - Sleeping for {1} ms",
                    this,
                    (DWORD)endDelay_ * 1000);

                ::SleepEx((DWORD)endDelay_ * 1000, FALSE);
            }

            if (commit_)
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    "SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::EndComAsyncOperation ({0}) - EndReplicateAtomicGroupCommit",
                    this);

                return stateReplicator_->EndReplicateAtomicGroupCommit(context, nullptr);
            }
            else
            {
                TestSession::WriteNoise(
                    TraceSource, 
                    "SGComProxyCommitOrRollbackAtomicGroupAsyncOperation::EndComAsyncOperation ({0}) - EndReplicateAtomicGroupRollback",
                    this);

                return stateReplicator_->EndReplicateAtomicGroupRollback(context, nullptr);
            }
        }

         __declspec (property(get=get_ReplicationData)) IFabricOperationData* ReplicationData;
        IFabricOperationData* get_ReplicationData() { return replicationData_; }

        __declspec (property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
        FABRIC_SEQUENCE_NUMBER get_SequenceNumber() { return sequenceNumber_; }

         __declspec (property(get=get_AtomicGroupId)) FABRIC_ATOMIC_GROUP_ID AtomicGroupId;
        FABRIC_ATOMIC_GROUP_ID get_AtomicGroupId() { return atomicGroup_; }

    private:
        Common::ComPointer<IFabricAtomicGroupStateReplicator> stateReplicator_;

        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
        FABRIC_ATOMIC_GROUP_ID atomicGroup_;

        IFabricOperationData* replicationData_;

        bool commit_;

        int endDelay_;

        static Common::StringLiteral const TraceSource;
    };
};
