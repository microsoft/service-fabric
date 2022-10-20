// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{
    class RepairPolicyEngineServiceReplica : 
        public IFabricStatefulServiceReplica,
        public IFabricStoreEventHandler,
        private Common::ComUnknownBase
    {
        DENY_COPY(RepairPolicyEngineServiceReplica);

        BEGIN_COM_INTERFACE_LIST(RepairPolicyEngineServiceReplica)
            COM_INTERFACE_ITEM(IID_IFabricStoreEventHandler,IFabricStoreEventHandler)
            COM_INTERFACE_ITEM(IID_IFabricStatefulServiceReplica,IFabricStatefulServiceReplica)
        END_COM_INTERFACE_LIST()

    public:

        RepairPolicyEngineServiceReplica();
        ~RepairPolicyEngineServiceReplica();

        HRESULT Initialize(
            IFabricCodePackageActivationContext *activationContext,
            FABRIC_PARTITION_ID partitionId,
            FABRIC_REPLICA_ID replicaId,
            Common::ManualResetEvent* exitServiceHostEventPtr);

        // Property accessors
        __declspec(property(put=put_ReplicaRole)) FABRIC_REPLICA_ROLE ReplicaRole;
        void put_ReplicaRole(FABRIC_REPLICA_ROLE value) 
        { 
            replicaRole_ = value; 
            Common::StringWriter stringWriter(replicaRoleString_);
            stringWriter << value;
        };

        //
        // IFabricStatefulServiceReplica members
        //

        STDMETHODIMP BeginOpen( 
            __in FABRIC_REPLICA_OPEN_MODE openMode,
            __in IFabricStatefulServicePartition *partition,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndOpen( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricReplicator **replicator);
        
        STDMETHODIMP BeginChangeRole( 
            __in FABRIC_REPLICA_ROLE newRole,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndChangeRole( 
            __in IFabricAsyncOperationContext *context,
            __out IFabricStringResult **serviceAddress);
        
        STDMETHODIMP BeginClose( 
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);
        
        STDMETHODIMP EndClose( 
            __in IFabricAsyncOperationContext *context);
        
        STDMETHODIMP_(void) Abort( void);

        //
        // IFabricStoreEventHandler members
        //

        STDMETHODIMP_(void) OnDataLoss( void);

    private:
         // These are needed for tracing
        std::wstring partitionIdString_;
        std::wstring replicaRoleString_;
        std::wstring replicaIdString_;
        
        std::wstring replicatorAddress_;

        FABRIC_REPLICA_ROLE replicaRole_;

        RepairPolicyEngineService repairPolicyEngineService_;

        Common::ComPointer<IFabricCodePackageActivationContext> activationContextCPtr_;
        Common::ComPointer<IFabricReplicator> replicatorCPtr_;
        Common::ComPointer<IFabricStateReplicator> stateReplicatorCPtr_;
        Common::ComPointer<IFabricStateProvider> stateProviderCPtr_;

        Common::ManualResetEvent* exitServiceHostEventPtr_;
        Common::ComPointer<IFabricKeyValueStoreReplica2> keyValueStorePtr_;

        static const std::wstring KeyValueStoreName;
    };
}
