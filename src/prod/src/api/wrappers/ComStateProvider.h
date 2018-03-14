// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {DCDF794B-381D-49AE-9154-D8CC0011CB8B}
    static const GUID CLSID_ComStateProvider = 
    { 0xdcdf794b, 0x381d, 0x49ae, { 0x91, 0x54, 0xd8, 0xcc, 0x0, 0x11, 0xcb, 0x8b } };

    class ComStateProvider :
        public IFabricStateProvider,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComStateProvider)

        BEGIN_COM_INTERFACE_LIST(ComStateProvider)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStateProvider)
            COM_INTERFACE_ITEM(IID_IFabricStateProvider, IFabricStateProvider)
            COM_INTERFACE_ITEM(CLSID_ComStateProvider, ComStateProvider)
        END_COM_INTERFACE_LIST()

    public:
        ComStateProvider(IStateProviderPtr const & impl);
        virtual ~ComStateProvider();

        IStateProviderPtr const & get_Impl() const { return impl_; }

        // 
        // IFabricStateProvider methods
        // 
       HRESULT STDMETHODCALLTYPE BeginUpdateEpoch( 
            __in ::FABRIC_EPOCH const * epoch,
            __in ::FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
            __in IFabricAsyncOperationCallback *callback,
            __out IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUpdateEpoch( 
            __in IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE GetLastCommittedSequenceNumber(
            __out FABRIC_SEQUENCE_NUMBER * sequenceNumber);

        HRESULT STDMETHODCALLTYPE BeginOnDataLoss(
            __in IFabricAsyncOperationCallback * callback,
            __out IFabricAsyncOperationContext ** context);

        HRESULT STDMETHODCALLTYPE EndOnDataLoss(
            __in IFabricAsyncOperationContext * context,
            __out BOOLEAN * isStateChangedResult);

        HRESULT STDMETHODCALLTYPE GetCopyContext(
            __out IFabricOperationDataStream ** context);
    
        HRESULT STDMETHODCALLTYPE GetCopyState(
            __in FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
            __in IFabricOperationDataStream * context, 
            __out IFabricOperationDataStream ** enumerator);

    private:
        class UpdateEpochOperationContext;
        class OnDataLossOperationContext;

    private:
        IStateProviderPtr impl_;
    };
}
