// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Api;
using namespace Communication::TcpServiceCommunication;

ComFabricServiceCommunicationListenerFactory::ComFabricServiceCommunicationListenerFactory(IServiceCommunicationListenerFactory  & impl)
    : impl_(impl)
{
    //Socket Startup to be called  before  parsing any endpoint string
    Common::Sockets::Startup();
}

ComFabricServiceCommunicationListenerFactory::~ComFabricServiceCommunicationListenerFactory()
{
}


/* [entry] */ HRESULT ComFabricServiceCommunicationListenerFactory::CreateFabricServiceCommunicationListener(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in const FABRIC_SERVICE_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in const FABRIC_SERVICE_LISTENER_ADDRESS * address,
    /* [in] */ __RPC__in  IFabricCommunicationMessageHandler *clientRequestHandler,
    /* [in] */ __RPC__in  IFabricServiceConnectionHandler *clientConnectionHandler,
    /* [out, retval] */ __RPC__deref_out_opt IFabricServiceCommunicationListener ** listener)
{
    if (riid != IID_IFabricServiceCommunicationListener) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
    if (listener == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring ipAddressOrFQDN;
    auto  hr = StringUtility::LpcwstrToWstring(address->IPAddressOrFQDN,
        false,
        ParameterValidator::MinStringSize,
        ParameterValidator::MaxStringSize,
        ipAddressOrFQDN);

    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
    wstring servicePath;
    hr = StringUtility::LpcwstrToWstring(address->Path,
        false,
        ParameterValidator::MinStringSize,
        ParameterValidator::MaxStringSize,
        servicePath);
    if (FAILED(hr))
    {
        return ComUtility::OnPublicApiReturn(hr);
    }

    ServiceCommunicationTransportSettingsUPtr transportsettings;
    auto error = ServiceCommunicationTransportSettings::FromPublicApi(*settings, transportsettings);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    IServiceCommunicationListenerPtr listenerPtr;
    auto listenAddress = GetListenerAddress(ipAddressOrFQDN, address->Port);

    error = impl_.CreateServiceCommunicationListener(move(transportsettings),
        listenAddress,
        servicePath,
        WrapperFactory::create_rooted_com_proxy(clientRequestHandler),
        WrapperFactory::create_rooted_com_proxy(clientConnectionHandler),
        listenerPtr);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricServiceCommunicationListener> listenerCPtr = WrapperFactory::create_com_wrapper(listenerPtr);
    *listener = listenerCPtr.DetachNoRelease();
    return S_OK;
}


wstring ComFabricServiceCommunicationListenerFactory::GetListenerAddress(
    wstring const & ipAddressOrFQDN,
    ULONG port)
{
    return TcpTransportUtility::ConstructAddressString(ipAddressOrFQDN, port);
}
