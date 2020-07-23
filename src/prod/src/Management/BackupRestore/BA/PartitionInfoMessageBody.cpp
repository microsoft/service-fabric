// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

PartitionInfoMessageBody::PartitionInfoMessageBody()
{
}

PartitionInfoMessageBody::PartitionInfoMessageBody(
    wstring serviceName,
    Common::Guid partitionId) :
    serviceName_(serviceName),
    partitionId_(partitionId)
{
}

PartitionInfoMessageBody::~PartitionInfoMessageBody()
{
}
