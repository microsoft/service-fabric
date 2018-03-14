// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{

    // {2D8F5A4A-85AC-4066-80A1-502C02230517}
    static const GUID CLSID_ComFabricServiceCommunicationClient =
    { 0x2d8f5a4a, 0x85ac, 0x4066, { 0x80, 0xa1, 0x50, 0x2c, 0x2, 0x23, 0x5, 0x17 } };

    class ComFabricServiceCommunicationClient : public  IFabricServiceCommunicationClient2, 
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricServiceCommunicationClient)

        BEGIN_COM_INTERFACE_LIST(ComFabricServiceCommunicationClient)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricServiceCommunicationClient)
        COM_INTERFACE_ITEM(IID_IFabricCommunicationMessageSender, IFabricCommunicationMessageSender)
        COM_INTERFACE_ITEM(IID_IFabricServiceCommunicationClient, IFabricServiceCommunicationClient)
        COM_INTERFACE_ITEM(IID_IFabricServiceCommunicationClient2, IFabricServiceCommunicationClient2)
        COM_INTERFACE_ITEM(CLSID_ComFabricServiceCommunicationClient, ComFabricServiceCommunicationClient)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricServiceCommunicationClient(IServiceCommunicationClientPtr const & impl);

        /* IFabricCommunicationMessageSender Methods */
        virtual    HRESULT BeginRequest(
            /* [in] */IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        virtual HRESULT EndRequest(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IFabricServiceCommunicationMessage **message);

        virtual HRESULT SendMessage(
            /* [in] */ IFabricServiceCommunicationMessage *message);

        /* IFabricCommunicationMessageClient2 Methods */

        virtual void Abort(void);

        virtual ~ComFabricServiceCommunicationClient();
        
    private:
        IServiceCommunicationClientPtr impl_;
     };
}
