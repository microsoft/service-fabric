// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupPolicyType
        {
            enum Enum
            {
                Invalid = 0,
                FrequencyBased = 1,
                ScheduleBased = 2
            };

            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result);
            Common::ErrorCode FromPublic(::FABRIC_BACKUP_POLICY_TYPE publicEnum, BackupPolicyType::Enum & internalEnum);
            ::FABRIC_BACKUP_POLICY_TYPE ToPublic(BackupPolicyType::Enum internalEnum);
            std::wstring EnumToString(BackupPolicyType::Enum value);
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        }
    }
}
