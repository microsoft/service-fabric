// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupPolicyRunFrequencyMode
        {
            enum Enum
            {
                Invalid = 0,
                Minutes = 1,
                Hours = 2
            };

            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result);
            Common::ErrorCode FromPublic(::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE publicEnum, BackupPolicyRunFrequencyMode::Enum & internalEnum);
            ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE ToPublic(BackupPolicyRunFrequencyMode::Enum internalEnum);
            std::wstring EnumToString(BackupPolicyRunFrequencyMode::Enum value);
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        }
    }
}
