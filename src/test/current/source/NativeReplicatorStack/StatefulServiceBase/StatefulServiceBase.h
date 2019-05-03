// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

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

        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
            /* [in] */ IFabricStatefulServicePartition *partition,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricReplicator **replicationEngine);

        virtual HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceEndpoint);

        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual void STDMETHODCALLTYPE Abort();

    protected:

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

        __declspec(property(get = get_TxReplicator)) TxnReplicator::ITransactionalReplicator::SPtr TxReplicator;
        __declspec(noinline)
        TxnReplicator::ITransactionalReplicator::SPtr get_TxReplicator() const
        {
            return txReplicatorSPtr_;
        }

        __declspec(property(get = get_Role)) FABRIC_REPLICA_ROLE Role;
        FABRIC_REPLICA_ROLE get_Role() const
        {
            return role_;
        }

        __declspec(property(get = get_KtlSystem)) KtlSystem * KtlSystemValue;
        KtlSystem * get_KtlSystem() const
        {
            return ktlSystem_;
        }

        virtual Common::ComPointer<IFabricStateProvider2Factory> GetStateProviderFactory() = 0;

        virtual Common::ComPointer<IFabricDataLossHandler> GetDataLossHandler() = 0;

        virtual Common::ErrorCode OnHttpPostRequest(Common::ByteBufferUPtr && body, Common::ByteBufferUPtr & responseBody) = 0;

        virtual void OnChangeRole(
            /* [in] */ FABRIC_REPLICA_ROLE newRole,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

    private:

        LONGLONG const instanceId_;
        Common::ComponentRootSPtr const root_;
        FABRIC_REPLICA_ID const replicaId_;
        FABRIC_PARTITION_ID const partitionId_;
        std::wstring const httpListenAddress_;
        std::wstring const changeRoleEndpoint_;
        
        FABRIC_REPLICA_ROLE role_;

        HttpServer::IHttpServerSPtr httpServerSPtr_;

        Common::ComPointer<IFabricStatefulServicePartition> partition_;
        Common::ComPointer<IFabricPrimaryReplicator> primaryReplicator_;
        TxnReplicator::ITransactionalReplicator::SPtr txReplicatorSPtr_;
        KtlSystem * ktlSystem_;
    };

    typedef Common::ComPointer<StatefulServiceBase> StatefulServiceBaseCPtr;
}
