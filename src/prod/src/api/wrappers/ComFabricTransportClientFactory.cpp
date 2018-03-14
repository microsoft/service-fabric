// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Communication::TcpServiceCommunication;

INIT_ONCE ComFabricTransportClientFactory::initOnce_;
Global<ComFabricTransportClientFactory> ComFabricTransportClientFactory::singeltoncomFabricTransportClientFactory;


ComFabricTransportClientFactory::ComFabricTransportClientFactory( /* [in] */ __RPC__in  IFabricTransportMessageDisposer *disposeProcessor)
{
    auto root = make_shared<ComponentRoot>();
    this->comDisposerImpl_.SetAndAddRef(disposeProcessor);
    this->impl_ = Communication::TcpServiceCommunication::ServiceCommunicationClientFactory::Create();
    this->disposeQueue_ = make_unique<BatchJobQueue<ComFabricTransportMessageCPtr,
        ComponentRoot>>(
            L"DisposeQueue",
            [this](vector<ComFabricTransportMessageCPtr> & items, ComponentRoot & root)
    {
        ComProxyFabricTransportDisposer disposer(this->comDisposerImpl_);
        disposer.ProcessJob(items, root);
    },
        *root,
        false,
        0);
}

ComFabricTransportClientFactory::~ComFabricTransportClientFactory()
{
}



BOOL CALLBACK ComFabricTransportClientFactory::InitalizeComFabricTransportClientFactory(PINIT_ONCE, PVOID parameter, PVOID *)
{

    singeltoncomFabricTransportClientFactory = make_global<ComFabricTransportClientFactory>(reinterpret_cast<IFabricTransportMessageDisposer*>(parameter));
    return TRUE;
}

ComFabricTransportClientFactory& ComFabricTransportClientFactory::GetComFabricTransportClientFactory(/* [in] */ __RPC__in  IFabricTransportMessageDisposer *disposeProcessor)
{
    PVOID lpContext = NULL;
    BOOL result = InitOnceExecuteOnce(
        &initOnce_,
        InitalizeComFabricTransportClientFactory,
        disposeProcessor,
        &lpContext);

    ASSERT_IF(!result, "Failed to initialize ServiceCommunicationListenerFactory");

    return *(singeltoncomFabricTransportClientFactory);
}


/* [entry] */ HRESULT ComFabricTransportClientFactory::CreateFabricTransportClient(
    /* [in] */ __RPC__in REFIID riid,
    /* [in] */ __RPC__in  LPCWSTR connectionAddress,
    /* [in] */ __RPC__in const FABRIC_TRANSPORT_SETTINGS *settings,
    /* [in] */ __RPC__in IFabricTransportCallbackMessageHandler *notificationHandler,
    /* [in] */ __RPC__in IFabricTransportClientEventHandler *connectionEventHandler,
    /* [out, retval] */ __RPC__deref_out_opt IFabricTransportClient ** client)
{

    if (riid != IID_IFabricTransportClient)
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
        clientPtr,
        true);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricTransportClient> clientCPtr = make_com<ComFabricTransportClient, IFabricTransportClient>(clientPtr, [this](ComPointer<IFabricTransportMessage> message) {

        if (!this->disposeQueue_->Enqueue(move(message)))
        {//if Enqueue Fails(e.g Queue Full), then Dispose will be called on the same threads
            message->Dispose();
        }
    });
    *client = clientCPtr.DetachNoRelease();
    return S_OK;
}
