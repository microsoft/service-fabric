// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupSchedule::BackupSchedule() :
    type_(BackupPolicyRunScheduleMode::Enum::Invalid),
    runDays_(0)
{
}

BackupSchedule::~BackupSchedule()
{
}

Common::ErrorCode BackupSchedule::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_SCHEDULE_BASED_BACKUP_POLICY & fabricBackupSchedule) const
{
    auto scheduleList = heap.AddItem<FABRIC_BACKUP_SCHEDULE_RUNTIME_LIST>();
    
    auto itemCount = runTimes_.size();
    scheduleList->Count = static_cast<ULONG>(itemCount);

    if (itemCount > 0)
    {
        auto items = heap.AddArray<DWORD>(itemCount);
        for (size_t i = 0; i < items.GetCount(); ++i)
        {
            items[i] = runTimes_[i];
        }

        scheduleList->RunTimes = items.GetRawArray();
    }
    else
    {
        scheduleList->RunTimes = NULL;
    }

    fabricBackupSchedule.RunDays = runDays_;
    fabricBackupSchedule.RunScheduleType = BackupPolicyRunScheduleMode::ToPublic(type_);
    fabricBackupSchedule.RunTimesList = scheduleList.GetRawPointer();
    fabricBackupSchedule.Reserved = NULL;

    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupSchedule::FromPublicApi(
    FABRIC_SCHEDULE_BASED_BACKUP_POLICY const & fabricBackupSchedule)
{
    ErrorCode error(ErrorCodeValue::Success);
    runDays_ = fabricBackupSchedule.RunDays;
    error = BackupPolicyRunScheduleMode::FromPublic(fabricBackupSchedule.RunScheduleType, type_);
    if (!error.IsSuccess()) { return error; }

    const FABRIC_BACKUP_SCHEDULE_RUNTIME_LIST * runtimeList = (const FABRIC_BACKUP_SCHEDULE_RUNTIME_LIST *)fabricBackupSchedule.RunTimesList;
    if (runtimeList != NULL)
    {
        for (uint64 i = 0; i < runtimeList->Count; i++)
        {
            runTimes_.push_back(*(runtimeList->RunTimes + i));
        }
    }

    return error;
}
