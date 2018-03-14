// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // The loggingreplicator is a state provider to the V1 replicator
    // This com based KTL state provider converts the com API calls from the V1 replicator to KTL based async operations
    //
    // The KTL based async operations are further converted to awaitables in the implementation of the state provider
    //
    class ComStateProvider final
        : public KObject<ComStateProvider>
        , public KShared<ComStateProvider>
        , public IFabricStateProvider
        , public IFabricStateProviderSupportsCopyUntilLatestLsn
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(ComStateProvider)

        K_BEGIN_COM_INTERFACE_LIST(ComStateProvider)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateProvider)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider, IFabricStateProvider)
            COM_INTERFACE_ITEM(IID_IFabricStateProviderSupportsCopyUntilLatestLsn, IFabricStateProviderSupportsCopyUntilLatestLsn)
        K_END_COM_INTERFACE_LIST()

    public:

        static NTSTATUS Create(
            __in KAllocator & allocator,
            __in Data::Utilities::PartitionedReplicaId const & prId,
            __out ComStateProvider::SPtr & object);

        void Initialize(__in Data::LoggingReplicator::IStateProvider & innerStateProvider);

        STDMETHOD_(HRESULT, BeginUpdateEpoch)(
            __in FABRIC_EPOCH const * epoch,
            __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        STDMETHOD_(HRESULT, EndUpdateEpoch)(
            __in IFabricAsyncOperationContext * context) override;

        STDMETHOD_(HRESULT, GetLastCommittedSequenceNumber)(
            __out FABRIC_SEQUENCE_NUMBER * sequenceNumber) override;

        STDMETHOD_(HRESULT, BeginOnDataLoss)(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) override;

        STDMETHOD_(HRESULT, EndOnDataLoss)(
            __in IFabricAsyncOperationContext * context,
            __out BOOLEAN * isStateChanged) override;

        STDMETHOD_(HRESULT, GetCopyContext)(
            __out IFabricOperationDataStream ** copyContextStream) override;

        STDMETHOD_(HRESULT, GetCopyState)(
            __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            __in IFabricOperationDataStream * copyContextStream,
            __out IFabricOperationDataStream ** copyStateStream) override;

    private:

        ComStateProvider(__in Data::Utilities::PartitionedReplicaId const & traceId);

        Data::LoggingReplicator::IStateProvider::SPtr innerStateProvider_;
    };
}
