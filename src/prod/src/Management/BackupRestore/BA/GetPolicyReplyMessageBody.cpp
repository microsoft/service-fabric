// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

GetPolicyReplyMessageBody::GetPolicyReplyMessageBody()
{
}

GetPolicyReplyMessageBody::GetPolicyReplyMessageBody(BackupPolicy policy) : policy_(policy)
{
}

GetPolicyReplyMessageBody::~GetPolicyReplyMessageBody()
{
}
