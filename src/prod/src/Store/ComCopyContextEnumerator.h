// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ComCopyContextEnumerator
        : public IFabricOperationDataStream 
        , public Common::ComUnknownBase
        , public ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        COM_INTERFACE_LIST1(
            ComCopyContextEnumerator,
            IID_IFabricOperationDataStream,
            IFabricOperationDataStream)

    public:
        ComCopyContextEnumerator(
            Store::PartitionedReplicaId const &,
            bool isEpochValid,
            ::FABRIC_EPOCH const & currentEpoch,
            ::FABRIC_SEQUENCE_NUMBER lastOperationLSN,
            Common::ComponentRoot const & root);
        ~ComCopyContextEnumerator();

        HRESULT STDMETHODCALLTYPE BeginGetNext(__in IFabricAsyncOperationCallback *, __out IFabricAsyncOperationContext **);
        HRESULT STDMETHODCALLTYPE EndGetNext(__in IFabricAsyncOperationContext *, __out IFabricOperationData **);

    private:
        class ComCopyContextContext;

        bool isEpochValid_;
        ::FABRIC_EPOCH currentEpoch_;
        ::FABRIC_SEQUENCE_NUMBER lastOperationLSN_;
        Common::ComponentRootSPtr root_; 
        bool isContextSent_;
    };
}
