// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComFaultAnalysisServiceAgentFactory
//

ComFaultAnalysisServiceAgentFactory::ComFaultAnalysisServiceAgentFactory(IFaultAnalysisServiceAgentFactoryPtr const & impl)
    : impl_(impl)
{
}

ComFaultAnalysisServiceAgentFactory::~ComFaultAnalysisServiceAgentFactory()
{
}

HRESULT ComFaultAnalysisServiceAgentFactory::CreateFabricFaultAnalysisServiceAgent(
    /* [in] */ __RPC__in REFIID riid,
    /* [out, retval] */ __RPC__deref_out_opt void ** fabricFaultAnalysisServiceAgent)
{
    if (riid != IID_IFabricFaultAnalysisServiceAgent) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
    if (fabricFaultAnalysisServiceAgent == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IFaultAnalysisServiceAgentPtr agentPtr;
    ErrorCode error = impl_->CreateFaultAnalysisServiceAgent(agentPtr);

    if (!error.IsSuccess()) 
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricFaultAnalysisServiceAgent> agentCPtr = WrapperFactory::create_com_wrapper(agentPtr);

    *fabricFaultAnalysisServiceAgent = agentCPtr.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}
