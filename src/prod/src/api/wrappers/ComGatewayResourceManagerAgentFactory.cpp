// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComGatewayResourceManagerAgentFactory
//

ComGatewayResourceManagerAgentFactory::ComGatewayResourceManagerAgentFactory(IGatewayResourceManagerAgentFactoryPtr const & impl)
    : impl_(impl)
{
}

ComGatewayResourceManagerAgentFactory::~ComGatewayResourceManagerAgentFactory()
{
}

HRESULT ComGatewayResourceManagerAgentFactory::CreateFabricGatewayResourceManagerAgent(
    /* [in] */ __RPC__in REFIID riid,
    /* [out, retval] */ __RPC__deref_out_opt void ** fabricGatewayResourceManagerAgent)
{
    if (riid != IID_IFabricGatewayResourceManagerAgent) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
    if (fabricGatewayResourceManagerAgent == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IGatewayResourceManagerAgentPtr agentPtr;
    ErrorCode error = impl_->CreateGatewayResourceManagerAgent(agentPtr);

    if (!error.IsSuccess()) 
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricGatewayResourceManagerAgent> agentCPtr = WrapperFactory::create_com_wrapper(agentPtr);

    *fabricGatewayResourceManagerAgent = agentCPtr.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}
