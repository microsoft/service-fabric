// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComUpgradeOrchestrationServiceAgentFactory
//

ComUpgradeOrchestrationServiceAgentFactory::ComUpgradeOrchestrationServiceAgentFactory(IUpgradeOrchestrationServiceAgentFactoryPtr const & impl)
    : impl_(impl)
{
}

ComUpgradeOrchestrationServiceAgentFactory::~ComUpgradeOrchestrationServiceAgentFactory()
{
}

HRESULT ComUpgradeOrchestrationServiceAgentFactory::CreateFabricUpgradeOrchestrationServiceAgent(
    /* [in] */ __RPC__in REFIID riid,
    /* [out, retval] */ __RPC__deref_out_opt void ** fabricUpgradeOrchestrationServiceAgent)
{
    if (riid != IID_IFabricUpgradeOrchestrationServiceAgent) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
    if (fabricUpgradeOrchestrationServiceAgent == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IUpgradeOrchestrationServiceAgentPtr agentPtr;
    ErrorCode error = impl_->CreateUpgradeOrchestrationServiceAgent(agentPtr);

    if (!error.IsSuccess()) 
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricUpgradeOrchestrationServiceAgent> agentCPtr = WrapperFactory::create_com_wrapper(agentPtr);
    *fabricUpgradeOrchestrationServiceAgent = agentCPtr.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}
