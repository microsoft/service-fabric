// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{

    // {ACABF40A-D39F-4A0B-B05A-269204FA506B}
    static const GUID CLSID_ComFabricTransportClient =
    { 0xacabf40a, 0xd39f, 0x4a0b,{ 0xb0, 0x5a, 0x26, 0x92, 0x4, 0xfa, 0x50, 0x6b } };

    class ComFabricTransportClient : public  IFabricTransportClient,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComFabricTransportClient)

            BEGIN_COM_INTERFACE_LIST(ComFabricTransportClient)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportClient)
            COM_INTERFACE_ITEM(IID_IFabricTransportClient, IFabricTransportClient)
            COM_INTERFACE_ITEM(CLSID_ComFabricTransportClient, ComFabricTransportClient)
            END_COM_INTERFACE_LIST()

    public:
        ComFabricTransportClient(IServiceCommunicationClientPtr const & impl,
            DeleteCallback const & deleteCallBack);
        
        virtual HRESULT BeginOpen(
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        virtual HRESULT EndOpen(
            /* [in] */   IFabricAsyncOperationContext * context);

        virtual HRESULT BeginClose(
            /* [in] */  DWORD timeoutMilliseconds,
            /* [in] */  IFabricAsyncOperationCallback * callback,
            /* [out, retval] */  IFabricAsyncOperationContext ** context);

        virtual HRESULT EndClose(
            /* [in] */  IFabricAsyncOperationContext * context);

        virtual void Abort(void);


        virtual    HRESULT BeginRequest(
            /* [in] */IFabricTransportMessage *message,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context);

        virtual HRESULT EndRequest(
            /* [in] */ IFabricAsyncOperationContext * context,
            /* [out, retval] */ IFabricTransportMessage **message);

        virtual HRESULT Send(
            /* [in] */ IFabricTransportMessage *message);


        virtual ~ComFabricTransportClient();

    private:
        
        void DisposeCallack(ComPointer<IFabricTransportMessage> message);

        IServiceCommunicationClientPtr impl_;
        DeleteCallback  deleteCallBack_;
    };
}
