// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ComDummyService : public IFabricCommunicationMessageHandler, private Common::ComUnknownBase
        {
            DENY_COPY(ComDummyService);

            BEGIN_COM_INTERFACE_LIST(ComDummyService)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricCommunicationMessageHandler)
                COM_INTERFACE_ITEM(IID_IFabricCommunicationMessageHandler, IFabricCommunicationMessageHandler)
                END_COM_INTERFACE_LIST()

        public:
            ComDummyService(Common::ComPointer<IFabricServiceCommunicationMessage>  reply);
            virtual HRESULT BeginProcessRequest(
                LPCWSTR clientId,
                IFabricServiceCommunicationMessage *message,
                DWORD timeoutMilliseconds,
                IFabricAsyncOperationCallback * callback,
                IFabricAsyncOperationContext ** context);

            virtual HRESULT EndProcessRequest(
                IFabricAsyncOperationContext * context,
                IFabricServiceCommunicationMessage ** reply);

            virtual HRESULT  HandleOneWay(
                /* [in] */   LPCWSTR clientId,
                /* [in] */ IFabricServiceCommunicationMessage *message);

            private :
                Common::ComPointer<IFabricServiceCommunicationMessage>  reply_;
                
        };
    }   
}
