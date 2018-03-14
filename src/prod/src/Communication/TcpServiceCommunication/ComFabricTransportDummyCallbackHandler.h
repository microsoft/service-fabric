// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ComFabricTransportDummyCallbackHandler : public IFabricTransportCallbackMessageHandler, private Common::ComUnknownBase
        {
            DENY_COPY(ComFabricTransportDummyCallbackHandler);

            BEGIN_COM_INTERFACE_LIST(ComFabricTransportDummyCallbackHandler)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportCallbackMessageHandler)
                COM_INTERFACE_ITEM(IID_IFabricTransportCallbackMessageHandler, IFabricTransportCallbackMessageHandler)
                END_COM_INTERFACE_LIST()

        public:
            ComFabricTransportDummyCallbackHandler();

            virtual HRESULT  HandleOneWay(
                /* [in] */ IFabricTransportMessage *message);

            bool IsMessageRecieved(Common::TimeSpan timespan);
        private:
            Common::ManualResetEvent recievedEvent;
        };
    }
}
