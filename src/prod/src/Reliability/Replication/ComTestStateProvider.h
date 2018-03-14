// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReplicationUnitTest
{
    class ComTestStateProvider : public IFabricStateProvider, private Common::ComUnknownBase
    {
        DENY_COPY(ComTestStateProvider)

        COM_INTERFACE_LIST1(
            ComTestStateProvider,
            IID_IFabricStateProvider,
            IFabricStateProvider)

    public:
        ComTestStateProvider(
            int64 numberOfCopyOperations,
            int64 numberOfCopyContextOperations,
            Common::ComponentRoot const & root,
            bool changeStateOnDataLoss = false);

        virtual ~ComTestStateProvider() { }

        HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            /* [in] */ FABRIC_EPOCH const * epoch,
            /* [in] */ FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);
        HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            /* [in] */ IFabricAsyncOperationContext *context);
        
        HRESULT STDMETHODCALLTYPE GetLastCommittedSequenceNumber( 
            /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber);

        HRESULT STDMETHODCALLTYPE BeginOnDataLoss( 
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndOnDataLoss( 
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][string][out] */ BOOLEAN * isStateChanged);
        
        HRESULT STDMETHODCALLTYPE GetCopyContext(/*[out, retval]*/ IFabricOperationDataStream **);

        HRESULT STDMETHODCALLTYPE GetCopyState(
            /*[in]*/ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber, 
            /*[in]*/ IFabricOperationDataStream * copyContextEnumerator,
            /*[out, retval]*/ IFabricOperationDataStream ** copyStateEnumerator);

        std::wstring GetProgressVectorString() const;
        void ClearProgressVector();
        void WaitForConfigurationNumber(LONGLONG configurationNumber);
            
        class ComTestAsyncEnumOperationData : public IFabricOperationDataStream, private Common::ComUnknownBase
        {
            DENY_COPY(ComTestAsyncEnumOperationData)

            COM_INTERFACE_LIST1(
                ComTestAsyncEnumOperationData,
                IID_IFabricOperationDataStream,
                IFabricOperationDataStream)

        public:
            ComTestAsyncEnumOperationData(
                std::wstring const & id,
                int64 numberOfOperations,
                ComTestStateProvider const & stateProvider,
                Common::ComponentRoot const & root);

            ComTestAsyncEnumOperationData(
                std::wstring const & id,
                int64 numberOfOperations,
                int64 numberOfInnerOperations,
                Common::ComPointer<IFabricOperationDataStream> && enumerator,
                ComTestStateProvider const & stateProvider,
                Common::ComponentRoot const & root);

            virtual ~ComTestAsyncEnumOperationData() {}
            
            virtual HRESULT STDMETHODCALLTYPE BeginGetNext(
                /*[in]*/ IFabricAsyncOperationCallback *,
                /*[out, retval]*/ IFabricAsyncOperationContext **);

            virtual HRESULT STDMETHODCALLTYPE EndGetNext(
                /*[in]*/ IFabricAsyncOperationContext *,
                /*[out, retval]*/ IFabricOperationData ** operation);
            
            bool TryIncrementOperationNumber() const;

        private:
            class ComGetNextOperation;

            std::wstring id_;
            int64 numberOfOperations_;
            int64 numberOfInnerOperations_;
            Reliability::ReplicationComponent::ComProxyAsyncEnumOperationData inEnumerator_;
            mutable Common::atomic_long currentIndex_;
            ComTestStateProvider const & stateProvider_;
            Common::ComponentRootSPtr rootSPtr_;
        };

        static Reliability::ReplicationComponent::ComProxyStateProvider GetProvider(
            int64 numberOfCopyOperations,
            int64 numberOfCopyContextOperations,
            Common::ComponentRoot const & root);

    protected:
        virtual HRESULT CreateOperation(
            bool lastOp,
            __out Common::ComPointer<IFabricOperationData> & op) const;

    private:
        typedef std::pair<FABRIC_EPOCH, FABRIC_SEQUENCE_NUMBER> ProgressVectorEntry;
        
        int64 numberOfCopyOperations_;
        int64 numberOfCopyContextOperations_;
        Common::ComponentRoot const & root_;
        bool changeStateOnDataLoss_;
        std::vector<ProgressVectorEntry> progressVector_;
        Common::RwLock progressVectorLock_;
    };
}
