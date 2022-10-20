// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupPolicyRunScheduleMode
        {
            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result)
            {
                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Daily"))
                {
                    result = Enum::Daily;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Weekly"))
                {
                    result = Enum::Weekly;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Invalid"))
                {
                    result = Enum::Invalid;
                    return Common::ErrorCode::Success();
                }

                return Common::ErrorCodeValue::InvalidArgument;
            }

            Common::ErrorCode FromPublic(::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE publicEnum, BackupPolicyRunScheduleMode::Enum & internalEnum)
            {
                switch (publicEnum)
                {
                case ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_INVALID:
                    internalEnum = Enum::Invalid;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_DAILY:
                    internalEnum = Enum::Daily;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_WEEKLY:
                    internalEnum = Enum::Weekly;
                    return Common::ErrorCode::Success();

                default:
                    return Common::ErrorCodeValue::InvalidArgument;
                }
            }

            ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE ToPublic(BackupPolicyRunScheduleMode::Enum internalEnum)
            {
                switch (internalEnum)
                {
                case Enum::Invalid:
                    return ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_INVALID;

                case Enum::Daily:
                    return ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_DAILY;

                case Enum::Weekly:
                    return ::FABRIC_BACKUP_POLICY_RUN_SCHEDULE_MODE_WEEKLY;
                default:
                    Common::Assert::CodingError("Cannot convert BackupPolicyRunScheduleMode {0}", internalEnum);
                }
            }

            std::wstring EnumToString(BackupPolicyRunScheduleMode::Enum value)
            {
                switch (value)
                {
                case Enum::Invalid:
                    return L"Invalid";

                case Enum::Daily:
                    return L"Daily";

                case Enum::Weekly:
                    return L"Weekly";

                default:
                    return L"UnknownValue";
                }
            }

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Enum::Invalid: w << L"Invalid"; return;

                case Enum::Daily: w << L"Daily"; return;

                case Enum::Weekly: w << L"Weekly"; return;

                default:
                    w << "BackupPolicyRunScheduleMode(" << static_cast<int>(e) << ')';
                }
            }
        }
    }
}
