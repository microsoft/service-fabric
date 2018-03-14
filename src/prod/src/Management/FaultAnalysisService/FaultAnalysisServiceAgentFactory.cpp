// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Management::FaultAnalysisService;

FaultAnalysisServiceAgentFactory::FaultAnalysisServiceAgentFactory() { }

FaultAnalysisServiceAgentFactory::~FaultAnalysisServiceAgentFactory() { }

IFaultAnalysisServiceAgentFactoryPtr FaultAnalysisServiceAgentFactory::Create()
{
    shared_ptr<FaultAnalysisServiceAgentFactory> factorySPtr(new FaultAnalysisServiceAgentFactory());
    return RootedObjectPointer<IFaultAnalysisServiceAgentFactory>(factorySPtr.get(), factorySPtr->CreateComponentRoot());
}

ErrorCode FaultAnalysisServiceAgentFactory::CreateFaultAnalysisServiceAgent(__out IFaultAnalysisServiceAgentPtr & result)
{
    shared_ptr<FaultAnalysisServiceAgent> agentSPtr;
    ErrorCode error = FaultAnalysisServiceAgent::Create(agentSPtr);

    if (error.IsSuccess())
    {
        result = RootedObjectPointer<IFaultAnalysisServiceAgent>(agentSPtr.get(), agentSPtr->CreateComponentRoot());
    }

    return error;
}

