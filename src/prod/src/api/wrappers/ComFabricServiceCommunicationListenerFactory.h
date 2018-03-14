// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComFabricServiceCommunicationListenerFactory
    {
    public:
        ComFabricServiceCommunicationListenerFactory(IServiceCommunicationListenerFactory  & impl);

        virtual ~ComFabricServiceCommunicationListenerFactory();

        /* [entry] */ HRESULT CreateFabricServiceCommunicationListener(
            /* [in] */ __RPC__in REFIID riid,
            /* [in] */ __RPC__in const FABRIC_SERVICE_TRANSPORT_SETTINGS *settings,
            /* [in] */ __RPC__in const FABRIC_SERVICE_LISTENER_ADDRESS * address,
            /* [in] */ __RPC__in  IFabricCommunicationMessageHandler *clientRequestHandler,
            /* [in] */ __RPC__in  IFabricServiceConnectionHandler *clientConnectionHandler,
            /* [out, retval] */ __RPC__deref_out_opt IFabricServiceCommunicationListener ** listener);

    private:
        IServiceCommunicationListenerFactory & impl_;
		ComFabricServiceCommunicationListenerFactory& operator=(const ComFabricServiceCommunicationListenerFactory &tmp);
        std::wstring GetListenerAddress(std::wstring const & ipAddressOrFQDN,
                                        ULONG port);
    };
}
