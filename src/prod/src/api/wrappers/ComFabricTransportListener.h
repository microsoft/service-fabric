// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
 
    // {8810C1DE-C786-4C89-9669-92C4839E9800}
    static const GUID CLSID_ComFabricTransportListener =
    { 0x8810c1de, 0xc786, 0x4c89,{ 0x96, 0x69, 0x92, 0xc4, 0x83, 0x9e, 0x98, 0x0 } };


    class ComFabricTransportListener
        : public  IFabricTransportListener,
          private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricTransportListener)

        BEGIN_COM_INTERFACE_LIST(ComFabricTransportListener)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportListener)
        COM_INTERFACE_ITEM(IID_IFabricTransportListener, IFabricTransportListener)
        COM_INTERFACE_ITEM(CLSID_ComFabricTransportListener, ComFabricTransportListener)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricTransportListener(IServiceCommunicationListenerPtr const & impl);

        virtual ~ComFabricTransportListener();

        //IFabricTransportListener methods //
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
