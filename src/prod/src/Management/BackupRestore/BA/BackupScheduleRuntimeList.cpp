// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupScheduleRuntimeList::BackupScheduleRuntimeList()
{
}

BackupScheduleRuntimeList::BackupScheduleRuntimeList(vector<int64> runTimes) : runTimes_(runTimes) // TODO: Check if effective copy possible.
{
}

BackupScheduleRuntimeList::~BackupScheduleRuntimeList()
{
}
