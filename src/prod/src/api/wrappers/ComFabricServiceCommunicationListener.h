// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {704AB7E6-4B1D-4E51-9591-153E19C08368}
    static const GUID CLSID_ComFabricServiceCommunicationListener =
    { 0x704ab7e6, 0x4b1d, 0x4e51, { 0x95, 0x91, 0x15, 0x3e, 0x19, 0xc0, 0x83, 0x68 } };

    class ComFabricServiceCommunicationListener
        : public  IFabricServiceCommunicationListener,
          private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricServiceCommunicationListener)

        BEGIN_COM_INTERFACE_LIST(ComFabricServiceCommunicationListener)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricServiceCommunicationListener)
        COM_INTERFACE_ITEM(IID_IFabricServiceCommunicationListener, IFabricServiceCommunicationListener)
        COM_INTERFACE_ITEM(CLSID_ComFabricServiceCommunicationListener, ComFabricServiceCommunicationListener)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricServiceCommunicationListener(IServiceCommunicationListenerPtr const & impl);

        virtual ~ComFabricServiceCommunicationListener();

        //IFabricServiceCommunicationListener methods //
        virtual HRESULT  BeginOpen(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT  EndOpen(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricStringResult **serviceAddress);

        virtual HRESULT  BeginClose(
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT  EndClose(
            /* [in] */ IFabricAsyncOperationContext *context);

        virtual void  Abort(void);

    private:
        IServiceCommunicationListenerPtr impl_;
        class OpenOperationContext;
        class CloseOperationContext;

    };


}
