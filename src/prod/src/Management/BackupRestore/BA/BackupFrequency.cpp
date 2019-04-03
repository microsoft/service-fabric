// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupFrequency::BackupFrequency() :
    type_(BackupPolicyRunFrequencyMode::Enum::Invalid)
{
}

BackupFrequency::~BackupFrequency()
{
}

Common::ErrorCode BackupFrequency::ToPublicApi(
    __in Common::ScopedHeap &,
    __out FABRIC_FREQUENCY_BASED_BACKUP_POLICY & fabricBackupFrequency) const
{
    fabricBackupFrequency.RunFrequencyType = BackupPolicyRunFrequencyMode::ToPublic(type_);
    fabricBackupFrequency.Value = value_;
    fabricBackupFrequency.Reserved = NULL;
    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupFrequency::FromPublicApi(
    FABRIC_FREQUENCY_BASED_BACKUP_POLICY const & fabricBackupFrequency)
{
    ErrorCode error(ErrorCodeValue::Success);

    error = BackupPolicyRunFrequencyMode::FromPublic(fabricBackupFrequency.RunFrequencyType, type_);
    if (!error.IsSuccess()) { return error; }

    value_ = fabricBackupFrequency.Value;
    return error;
}
