// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ComFabricTransportDummyHandler : public IFabricTransportMessageHandler, private Common::ComUnknownBase
        {
            DENY_COPY(ComFabricTransportDummyHandler);

            BEGIN_COM_INTERFACE_LIST(ComFabricTransportDummyHandler)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportMessageHandler)
                COM_INTERFACE_ITEM(IID_IFabricTransportMessageHandler, IFabricTransportMessageHandler)
                END_COM_INTERFACE_LIST()

        public:
            ComFabricTransportDummyHandler(Common::ComPointer<IFabricTransportMessage>  reply);
         
            virtual HRESULT BeginProcessRequest(
                LPCWSTR clientId,
                IFabricTransportMessage *message,
                DWORD timeoutMilliseconds,
                IFabricAsyncOperationCallback * callback,
                IFabricAsyncOperationContext ** context);

            virtual HRESULT EndProcessRequest(
                IFabricAsyncOperationContext * context,
                IFabricTransportMessage ** reply);

            virtual HRESULT  HandleOneWay(
                /* [in] */   LPCWSTR clientId,
                /* [in] */ IFabricTransportMessage *message);

            private :
                Common::ComPointer<IFabricTransportMessage>  reply_;
                
        };
    }   
}
