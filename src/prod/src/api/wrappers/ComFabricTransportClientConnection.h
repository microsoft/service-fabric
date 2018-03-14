// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {4420D1E3-2878-43B4-B626-F7591D35886D}
    static const GUID CLSID_ComFabricTransportClientConnection =
    { 0x4420d1e3, 0x2878, 0x43b4,{ 0xb6, 0x26, 0xf7, 0x59, 0x1d, 0x35, 0x88, 0x6d } };

    class ComFabricTransportClientConnection 
        : public  IFabricTransportClientConnection,
          private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricTransportClientConnection)

        BEGIN_COM_INTERFACE_LIST(ComFabricTransportClientConnection)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportClientConnection)
        COM_INTERFACE_ITEM(IID_IFabricTransportClientConnection, IFabricTransportClientConnection)
        COM_INTERFACE_ITEM(CLSID_ComFabricTransportClientConnection, ComFabricTransportClientConnection)
        END_COM_INTERFACE_LIST()

    public:
        ComFabricTransportClientConnection(IClientConnectionPtr const & impl);
       
        virtual HRESULT  Send(
            /* [in] */ IFabricTransportMessage *message);

        virtual  COMMUNICATION_CLIENT_ID   get_ClientId();
    private:
        IClientConnectionPtr impl_;
    };
}
