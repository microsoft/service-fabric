// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace std;
using namespace Common;
using namespace Communication::TcpServiceCommunication;

INIT_ONCE ServiceCommunicationListenerFactory::initOnce_;
Global<ServiceCommunicationListenerFactory> ServiceCommunicationListenerFactory::singletonComunicationListenerFactory_;
static const StringLiteral FactoryTraceType("ServiceCommunicationListenerFactory");

ServiceCommunicationListenerFactory::ServiceCommunicationListenerFactory()
{
}

BOOL CALLBACK ServiceCommunicationListenerFactory::InitalizeCommunicationListenerFactory(PINIT_ONCE, PVOID, PVOID *)
{
    singletonComunicationListenerFactory_ = make_global<ServiceCommunicationListenerFactory>();
    return TRUE;
}

IServiceCommunicationListenerFactory& ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory()
{
    PVOID lpContext = NULL;
    BOOL result = InitOnceExecuteOnce(
        &initOnce_,
        InitalizeCommunicationListenerFactory,
        NULL,
        &lpContext);

    ASSERT_IF(!result, "Failed to initialize ServiceCommunicationListenerFactory");

    return *(singletonComunicationListenerFactory_);
}

ErrorCode ServiceCommunicationListenerFactory::CreateServiceCommunicationListener(
    ServiceCommunicationTransportSettingsUPtr && settings,
    std::wstring const& address,
    std::wstring const& path,
    IServiceCommunicationMessageHandlerPtr const & clientRequestHandler,
    IServiceConnectionHandlerPtr const & clientConnectionHandler,
    __out IServiceCommunicationListenerPtr & listener)
{
    IServiceCommunicationTransportSPtr transportSPtr = nullptr;
    ErrorCode error;
    {
        WriteInfo(FactoryTraceType, "Creating Listener for Address: {0} ,path: {1} ", address, path);

        AcquireWriteLock writeLock(knownTransportsMapLock_);
        if (knownTransports_.count(address) == 0)
        {
            error = CreateServiceCommunicationTransportCallerHoldsLock(move(settings), address, transportSPtr);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    FactoryTraceType,
                    "{0} Failed to Create Service Transport :",
                    error);
                return error;
            }
        }
        else
        {

            auto transportWPtr = knownTransports_[address];
            transportSPtr = transportWPtr.lock();
            if (!transportSPtr || transportSPtr->IsClosed())
            {
                error = CreateServiceCommunicationTransportCallerHoldsLock(move(settings), address, transportSPtr);

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        FactoryTraceType,
                        "{0} Failed to Create Service Transport :",
                        error);
                    return error;
                }
            }
           else
            {
                if (!(transportSPtr->GetSettings() == *settings))
                {
                    WriteWarning(
                        FactoryTraceType,
                        "{0} InCompatible  Transport Settings:",
                        ErrorCodeValue::AddressAlreadyInUse);
                    return ErrorCodeValue::AddressAlreadyInUse;
                }
                else
                {
                    WriteInfo(FactoryTraceType, "Using Existing Transport for this Listener {0}", path);
                }
           }
        }
    }

    error = CreateServiceCommunicationListenerInternal(
        path,
        clientRequestHandler,
        clientConnectionHandler,
        transportSPtr,
        listener);

    return error;
}

ErrorCode ServiceCommunicationListenerFactory::CreateServiceCommunicationTransportCallerHoldsLock(
    ServiceCommunicationTransportSettingsUPtr && settings,
    wstring const& address,
    __out IServiceCommunicationTransportSPtr &transportSPtr)
{
    ErrorCode error = ServiceCommunicationTransport::Create(move(settings),
                                                            address,
                                                            transportSPtr);
    if (!error.IsSuccess())
    {
        WriteWarning(
            FactoryTraceType,
            "{0} Failed to Create Service Transport :",
            error);       
        return error;
    }
    IServiceCommunicationTransportWPtr transportWPtr = transportSPtr;
    knownTransports_[address] = transportWPtr;

    return error;
}

ErrorCode ServiceCommunicationListenerFactory::CreateServiceCommunicationListenerInternal(
    wstring const& path,
    IServiceCommunicationMessageHandlerPtr const& clientRequestHandler,
    IServiceConnectionHandlerPtr const& clientConnectionHandler,
    IServiceCommunicationTransportSPtr const& transport,
    __out IServiceCommunicationListenerPtr & listener)
{
    ServiceCommunicationListenerSPtr listenerSPtr = make_shared<ServiceCommunicationListener>(transport);
                                                                                             
    listenerSPtr->CreateAndSetDispatcher(clientRequestHandler,
                                         clientConnectionHandler,
                                         path);

    listener = RootedObjectPointer<IServiceCommunicationListener>(listenerSPtr.get(), listenerSPtr->CreateComponentRoot());
     return ErrorCodeValue::Success;
}
