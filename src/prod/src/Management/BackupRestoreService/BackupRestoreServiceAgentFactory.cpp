// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Management::BackupRestoreService;

BackupRestoreServiceAgentFactory::BackupRestoreServiceAgentFactory() { }

BackupRestoreServiceAgentFactory::~BackupRestoreServiceAgentFactory() { }

IBackupRestoreServiceAgentFactoryPtr BackupRestoreServiceAgentFactory::Create()
{
    shared_ptr<BackupRestoreServiceAgentFactory> factorySPtr(new BackupRestoreServiceAgentFactory());
    return RootedObjectPointer<IBackupRestoreServiceAgentFactory>(factorySPtr.get(), factorySPtr->CreateComponentRoot());
}

ErrorCode BackupRestoreServiceAgentFactory::CreateBackupRestoreServiceAgent(__out IBackupRestoreServiceAgentPtr & result)
{
    shared_ptr<BackupRestoreServiceAgent> agentSPtr;
    ErrorCode error = BackupRestoreServiceAgent::Create(agentSPtr);

    if (error.IsSuccess())
    {
        result = RootedObjectPointer<IBackupRestoreServiceAgent>(agentSPtr.get(), agentSPtr->CreateComponentRoot());
    }

    return error;
}
