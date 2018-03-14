// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Communication::TcpServiceCommunication;

ComFabricServiceCommunicationClientFactory::ComFabricServiceCommunicationClientFactory(IServiceCommunicationClientFactoryPtr const & impl)
    : impl_(impl)
{

}

ComFabricServiceCommunicationClientFactory::~ComFabricServiceCommunicationClientFactory()
{
}

/* [entry] */ HRESULT ComFabricServiceCommunicationClientFactory::CreateFabricServiceCommunicationClient(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in LPCWSTR connectionAddress,
    /* [in] */ __RPC__in const FABRIC_SERVICE_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in IFabricCommunicationMessageHandler *notificationHandler,
    /* [in] */ __RPC__in IFabricServiceConnectionEventHandler *connectionEventHandler,
    /* [out, retval] */ __RPC__deref_out_opt IFabricServiceCommunicationClient ** client)
{

    if (riid != IID_IFabricServiceCommunicationClient && riid != IID_IFabricServiceCommunicationClient2)
    {
        return ComUtility::OnPublicApiReturn(E_NOINTERFACE);
    }

    if (client == NULL)
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    wstring address;
    auto hr = StringUtility::LpcwstrToWstring(connectionAddress, false, address);
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


    IServiceCommunicationClientPtr clientPtr;

    error = impl_->CreateServiceCommunicationClient(
        address,
        transportsettings,
        WrapperFactory::create_rooted_com_proxy(notificationHandler),
        WrapperFactory::create_rooted_com_proxy(connectionEventHandler),
        clientPtr);
        

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricServiceCommunicationClient> clientCPtr = WrapperFactory::create_com_wrapper(clientPtr);
    *client = clientCPtr.DetachNoRelease();
    return S_OK;
}
