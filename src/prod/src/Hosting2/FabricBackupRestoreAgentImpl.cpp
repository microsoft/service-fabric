// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

FabricBackupRestoreAgentImpl::FabricBackupRestoreAgentImpl(
    ComponentRoot const & parent,
    __in ApplicationHost & host)
    : ComponentRoot(),
    parent_(parent.CreateComponentRoot()),
    host_(host)
{
}

FabricBackupRestoreAgentImpl::~FabricBackupRestoreAgentImpl()
{
    WriteNoise(
        "FabricBackupRestoreAgentImpl",
        TraceId,
        "FabricBackupRestoreAgentImpl::Destructed");
}
