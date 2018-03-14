// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComFabricServiceCommunicationClientFactory
    {
    public:
        ComFabricServiceCommunicationClientFactory(IServiceCommunicationClientFactoryPtr const & impl);
        virtual ~ComFabricServiceCommunicationClientFactory();

        /* [entry] */ HRESULT CreateFabricServiceCommunicationClient(
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ __RPC__in  LPCWSTR connectionAddress,
            /* [in] */ __RPC__in const FABRIC_SERVICE_TRANSPORT_SETTINGS *settings,
            /* [in] */ __RPC__in IFabricCommunicationMessageHandler *notificationHandler,
            /* [in] */ __RPC__in IFabricServiceConnectionEventHandler *connectionEventHandler,
            /* [out, retval] */ __RPC__deref_out_opt IFabricServiceCommunicationClient ** client);

    private:
        IServiceCommunicationClientFactoryPtr impl_;
    };
}
