// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;


// ********************************************************************************************************************
// ComBackupRestoreServiceAgentFactory
//

ComBackupRestoreServiceAgentFactory::ComBackupRestoreServiceAgentFactory(IBackupRestoreServiceAgentFactoryPtr const & impl)
    : impl_(impl)
{
}

ComBackupRestoreServiceAgentFactory::~ComBackupRestoreServiceAgentFactory()
{
}

HRESULT ComBackupRestoreServiceAgentFactory::CreateFabricBackupRestoreServiceAgent(
    /* [in] */ __RPC__in REFIID riid,
    /* [out, retval] */ __RPC__deref_out_opt void ** fabricBackupRestoreServiceAgent)
{
    if (riid != IID_IFabricBackupRestoreServiceAgent) { return ComUtility::OnPublicApiReturn(E_NOINTERFACE); }
    if (fabricBackupRestoreServiceAgent == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    IBackupRestoreServiceAgentPtr agentPtr;
    ErrorCode error = impl_->CreateBackupRestoreServiceAgent(agentPtr);

    if (!error.IsSuccess()) 
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }

    ComPointer<IFabricBackupRestoreServiceAgent> agentCPtr = WrapperFactory::create_com_wrapper(agentPtr);

    *fabricBackupRestoreServiceAgent = agentCPtr.DetachNoRelease();

    return ComUtility::OnPublicApiReturn(S_OK);
}
