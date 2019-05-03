// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupPolicyType
        {
            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result)
            {
                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"FrequencyBased"))
                {
                    result = Enum::FrequencyBased;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"ScheduleBased"))
                {
                    result = Enum::ScheduleBased;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Invalid"))
                {
                    result = Enum::Invalid;
                    return Common::ErrorCode::Success();
                }

                return Common::ErrorCodeValue::InvalidArgument;
            }

            Common::ErrorCode FromPublic(::FABRIC_BACKUP_POLICY_TYPE publicEnum, BackupPolicyType::Enum & internalEnum)
            {
                switch (publicEnum)
                {
                case ::FABRIC_BACKUP_POLICY_TYPE_INVALID:
                    internalEnum = Enum::Invalid;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_POLICY_TYPE_FREQUENCY_BASED:
                    internalEnum = Enum::FrequencyBased;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_POLICY_TYPE_SCHEDULE_BASED:
                    internalEnum = Enum::ScheduleBased;
                    return Common::ErrorCode::Success();

                default:
                    return Common::ErrorCodeValue::InvalidArgument;
                }
            }

            ::FABRIC_BACKUP_POLICY_TYPE ToPublic(BackupPolicyType::Enum internalEnum)
            {
                switch (internalEnum)
                {
                case Enum::Invalid:
                    return ::FABRIC_BACKUP_POLICY_TYPE_INVALID;

                case Enum::FrequencyBased:
                    return ::FABRIC_BACKUP_POLICY_TYPE_FREQUENCY_BASED;

                case Enum::ScheduleBased:
                    return ::FABRIC_BACKUP_POLICY_TYPE_SCHEDULE_BASED;
                default:
                    Common::Assert::CodingError("Cannot convert BackupPolicyType {0}", internalEnum);
                }
            }

            std::wstring EnumToString(BackupPolicyType::Enum value)
            {
                switch (value)
                {
                case Enum::Invalid:
                    return L"Invalid";

                case Enum::FrequencyBased:
                    return L"FrequencyBased";

                case Enum::ScheduleBased:
                    return L"ScheduleBased";

                default:
                    return L"UnknownValue";
                }
            }

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Enum::Invalid: w << L"Invalid"; return;

                case Enum::FrequencyBased: w << L"FrequencyBased"; return;

                case Enum::ScheduleBased: w << L"ScheduleBased"; return;

                default:
                    w << "BackupPolicyType(" << static_cast<int>(e) << ')';
                }
            }
        }
    }
}
