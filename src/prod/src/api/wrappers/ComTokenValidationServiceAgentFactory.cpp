// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComTokenValidationServiceAgentFactory
//

ComTokenValidationServiceAgentFactory::ComTokenValidationServiceAgentFactory(ITokenValidationServiceAgentFactoryPtr const & impl)
    : impl_(impl)
{
}

ComTokenValidationServiceAgentFactory::~ComTokenValidationServiceAgentFactory()
{
}

HRESULT ComTokenValidationServiceAgentFactory::CreateFabricTokenValidationServiceAgent(
    /* [in] */ __RPC__in REFIID riid,
    /* [out, retval] */ __RPC__deref_out_opt void ** fabricTokenValidationServiceAgent)
{
    if (riid != IID_IFabricTokenValidationServiceAgent) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
    UNREFERENCED_PARAMETER(riid);
    if (fabricTokenValidationServiceAgent == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    ITokenValidationServiceAgentPtr agentPtr;
    ErrorCode error = impl_->CreateTokenValidationServiceAgent(agentPtr);

    if (!error.IsSuccess()) 
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricTokenValidationServiceAgent> agentCPtr = WrapperFactory::create_com_wrapper(agentPtr);

    *fabricTokenValidationServiceAgent = agentCPtr.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}
