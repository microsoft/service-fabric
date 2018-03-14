// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComInfrastructureServiceAgentFactory
//

ComInfrastructureServiceAgentFactory::ComInfrastructureServiceAgentFactory(IInfrastructureServiceAgentFactoryPtr const & impl)
    : impl_(impl)
{
}

ComInfrastructureServiceAgentFactory::~ComInfrastructureServiceAgentFactory()
{
}

HRESULT ComInfrastructureServiceAgentFactory::CreateFabricInfrastructureServiceAgent(
    /* [in] */ __RPC__in REFIID riid,
    /* [out, retval] */ __RPC__deref_out_opt void ** fabricInfrastructureServiceAgent)
{
    if (riid != IID_IFabricInfrastructureServiceAgent) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
    if (fabricInfrastructureServiceAgent == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IInfrastructureServiceAgentPtr agentPtr;
    ErrorCode error = impl_->CreateInfrastructureServiceAgent(agentPtr);

    if (!error.IsSuccess()) 
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricInfrastructureServiceAgent> agentCPtr = WrapperFactory::create_com_wrapper(agentPtr);

    *fabricInfrastructureServiceAgent = agentCPtr.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}
