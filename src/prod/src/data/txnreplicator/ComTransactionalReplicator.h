// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Entry point to the replicator from failover subsystem
    // This class inherits from the IFabricPrimaryReplicator, which the RAP uses to talk to the replicator 
    //
    // There are 2 categories of methods in this class
    //      1. Pass through to V1 replicator - API's specific to the IFabricPrimaryReplicator like catchup, UC etc are just a pass through to the V1 replicator
    //      2. Lifecycle API's - Open, Close, Abort and ChangeRole API's are first forwarded to the V1 replicator and then invoked on the Transactional Replicator
    //
    class ComTransactionalReplicator final
        : public KObject<ComTransactionalReplicator>
        , public KShared<ComTransactionalReplicator>
        , public IFabricPrimaryReplicator
        , public IFabricTransactionalReplicator
        , public IFabricInternalReplicator
        , public IFabricReplicatorCatchupSpecificQuorum
        , public IFabricInternalStateReplicator
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(ComTransactionalReplicator)

        K_BEGIN_COM_INTERFACE_LIST(ComTransactionalReplicator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricReplicator)
            COM_INTERFACE_ITEM(IID_IFabricReplicator, IFabricReplicator)
            COM_INTERFACE_ITEM(IID_IFabricPrimaryReplicator, IFabricPrimaryReplicator)
            COM_INTERFACE_ITEM(IID_IFabricTransactionalReplicator, IFabricTransactionalReplicator)
            COM_INTERFACE_ITEM(IID_IFabricInternalReplicator, IFabricInternalReplicator)
            COM_INTERFACE_ITEM(IID_IFabricReplicatorCatchupSpecificQuorum, IFabricReplicatorCatchupSpecificQuorum)
            COM_INTERFACE_ITEM(IID_IFabricInternalStateReplicator, IFabricInternalStateReplicator)
        K_END_COM_INTERFACE_LIST()

    public:

        //
        // To be used by services to create the replicator
        //
        static NTSTATUS Create(
            __in FABRIC_REPLICA_ID replicaId,
            __in Reliability::ReplicationComponent::IReplicatorFactory & replicatorFactory,
            __in IFabricStatefulServicePartition & partition,
            __in_opt FABRIC_REPLICATOR_SETTINGS const * replicatorSettings,
            __in IRuntimeFolders & runtimeFolders,
            __in BOOLEAN hasPersistedState,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
            __in IFabricStateProvider2Factory & factory,
            __in TxnReplicator::TRInternalSettingsSPtr && transactionalReplicatorConfig,
            __in TxnReplicator::SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
            __in Data::Log::LogManager & logManager,
            __in IFabricDataLossHandler & userDataLossHandler,
            __in KAllocator & allocator,
            __out IFabricPrimaryReplicator ** primaryReplicator,
            __out ITransactionalReplicator::SPtr & transactionalReplicator);

        Data::LoggingReplicator::IStateProvider::SPtr GetStateProvider()
        {
            return transactionalReplicator_->GetStateProvider();
        }

        // IFabricReplicator methods
        STDMETHOD_(HRESULT, BeginOpen)(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        STDMETHOD_(HRESULT, EndOpen)(
            __in IFabricAsyncOperationContext * context,
            __out IFabricStringResult ** replicationAddress) override;

        STDMETHOD_(HRESULT, BeginChangeRole)(
            __in FABRIC_EPOCH const * epoch,
            __in FABRIC_REPLICA_ROLE role,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        STDMETHOD_(HRESULT, EndChangeRole)(
            __in IFabricAsyncOperationContext * context) override;

        STDMETHOD_(HRESULT, BeginUpdateEpoch)(
            __in FABRIC_EPOCH const * epoch,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        STDMETHOD_(HRESULT, EndUpdateEpoch)(
            __in IFabricAsyncOperationContext * context) override;

        STDMETHOD_(HRESULT, BeginClose)(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context)override;

        STDMETHOD_(HRESULT, EndClose)(
            __in IFabricAsyncOperationContext * context)override;

        STDMETHOD_(void, Abort)()override;

        STDMETHOD_(HRESULT, GetCurrentProgress)(
            __out FABRIC_SEQUENCE_NUMBER * lastSequenceNumber)override;

        STDMETHOD_(HRESULT, GetCatchUpCapability)(
            __out FABRIC_SEQUENCE_NUMBER * fromSequenceNumber)override;
        
        // IFabricPrimaryReplicator methods
        STDMETHOD_(HRESULT, BeginOnDataLoss)(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context)override;

        STDMETHOD_(HRESULT, EndOnDataLoss)(
            __in IFabricAsyncOperationContext * context,
            __out BOOLEAN * isStateChanged)override;

        STDMETHOD_(HRESULT, UpdateCatchUpReplicaSetConfiguration)(
            __in const FABRIC_REPLICA_SET_CONFIGURATION * currentConfiguration,
            __in const FABRIC_REPLICA_SET_CONFIGURATION * previousConfiguration)override;

        STDMETHOD_(HRESULT, BeginWaitForCatchUpQuorum)(
            __in FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context)override;

        STDMETHOD_(HRESULT, EndWaitForCatchUpQuorum)(
            __in IFabricAsyncOperationContext * context)override;

        STDMETHOD_(HRESULT, UpdateCurrentReplicaSetConfiguration)(
            __in const FABRIC_REPLICA_SET_CONFIGURATION * currentConfiguration)override;

        STDMETHOD_(HRESULT, BeginBuildReplica)(
            __in const FABRIC_REPLICA_INFORMATION * replica,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context)override;

        STDMETHOD_(HRESULT, EndBuildReplica)(
            __in IFabricAsyncOperationContext * context)override;

        STDMETHOD_(HRESULT, RemoveReplica)(
            __in FABRIC_REPLICA_ID replicaId)override;

        // IFabricTransactionalReplicator methods
        STDMETHOD_(HRESULT, UpdateReplicatorSettings)(
            __in FABRIC_REPLICATOR_SETTINGS const * replicatorSettings)override;       

        // IFabricInternalReplicator methods
        STDMETHOD_(HRESULT, GetReplicatorStatus)(
            __in IFabricGetReplicatorStatusResult **replicatorStatus)override;

        // IFabricInternalStateReplicator methods
        STDMETHOD_(HRESULT, BeginReplicateBatch)(
            __in IFabricBatchOperationData * operationData,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context)override
        {
            UNREFERENCED_PARAMETER(operationData);
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(context);
            return E_FAIL;
        }

        STDMETHOD_(HRESULT, EndReplicateBatch)(
            __in IFabricAsyncOperationContext * operationData)override
        {
            UNREFERENCED_PARAMETER(operationData);
            return E_FAIL;
        }

        STDMETHOD_(HRESULT, ReserveSequenceNumber)(
            __in BOOLEAN alwaysReserveWhenPrimary,
            __out FABRIC_SEQUENCE_NUMBER * sequenceNumber)override
        {
            UNREFERENCED_PARAMETER(alwaysReserveWhenPrimary);
            UNREFERENCED_PARAMETER(sequenceNumber);
            return E_FAIL;
        }

        STDMETHOD_(HRESULT, GetBatchReplicationStream)(
            __in IFabricBatchOperationStream **batchStream)override
        {
            UNREFERENCED_PARAMETER(batchStream);
            return E_FAIL;
        }

        STDMETHOD_(HRESULT, GetReplicationQueueCounters)(
            __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS * counters)override;

        void Test_SetFaulted();

    private:

        //
        // KTL based Async operation to open the v1 replicator and then open the transactional replicator
        //
        class ComAsyncOpenContext
            : public Ktl::Com::FabricAsyncContextBase
            , public IFabricAsyncOperationCallback
        {
            friend ComTransactionalReplicator;

            K_FORCE_SHARED(ComAsyncOpenContext)

            K_BEGIN_COM_INTERFACE_LIST(ComAsyncOpenContext)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
                COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
            K_END_COM_INTERFACE_LIST()

        public:

            HRESULT StartOpen(
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback);

            STDMETHOD_(void, Invoke)(
                __in IFabricAsyncOperationContext * context) override;

            void GetResult(__out IFabricStringResult ** replicatorAddress);

        protected:

            void OnStart();

        private:

            void OnFinishV1ReplicatorOpen(__in IFabricAsyncOperationContext *context, __in BOOLEAN expectCompletedSynchronously);

            ktl::Task OpenTransactionalReplicator();
            
            ComTransactionalReplicator::SPtr parent_;
            Common::ComPointer<IFabricStringResult> replicatorAddress_;
        };
    
        //
        // KTL based Async operation to close the v1 replicator and then close the transactional replicator
        //
        class ComAsyncCloseContext
            : public Ktl::Com::FabricAsyncContextBase
            , public IFabricAsyncOperationCallback
        {
            friend ComTransactionalReplicator;

            K_FORCE_SHARED(ComAsyncCloseContext)

            K_BEGIN_COM_INTERFACE_LIST(ComAsyncCloseContext)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
                COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
            K_END_COM_INTERFACE_LIST()

        public:

            HRESULT StartClose(
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback);

            STDMETHOD_(void, Invoke)(
                __in IFabricAsyncOperationContext * context) override;

        protected:

            void OnStart();

        private:

            void OnFinishV1ReplicatorClose(__in IFabricAsyncOperationContext *context, __in BOOLEAN expectCompletedSynchronously);

            ktl::Task CloseTransactionalReplicator();
            
            ComTransactionalReplicator::SPtr parent_;
        };

        //
        // KTL based Async operation to Change role the v1 replicator and then the transactional replicator
        //
        class ComAsyncChangeRoleContext
            : public Ktl::Com::FabricAsyncContextBase
            , public IFabricAsyncOperationCallback
        {
            friend ComTransactionalReplicator;

            K_FORCE_SHARED(ComAsyncChangeRoleContext)

            K_BEGIN_COM_INTERFACE_LIST(ComAsyncChangeRoleContext)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricAsyncOperationCallback)
                COM_INTERFACE_ITEM(IID_IFabricAsyncOperationCallback, IFabricAsyncOperationCallback)
            K_END_COM_INTERFACE_LIST()

        public:

            HRESULT StartChangeRole(
                __in FABRIC_EPOCH const * epoch,
                __in FABRIC_REPLICA_ROLE role,
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback);

            STDMETHOD_(void, Invoke)(
                __in IFabricAsyncOperationContext * context) override;

        protected:

            void OnStart();

        private:

            void OnFinishV1ReplicatorChangeRole(__in IFabricAsyncOperationContext *context, __in BOOLEAN expectCompletedSynchronously);

            ktl::Task ChangeRoleTransactionalReplicator();
            
            ComTransactionalReplicator::SPtr parent_;
            FABRIC_EPOCH epoch_;
            FABRIC_REPLICA_ROLE newRole_;
        };

        ComTransactionalReplicator(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in FABRIC_REPLICA_ID replicaId,
            __in IFabricStatefulServicePartition & partition,
            __in Common::ComPointer<IFabricPrimaryReplicator> & v1PrimaryReplicator,
            __in Data::LoggingReplicator::IStateReplicator & v1StateReplicator,
            __in IFabricStateProvider2Factory & factory,
            __in IRuntimeFolders & runtimeFolders,
            __in TxnReplicator::TRInternalSettingsSPtr && transactionalReplicatorConfig,
            __in TxnReplicator::SLInternalSettingsSPtr && ktlLoggerSharedLogConfig,
            __in Data::Log::LogManager & logManager,
            __in IFabricDataLossHandler & userDataLossHandler,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr && healthClient,
            __in BOOLEAN hasPersistedState);

        Common::ComPointer<IFabricPrimaryReplicator> primaryReplicator_;
        Common::ComPointer<IFabricStateReplicator> fabricStateReplicator_;
        Common::ComPointer<IFabricInternalReplicator> fabricInternalReplicator_;
        TransactionalReplicator::SPtr transactionalReplicator_;
        Common::atomic_bool isFaulted_;
    };
}
