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

ComFabricTransportListenerFactory::ComFabricTransportListenerFactory(IServiceCommunicationListenerFactory  & impl)
: impl_(impl)
{
    //Socket Startup to be called  before  parsing any endpoint string
    Common::Sockets::Startup();
}

ComFabricTransportListenerFactory::~ComFabricTransportListenerFactory()
{
}


/* [entry] */ HRESULT ComFabricTransportListenerFactory::CreateFabricTransportListener(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in const FABRIC_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in const FABRIC_TRANSPORT_LISTEN_ADDRESS * address,
    /* [in] */ __RPC__in  IFabricTransportMessageHandler *clientRequestHandler,
    /* [in] */ __RPC__in  IFabricTransportConnectionHandler *clientConnectionHandler,
    /* [in] */ __RPC__in  IFabricTransportMessageDisposer *disposeProcessor,
    /* [out, retval] */ __RPC__deref_out_opt IFabricTransportListener ** listener)
{
    if (riid != IID_IFabricTransportListener) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
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
    Common::ComPointer<IFabricTransportMessageDisposer> comdisposerImplPtr;
    comdisposerImplPtr.SetAndAddRef(disposeProcessor);
    Common::ComPointer<IFabricTransportMessageHandler> comImplCPtr;
    comImplCPtr.SetAndAddRef(clientRequestHandler);
    auto proxy = std::make_shared<ComProxyFabricTransportMessageHandler>(comImplCPtr, comdisposerImplPtr);
    
    auto rootedProxy = Common::RootedObjectPointer<IServiceCommunicationMessageHandler>((IServiceCommunicationMessageHandler*)proxy.get(), proxy->CreateComponentRoot());

    error = impl_.CreateServiceCommunicationListener(move(transportsettings),
                                                     listenAddress,
                                                     servicePath,
                                                     rootedProxy,
                                                     WrapperFactory::create_rooted_com_proxy(clientConnectionHandler),
                                                     listenerPtr);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    //Not using Wrapperfactory as there is already function for v1 stack using same parameter
    ComPointer<ComFabricTransportListener> listenerCPtr = make_com<ComFabricTransportListener>(listenerPtr);
    *listener = listenerCPtr.DetachNoRelease();
    return S_OK;
}


wstring ComFabricTransportListenerFactory::GetListenerAddress(wstring const & ipAddressOrFQDN,
                                                                         ULONG port)
{
    return TcpTransportUtility::ConstructAddressString(ipAddressOrFQDN, port);
}
