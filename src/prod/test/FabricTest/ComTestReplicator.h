// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComTestReplicator : 
        public IFabricPrimaryReplicator, 
        public IFabricInternalReplicator,
        public IFabricReplicatorCatchupSpecificQuorum,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComTestReplicator)

        BEGIN_COM_INTERFACE_LIST(ComTestReplicator)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricPrimaryReplicator)
            COM_INTERFACE_ITEM(IID_IFabricPrimaryReplicator, IFabricPrimaryReplicator)
            COM_INTERFACE_ITEM(IID_IFabricReplicator, IFabricPrimaryReplicator)
            COM_INTERFACE_ITEM(IID_IFabricInternalReplicator, IFabricInternalReplicator)
            COM_DELEGATE_TO_METHOD(QueryInterfaceEx)
        END_COM_INTERFACE_LIST()

    public:
        virtual ~ComTestReplicator() {}

        explicit ComTestReplicator(
            Common::ComPointer<IFabricReplicator> const& replicator, 
            Common::ComPointer<IFabricStatefulServicePartition> const& partition,
            bool disableCatchupSpecificQuorum,
            std::wstring const& serviceName,
            Federation::NodeId const& nodeId);

        STDMETHOD(QueryInterfaceEx)(REFIID riid, void ** result)
        {
            if (riid == IID_IFabricReplicatorCatchupSpecificQuorum && !disableCatchupSpecificQuorum_)
            {
                BaseAddRef();
                *result = static_cast<IFabricReplicatorCatchupSpecificQuorum*>(this);
                return S_OK;
            }
            else
            {
                return E_NOINTERFACE;
            }
        }

        // *****************************
        // IFabricReplicator methods
        // *****************************
        virtual HRESULT STDMETHODCALLTYPE BeginOpen( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndOpen( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **replicationEndpoint);

        virtual HRESULT STDMETHODCALLTYPE BeginChangeRole( 
            /* [in] */ FABRIC_EPOCH const * epoch,
            /* [in] */ FABRIC_REPLICA_ROLE role,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndChangeRole( 
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            /* [in] */ FABRIC_EPOCH const * epoch,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT STDMETHODCALLTYPE BeginClose( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndClose( 
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual void STDMETHODCALLTYPE Abort();

        virtual HRESULT STDMETHODCALLTYPE GetCurrentProgress( 
            /* [out] */ FABRIC_SEQUENCE_NUMBER *lastSequenceNumber);

        virtual HRESULT STDMETHODCALLTYPE GetCatchUpCapability( 
            /* [out] */ FABRIC_SEQUENCE_NUMBER *firstSequenceNumber);

        // *****************************
        // IFabricPrimaryReplicator methods
        // *****************************
        virtual HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        virtual HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ BOOLEAN * isStateChanged);

        virtual HRESULT STDMETHODCALLTYPE UpdateCatchUpReplicaSetConfiguration( 
            /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration,
            /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * previousConfiguration);

        virtual HRESULT STDMETHODCALLTYPE BeginWaitForCatchUpQuorum(
            /* [in] */ FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndWaitForCatchUpQuorum( 
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT STDMETHODCALLTYPE UpdateCurrentReplicaSetConfiguration( 
            /* [in] */ FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration);

        virtual HRESULT STDMETHODCALLTYPE BeginBuildReplica( 
            /* [in] */ FABRIC_REPLICA_INFORMATION const *replica,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT STDMETHODCALLTYPE EndBuildReplica( 
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual HRESULT STDMETHODCALLTYPE RemoveReplica( 
            /* [in] */ FABRIC_REPLICA_ID replicaId);

        virtual HRESULT STDMETHODCALLTYPE GetReplicatorStatus(
            /* [out, retval] */ IFabricGetReplicatorStatusResult **replicatorStatus);


        bool ShouldFailOn(ApiFaultHelper::ComponentName compName, std::wstring operationName, ApiFaultHelper::FaultType faultType=ApiFaultHelper::Failure)
        {
            return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, compName, operationName, faultType);
        }

        void WaitForSignalReset(std::wstring const& strSignal)
        {
            if (TestHooks::ApiSignalHelper::IsSignalSet(nodeId_, serviceName_, strSignal))
            {
                TestHooks::ApiSignalHelper::WaitForSignalReset(nodeId_, serviceName_, strSignal);
            }
        }

    private:

        void ComTestReplicator::VerifyReplicaAndIncrementMustCatchupCounter(
            FABRIC_REPLICA_INFORMATION const & replica,
            FABRIC_REPLICA_ROLE expectedRole,
            bool isMustCatchupAllowed,
            int & counter) const;

        bool ComTestReplicator::VerifyReplicaSetConfigurationAndReturnIfMustCatchupFound(
            FABRIC_REPLICA_SET_CONFIGURATION const * configuration,
            bool isMustCatchupAllowed) const;

        void ComTestReplicator::UpdateNewReplicas(FABRIC_REPLICA_SET_CONFIGURATION const * configuration, std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> &);

        Common::ComPointer<::IFabricReplicator> replicator_;
        Common::ComPointer<::IFabricPrimaryReplicator> primary_;
        Common::ComPointer<::IFabricInternalReplicator> internal_;
        Common::ComPointer<IFabricStatefulServicePartition> partition_;
        
        std::wstring serviceName_;
        Federation::NodeId nodeId_; 
        bool disableCatchupSpecificQuorum_;
        ReadWriteStatusValidatorUPtr readWriteStatusValidator_;
        
        ComTestReplicaMap replicaMap_;

        void CheckForReportFaultsAndDelays(ApiFaultHelper::ComponentName compName, std::wstring operationName);
    };
};
