// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Communication::TcpServiceCommunication;

IServiceCommunicationClientFactoryPtr ServiceCommunicationClientFactory::Create()
{
    shared_ptr<ServiceCommunicationClientFactory> factorySPtr(new ServiceCommunicationClientFactory());
    return RootedObjectPointer<IServiceCommunicationClientFactory>(factorySPtr.get(), factorySPtr->CreateComponentRoot());
}

ErrorCode ServiceCommunicationClientFactory::CreateServiceCommunicationClient(
    wstring const & address,
    ServiceCommunicationTransportSettingsUPtr const & setting,
    IServiceCommunicationMessageHandlerPtr const & notificationHandler,
    IServiceConnectionEventHandlerPtr const & connectionEventHandler,
    __out IServiceCommunicationClientPtr & result,
    bool explicitConnect)
{
    auto clientSptr = make_shared<ServiceCommunicationClient>(setting,
                                                              address,
                                                              notificationHandler,
                                                              connectionEventHandler,
                                                              explicitConnect);
    auto error = clientSptr->Open();
    if (error.IsSuccess())
    {
        result = RootedObjectPointer<IServiceCommunicationClient>(clientSptr.get(), clientSptr->CreateComponentRoot());
    }
    return error;
}
