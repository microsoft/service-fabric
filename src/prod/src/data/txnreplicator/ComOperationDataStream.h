// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // The Com OperationData Stream implements the IFabricOperationDataStream interface and converts the internal KTL async API's to com based API's
    // These are primarily used by GetCopyState and GetCopyContext
    //
    class ComOperationDataStream final
        : public KObject<ComOperationDataStream>
        , public KShared<ComOperationDataStream>
        , public IFabricOperationDataStream
        , public IFabricDisposable
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(ComOperationDataStream)

        K_BEGIN_COM_INTERFACE_LIST(ComOperationDataStream)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationDataStream)
            COM_INTERFACE_ITEM(IID_IFabricOperationDataStream, IFabricOperationDataStream)
            COM_INTERFACE_ITEM(IID_IFabricDisposable, IFabricDisposable)
        K_END_COM_INTERFACE_LIST()

    public:
 
        static NTSTATUS Create(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in IOperationDataStream & innerDataStream,
            __in KAllocator & allocator,
            __out Common::ComPointer<IFabricOperationDataStream> & object) noexcept;

        STDMETHOD_(HRESULT, BeginGetNext)(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context) noexcept override;

        STDMETHOD_(HRESULT, EndGetNext)(
            __in IFabricAsyncOperationContext * context,
            __out IFabricOperationData **operationData) noexcept override;

        STDMETHOD_(void, Dispose)(void) override;

    private:

        ComOperationDataStream(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in IOperationDataStream & innerOperationDataStream);

        IOperationDataStream::SPtr innerOperationDataStream_;
    };
}
