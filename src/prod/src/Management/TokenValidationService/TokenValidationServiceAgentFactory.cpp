// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Management::TokenValidationService;

TokenValidationServiceAgentFactory::TokenValidationServiceAgentFactory() { }

TokenValidationServiceAgentFactory::~TokenValidationServiceAgentFactory() { }

ITokenValidationServiceAgentFactoryPtr TokenValidationServiceAgentFactory::Create()
{
    shared_ptr<TokenValidationServiceAgentFactory> factorySPtr(new TokenValidationServiceAgentFactory());
    return RootedObjectPointer<ITokenValidationServiceAgentFactory>(factorySPtr.get(), factorySPtr->CreateComponentRoot());
}

ErrorCode TokenValidationServiceAgentFactory::CreateTokenValidationServiceAgent(__out ITokenValidationServiceAgentPtr & result)
{
    shared_ptr<TokenValidationServiceAgent> agentSPtr;
    ErrorCode error = TokenValidationServiceAgent::Create(agentSPtr);

    if (error.IsSuccess())
    {
        result = RootedObjectPointer<ITokenValidationServiceAgent>(agentSPtr.get(), agentSPtr->CreateComponentRoot());
    }

    return error;
}
