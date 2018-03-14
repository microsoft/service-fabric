// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace TestCommon;

extern ComPointer<IFabricServiceManagementClient> CreateFabricServiceClient(__in FabricTestFederation & testFederation);

ServiceLocationChangeClientManager::ServiceLocationChangeClientManager()
    : namedClients_()
    , defaultNamedClient_()
    , lock_()
{
}

void ServiceLocationChangeClientManager::GetDefaultNamedClient(__out ServiceLocationChangeClientSPtr & client)
{
    AcquireExclusiveLock lock(lock_);
    if (!defaultNamedClient_)
    {
        defaultNamedClient_ = make_shared<ServiceLocationChangeClient>(
            CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation));
    }

    client = defaultNamedClient_;
}

void ServiceLocationChangeClientManager::ResetDefaultNamedClient()
{
    AcquireExclusiveLock lock(lock_);
    defaultNamedClient_.reset();
}

bool ServiceLocationChangeClientManager::AddClient(
    std::wstring const & clientName,
    ComPointer<IFabricServiceManagementClient> && client)
{
    if (namedClients_.end() != namedClients_.find(clientName))
    {
        TestSession::WriteError(TraceSource, "Client with name: {0} already exists.", clientName);
        return false;
    }

    ComPointer<IFabricServiceManagementClient> nameClient = std::move(client);
    if (!nameClient)
    {
        nameClient = CreateFabricServiceClient(FABRICSESSION.FabricDispatcher.Federation);
    }

    namedClients_.insert(make_pair(
        clientName, 
        make_shared<ServiceLocationChangeClient>(nameClient)));

        return true;
}

bool ServiceLocationChangeClientManager::DeleteClient(std::wstring const & clientName)
{
    std::map<std::wstring, ServiceLocationChangeClientSPtr>::iterator iteratorClient = namedClients_.find(clientName);
    if (namedClients_.end() == iteratorClient)
    {
        TestSession::WriteError(TraceSource, "Client with name: {0} does not exist.", clientName);
        return false;
    }
    
    namedClients_.erase(iteratorClient);
    return true;
}

bool ServiceLocationChangeClientManager::FindClient(
    std::wstring const & clientName,
    ServiceLocationChangeClientSPtr & client)
{
    std::map<std::wstring, ServiceLocationChangeClientSPtr>::iterator iteratorClient = namedClients_.find(clientName);
    if (namedClients_.end() != iteratorClient)
    {
        client = iteratorClient->second;
        return true;
    }
    return false;
}

const Common::StringLiteral ServiceLocationChangeClientManager::TraceSource("FabricTest.ServiceLocationChangeClientManager");
