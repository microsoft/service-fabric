// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {69C7AEDA-13B9-46CC-BFCE-5B61FCED834C}
    static const GUID  CLSID_ComFabricClientConnection =
    { 0x69c7aeda, 0x13b9, 0x46cc, { 0xbf, 0xce, 0x5b, 0x61, 0xfc, 0xed, 0x83, 0x4c } };

    class ComFabricClientConnection 
        : public  IFabricClientConnection,
          private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricClientConnection)

        BEGIN_COM_INTERFACE_LIST(ComFabricClientConnection)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricClientConnection)
        COM_INTERFACE_ITEM(IID_IFabricCommunicationMessageSender, IFabricCommunicationMessageSender)
        COM_INTERFACE_ITEM(IID_IFabricClientConnection, IFabricClientConnection)
        COM_INTERFACE_ITEM(CLSID_ComFabricClientConnection, ComFabricClientConnection)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricClientConnection(IClientConnectionPtr const & impl);

        virtual HRESULT  BeginRequest(
            /* [in] */ IFabricServiceCommunicationMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback *callback,
            /* [retval][out] */ IFabricAsyncOperationContext **context);

        virtual HRESULT  EndRequest(
            /* [in] */ IFabricAsyncOperationContext *context,
            /* [retval][out] */ IFabricServiceCommunicationMessage **reply);

        virtual HRESULT  SendMessage(
            /* [in] */ IFabricServiceCommunicationMessage *message);

        virtual  COMMUNICATION_CLIENT_ID   get_ClientId();
    private:
        IClientConnectionPtr impl_;
    };
}
