// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

BackupCopierJobItem::BackupCopierJobItem()
    : operation_()
{
}

BackupCopierJobItem::BackupCopierJobItem(BackupCopierJobItem && other)
    : operation_(move(other.operation_))
{
}

BackupCopierJobItem::BackupCopierJobItem(shared_ptr<BackupCopierAsyncOperationBase> && operation)
    : operation_(move(operation))
{
}

bool BackupCopierJobItem::ProcessJob(ComponentRoot &)
{
    operation_->OnProcessJob(operation_);

    return false;
}
