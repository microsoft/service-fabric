// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Management::InfrastructureService;

InfrastructureServiceAgentFactory::InfrastructureServiceAgentFactory() { }

InfrastructureServiceAgentFactory::~InfrastructureServiceAgentFactory() { }

IInfrastructureServiceAgentFactoryPtr InfrastructureServiceAgentFactory::Create()
{
    shared_ptr<InfrastructureServiceAgentFactory> factorySPtr(new InfrastructureServiceAgentFactory());
    return RootedObjectPointer<IInfrastructureServiceAgentFactory>(factorySPtr.get(), factorySPtr->CreateComponentRoot());
}

ErrorCode InfrastructureServiceAgentFactory::CreateInfrastructureServiceAgent(__out IInfrastructureServiceAgentPtr & result)
{
    shared_ptr<InfrastructureServiceAgent> agentSPtr;
    ErrorCode error = InfrastructureServiceAgent::Create(agentSPtr);

    if (error.IsSuccess())
    {
        result = RootedObjectPointer<IInfrastructureServiceAgent>(agentSPtr.get(), agentSPtr->CreateComponentRoot());
    }

    return error;
}
