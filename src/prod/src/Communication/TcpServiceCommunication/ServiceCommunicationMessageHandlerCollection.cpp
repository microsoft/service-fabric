// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Transport;
using namespace Communication::TcpServiceCommunication;
using namespace std;

static const StringLiteral ServiceCommunicationMessageHandlerCollectionTraceType("ServiceCommunicationMessageHandlerCollection");

ErrorCode ServiceCommunicationMessageHandlerCollection::RegisterMessageHandler(
    ServiceMethodCallDispatcherSPtr const & dispatcher)
{
    AcquireWriteLock writeLock(serviceRegistrationLock_);

    if (serviceRegistration_.find(dispatcher->ServiceInfo) != serviceRegistration_.end())
    {
        WriteWarning(ServiceCommunicationMessageHandlerCollectionTraceType, "Service Registration Fails as service is already existing with this address {0}", dispatcher->ServiceInfo);
        return ErrorCodeValue::AlreadyExists;
    }
        
    serviceRegistration_[dispatcher->ServiceInfo] = dispatcher;
    return ErrorCodeValue::Success;
}

ServiceMethodCallDispatcherSPtr ServiceCommunicationMessageHandlerCollection::GetMessageHandler(std::wstring const & location) const
{
    AcquireReadLock readLock(serviceRegistrationLock_);
    auto iter = serviceRegistration_.find(location);
    if ( iter!= serviceRegistration_.end())
    {
        return  iter->second;
    }
    return  nullptr;
}

void ServiceCommunicationMessageHandlerCollection::UnregisterMessageHandler(std::wstring const &location)
{
    WriteInfo(ServiceCommunicationMessageHandlerCollectionTraceType, "Unregistering Dispatcher for service location:{0}", location);

    ServiceMethodCallDispatcherSPtr servicedispatcher;
    {
        AcquireReadLock readLock(serviceRegistrationLock_);

        if (serviceRegistration_.find(location) != serviceRegistration_.end())
        {
            servicedispatcher = serviceRegistration_[location];
        }
        else
        {
            return;
        }
    }
    servicedispatcher->Close();
    {
        AcquireWriteLock writelock(serviceRegistrationLock_);
        serviceRegistration_.erase(location);
    }
    WriteInfo(ServiceCommunicationMessageHandlerCollectionTraceType, "Unregistered Dispatcher for service location: {0}", location);
}
