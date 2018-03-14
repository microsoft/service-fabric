// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComFabricTransportListenerFactory
    {
    public:
        ComFabricTransportListenerFactory(IServiceCommunicationListenerFactory  & impl);

        virtual ~ComFabricTransportListenerFactory();

        HRESULT CreateFabricTransportListener(
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ __RPC__in const FABRIC_TRANSPORT_SETTINGS *settings,
            /* [in] */ __RPC__in const FABRIC_TRANSPORT_LISTEN_ADDRESS * address,
            /* [in] */ __RPC__in  IFabricTransportMessageHandler *clientRequestHandler,
            /* [in] */ __RPC__in  IFabricTransportConnectionHandler *clientConnectionHandler,
            /* [in] */ __RPC__in  IFabricTransportMessageDisposer *diposeProcessor,
            /* [out, retval] */ __RPC__deref_out_opt IFabricTransportListener ** listener);

    private:
        IServiceCommunicationListenerFactory & impl_;
        ComFabricTransportListenerFactory& operator=(const ComFabricTransportListenerFactory &tmp);
        std::wstring GetListenerAddress(std::wstring const & ipAddressOrFQDN,
                                        ULONG port);
    };
}
