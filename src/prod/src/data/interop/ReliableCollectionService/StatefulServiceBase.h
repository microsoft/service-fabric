// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

typedef void(*removePartitionContextCallback)(long long key);
typedef void(*addPartitionContextCallback)(long long key, TxnReplicatorHandle txReplicator, IFabricStatefulServicePartition* partition, GUID partitionId, long long replicaId);

namespace TXRStatefulServiceBase
{
    class StatefulServiceBase
        : public IFabricStatefulServiceReplica
        , Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(StatefulServiceBase)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica, IFabricStatefulServiceReplica)
        END_COM_INTERFACE_LIST()

    // IFabricStatefulServiceReplica methods
    public:

        HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicationEngine);

        HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceEndpoint);

        HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context);

        void STDMETHODCALLTYPE Abort();

		Common::ComPointer<IFabricStateProvider2Factory> GetStateProviderFactory();

        Common::ComPointer<IFabricDataLossHandler> GetDataLossHandler();

        StatefulServiceBase(
            __in ULONG httpListeningPort,
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);
        
        // Uses default http endpoint resource. Name is a const defined in Helpers::ServiceHttpEndpointResourceName in Helpers.cpp
        StatefulServiceBase(
            __in FABRIC_PARTITION_ID partitionId,
            __in FABRIC_REPLICA_ID replicaId,
            __in Common::ComponentRootSPtr const & root);

        virtual ~StatefulServiceBase();

    public:
        static void SetCallback(addPartitionContextCallback addCallback, removePartitionContextCallback removeCallback) 
        {
            s_addPartitionContextCallbackFnptr = addCallback;
            s_removePartitionContextCallbackFnptr = removeCallback;
        }

	protected:
        __declspec(property(get = get_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
        FABRIC_REPLICA_ID get_ReplicaId() const
        {
            return replicaId_;
        }

        __declspec(property(get = get_PartitionId)) FABRIC_PARTITION_ID PartitionId;
        FABRIC_PARTITION_ID get_PartitionId() const
        {
            return partitionId_;
        }

        __declspec(property(get = get_TxReplicator)) TxnReplicatorHandle TxReplicator;
        TxnReplicatorHandle get_TxReplicator() const
        {
            return txReplicator_;
        }

		__declspec(property(get = get_Role)) FABRIC_REPLICA_ROLE Role;
		FABRIC_REPLICA_ROLE get_Role() const
		{
			return role_;
		}


    private:

		LONGLONG StatefulServiceBase::GetPartitionLowKey();

        LONGLONG const instanceId_;
        Common::ComponentRootSPtr const root_;
        FABRIC_REPLICA_ID const replicaId_;
        FABRIC_PARTITION_ID const partitionId_;
        std::wstring const httpListenAddress_;
        std::wstring const changeRoleEndpoint_;
        
        FABRIC_REPLICA_ROLE role_;

        Common::ComPointer<IFabricStatefulServicePartition> partition_;
        Common::ComPointer<IFabricPrimaryReplicator> primaryReplicator_;
		TxnReplicatorHandle txReplicator_;

    private:
        static addPartitionContextCallback s_addPartitionContextCallbackFnptr;
        static removePartitionContextCallback s_removePartitionContextCallbackFnptr;
    };

    typedef Common::ComPointer<StatefulServiceBase> StatefulServiceBaseCPtr;
}
