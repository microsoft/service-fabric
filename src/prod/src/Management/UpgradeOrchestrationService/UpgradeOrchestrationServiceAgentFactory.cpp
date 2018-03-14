// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Management::UpgradeOrchestrationService;

UpgradeOrchestrationServiceAgentFactory::UpgradeOrchestrationServiceAgentFactory() { }

UpgradeOrchestrationServiceAgentFactory::~UpgradeOrchestrationServiceAgentFactory() { }

IUpgradeOrchestrationServiceAgentFactoryPtr UpgradeOrchestrationServiceAgentFactory::Create()
{
    shared_ptr<UpgradeOrchestrationServiceAgentFactory> factorySPtr(new UpgradeOrchestrationServiceAgentFactory());
    return RootedObjectPointer<IUpgradeOrchestrationServiceAgentFactory>(factorySPtr.get(), factorySPtr->CreateComponentRoot());
}

ErrorCode UpgradeOrchestrationServiceAgentFactory::CreateUpgradeOrchestrationServiceAgent(__out IUpgradeOrchestrationServiceAgentPtr & result)
{
    shared_ptr<UpgradeOrchestrationServiceAgent> agentSPtr;
    ErrorCode error = UpgradeOrchestrationServiceAgent::Create(agentSPtr);

    if (error.IsSuccess())
    {
        result = RootedObjectPointer<IUpgradeOrchestrationServiceAgent>(agentSPtr.get(), agentSPtr->CreateComponentRoot());
    }

    return error;
}

