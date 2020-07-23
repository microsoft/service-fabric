// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupStoreFileShareAccessType
        {
            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result)
            {
                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"DomainUser"))
                {
                    result = Enum::DomainUser;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"None"))
                {
                    result = Enum::None;
                    return Common::ErrorCode::Success();
                }

                return Common::ErrorCodeValue::InvalidArgument;
            }

            Common::ErrorCode FromPublic(::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE publicEnum, BackupStoreFileShareAccessType::Enum & internalEnum)
            {
                switch (publicEnum)
                {
                case ::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_NONE:
                    internalEnum = Enum::None;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_DOMAIN_USER:
                    internalEnum = Enum::DomainUser;
                    return Common::ErrorCode::Success();

                default:
                    return Common::ErrorCodeValue::InvalidArgument;
                }
            }

            ::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE ToPublic(BackupStoreFileShareAccessType::Enum internalEnum)
            {
                switch (internalEnum)
                {
                case Enum::None:
                    return ::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_NONE;

                case Enum::DomainUser:
                    return ::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE_DOMAIN_USER;

                default:
                    Common::Assert::CodingError("Cannot convert BackupStoreFileShareAccessType {0}", internalEnum);
                }
            }

            std::wstring EnumToString(BackupStoreFileShareAccessType::Enum value)
            {
                switch (value)
                {
                case Enum::None:
                    return L"None";

                case Enum::DomainUser:
                    return L"DomainUser";

                default:
                    return L"UnknownValue";
                }
            }

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Enum::None: w << L":"; return;

                case Enum::DomainUser: w << L"DomainUser"; return;

                default:
                    w << "BackupStoreFileShareAccessType(" << static_cast<int>(e) << ')';
                }
            }
        }
    }
}
