// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupPolicyRunFrequencyMode
        {
            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result)
            {
                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Minutes"))
                {
                    result = Enum::Minutes;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Hours"))
                {
                    result = Enum::Hours;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Invalid"))
                {
                    result = Enum::Invalid;
                    return Common::ErrorCode::Success();
                }

                return Common::ErrorCodeValue::InvalidArgument;
            }

            Common::ErrorCode FromPublic(::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE publicEnum, BackupPolicyRunFrequencyMode::Enum & internalEnum)
            {
                switch (publicEnum)
                {
                case ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_INVALID:
                    internalEnum = Enum::Invalid;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_MINUTES:
                    internalEnum = Enum::Minutes;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_HOURS:
                    internalEnum = Enum::Hours;
                    return Common::ErrorCode::Success();

                default:
                    return Common::ErrorCodeValue::InvalidArgument;
                }
            }

            ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE ToPublic(BackupPolicyRunFrequencyMode::Enum internalEnum)
            {
                switch (internalEnum)
                {
                case Enum::Invalid:
                    return ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_INVALID;

                case Enum::Minutes:
                    return ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_MINUTES;

                case Enum::Hours:
                    return ::FABRIC_BACKUP_POLICY_RUN_FREQUENCY_MODE_HOURS;
                default:
                    Common::Assert::CodingError("Cannot convert BackupPolicyRunFrequencyMode {0}", internalEnum);
                }
            }

            std::wstring EnumToString(BackupPolicyRunFrequencyMode::Enum value)
            {
                switch (value)
                {
                case Enum::Invalid:
                    return L"Invalid";

                case Enum::Minutes:
                    return L"Minutes";

                case Enum::Hours:
                    return L"Hours";

                default:
                    return L"UnknownValue";
                }
            }

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Enum::Invalid: w << L"Invalid"; return;

                case Enum::Minutes: w << L"Minutes"; return;

                case Enum::Hours: w << L"Hours"; return;

                default:
                    w << "BackupPolicyRunFrequencyMode(" << static_cast<int>(e) << ')';
                }
            }
        }
    }
}
