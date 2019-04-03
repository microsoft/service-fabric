// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Management::GatewayResourceManager;

GatewayResourceManagerAgentFactory::GatewayResourceManagerAgentFactory() { }

GatewayResourceManagerAgentFactory::~GatewayResourceManagerAgentFactory() { }

IGatewayResourceManagerAgentFactoryPtr GatewayResourceManagerAgentFactory::Create()
{
    shared_ptr<GatewayResourceManagerAgentFactory> factorySPtr(new GatewayResourceManagerAgentFactory());
    return RootedObjectPointer<IGatewayResourceManagerAgentFactory>(factorySPtr.get(), factorySPtr->CreateComponentRoot());
}

ErrorCode GatewayResourceManagerAgentFactory::CreateGatewayResourceManagerAgent(__out IGatewayResourceManagerAgentPtr & result)
{
    shared_ptr<GatewayResourceManagerAgent> agentSPtr;
    ErrorCode error = GatewayResourceManagerAgent::Create(agentSPtr);

    if (error.IsSuccess())
    {
        result = RootedObjectPointer<IGatewayResourceManagerAgent>(agentSPtr.get(), agentSPtr->CreateComponentRoot());
    }

    return error;
}
