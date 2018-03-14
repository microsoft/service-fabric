// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComFabricTransportClientFactory 
    {
    public:
        ComFabricTransportClientFactory( /* [in] */ __RPC__in  IFabricTransportMessageDisposer *diposeProcessor);
        virtual ~ComFabricTransportClientFactory();

        static ComFabricTransportClientFactory& GetComFabricTransportClientFactory( /* [in] */ __RPC__in  IFabricTransportMessageDisposer *diposeProcessor);

        /* [entry] */ HRESULT CreateFabricTransportClient(
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ __RPC__in  LPCWSTR connectionAddress,
            /* [in] */ __RPC__in const FABRIC_TRANSPORT_SETTINGS *settings,
            /* [in] */ __RPC__in IFabricTransportCallbackMessageHandler *notificationHandler,
            /* [in] */ __RPC__in IFabricTransportClientEventHandler *connectionEventHandler,
            /* [out, retval] */ __RPC__deref_out_opt IFabricTransportClient ** client);

    private:
        IServiceCommunicationClientFactoryPtr impl_;
        static BOOL CALLBACK InitalizeComFabricTransportClientFactory(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE initOnce_;
        static Common::Global<ComFabricTransportClientFactory> singeltoncomFabricTransportClientFactory;
        std::unique_ptr<Common::BatchJobQueue<ComFabricTransportMessageCPtr, Common::ComponentRoot>> disposeQueue_;
        Common::ComPointer<IFabricTransportMessageDisposer>  comDisposerImpl_;

    };
}
