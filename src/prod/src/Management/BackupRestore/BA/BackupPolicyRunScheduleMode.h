// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupPolicyRunScheduleMode
        {
            enum Enum
            {
                Invalid = 0,
                Daily = 1,
                Weekly = 2
            };

            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result);
            Common::ErrorCode FromPublic(::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE publicEnum, BackupPolicyRunScheduleMode::Enum & internalEnum);
            ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE ToPublic(BackupPolicyRunScheduleMode::Enum internalEnum);
            std::wstring EnumToString(BackupPolicyRunScheduleMode::Enum value);
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        }
    }
}
