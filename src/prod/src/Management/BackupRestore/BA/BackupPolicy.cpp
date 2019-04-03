// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupPolicy::BackupPolicy() :
    name_(L""),
    policyType_(BackupPolicyType::Enum::Invalid),
    maxIncrementalBackups_(0),
    backupStoreInfo_(),
    backupFrequency_(),
    backupSchedule_()
{
}

BackupPolicy::~BackupPolicy()
{
}

Common::ErrorCode BackupPolicy::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_BACKUP_POLICY & fabricBackupPolicy) const
{
    ErrorCode error(ErrorCodeValue::Success);

    fabricBackupPolicy.MaxIncrementalBackups = (BYTE)maxIncrementalBackups_;
    fabricBackupPolicy.Name = heap.AddString(name_);
    fabricBackupPolicy.PolicyId = policyId_;
    fabricBackupPolicy.PolicyType = BackupPolicyType::ToPublic(policyType_);

    switch (policyType_)
    {
        case BackupPolicyType::Enum::FrequencyBased:
        {
            auto policyDesc = heap.AddItem<FABRIC_FREQUENCY_BASED_BACKUP_POLICY>();
            error = backupFrequency_.ToPublicApi(heap, *policyDesc);
            if (!error.IsSuccess()) { return error; }
            fabricBackupPolicy.PolicyDescription = policyDesc.GetRawPointer();
        }
        break;

        case BackupPolicyType::Enum::ScheduleBased:
        {
            auto policyDesc1 = heap.AddItem<FABRIC_SCHEDULE_BASED_BACKUP_POLICY>();
            error = backupSchedule_.ToPublicApi(heap, *policyDesc1);
            if (!error.IsSuccess()) { return error; }
            fabricBackupPolicy.PolicyDescription = policyDesc1.GetRawPointer();
        }
        break;

        default:
        fabricBackupPolicy.PolicyDescription = NULL;
        break;
    }

    
    auto storeInfo = heap.AddItem<FABRIC_BACKUP_STORE_INFORMATION>();
    error = backupStoreInfo_.ToPublicApi(heap, *storeInfo);
    if (!error.IsSuccess()) { return error; }
    fabricBackupPolicy.StoreInformation = storeInfo.GetRawPointer();

    fabricBackupPolicy.Reserved = NULL;
    return error;
}

Common::ErrorCode BackupPolicy::FromPublicApi(
    FABRIC_BACKUP_POLICY const & fabricBackupPolicy)
{
    ErrorCode error(ErrorCodeValue::Success);

    name_ = fabricBackupPolicy.Name;
    policyId_ = fabricBackupPolicy.PolicyId;
    maxIncrementalBackups_ = fabricBackupPolicy.MaxIncrementalBackups;

    if (fabricBackupPolicy.StoreInformation != NULL)
    {
        error = backupStoreInfo_.FromPublicApi(*fabricBackupPolicy.StoreInformation);
        if (!error.IsSuccess()) { return error; }
    }

    error = BackupPolicyType::FromPublic(fabricBackupPolicy.PolicyType, policyType_);
    if (!error.IsSuccess()) { return error; }

    switch (policyType_)
    {
        case BackupPolicyType::FrequencyBased:
        {
            FABRIC_FREQUENCY_BASED_BACKUP_POLICY * freqBasedPolicy = (FABRIC_FREQUENCY_BASED_BACKUP_POLICY *)fabricBackupPolicy.PolicyDescription;
            if (freqBasedPolicy != NULL)
            {
                error = backupFrequency_.FromPublicApi(*freqBasedPolicy);
            }
        }
        break;

        case BackupPolicyType::ScheduleBased:
        {
            FABRIC_SCHEDULE_BASED_BACKUP_POLICY * scheduleBasedPolicy = (FABRIC_SCHEDULE_BASED_BACKUP_POLICY *)fabricBackupPolicy.PolicyDescription;
            if (scheduleBasedPolicy != NULL)
            {
                error = backupSchedule_.FromPublicApi(*scheduleBasedPolicy);
            }
        }
        break;
    }

    return error;
}
