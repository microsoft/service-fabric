// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupStoreType
        {
            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result)
            {
                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"FileShare"))
                {
                    result = Enum::FileShare;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"AzureStore"))
                {
                    result = Enum::AzureStore;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"DsmsAzureStore"))
                {
                    result = Enum::DsmsAzureStore;
                    return Common::ErrorCode::Success();
                }

                if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"Invalid"))
                {
                    result = Enum::Invalid;
                    return Common::ErrorCode::Success();
                }

                return Common::ErrorCodeValue::InvalidArgument;
            }

            Common::ErrorCode FromPublic(::FABRIC_BACKUP_STORE_TYPE publicEnum, BackupStoreType::Enum & internalEnum)
            {
                switch (publicEnum)
                {
                case ::FABRIC_BACKUP_STORE_TYPE_INVALID:
                    internalEnum = Enum::Invalid;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_STORE_TYPE_FILE_SHARE:
                    internalEnum = Enum::FileShare;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_STORE_TYPE_AZURE_STORE:
                    internalEnum = Enum::AzureStore;
                    return Common::ErrorCode::Success();

                case ::FABRIC_BACKUP_STORE_TYPE_DSMS_AZURE_STORE:
                    internalEnum = Enum::DsmsAzureStore;
                    return Common::ErrorCode::Success();

                default:
                    return Common::ErrorCodeValue::InvalidArgument;
                }
            }

            ::FABRIC_BACKUP_STORE_TYPE ToPublic(BackupStoreType::Enum internalEnum)
            {
                switch (internalEnum)
                {
                case Enum::Invalid:
                    return ::FABRIC_BACKUP_STORE_TYPE_INVALID;

                case Enum::FileShare:
                    return ::FABRIC_BACKUP_STORE_TYPE_FILE_SHARE;

                case Enum::AzureStore:
                    return ::FABRIC_BACKUP_STORE_TYPE_AZURE_STORE;

                case Enum::DsmsAzureStore:
                    return ::FABRIC_BACKUP_STORE_TYPE_DSMS_AZURE_STORE;

                default:
                    Common::Assert::CodingError("Cannot convert BackupStoreType {0}", internalEnum);
                }
            }

            std::wstring EnumToString(BackupStoreType::Enum value)
            {
                switch (value)
                {
                case Enum::Invalid:
                    return L"Invalid";

                case Enum::FileShare:
                    return L"FileShare";

                case Enum::AzureStore:
                    return L"AzureStore";

                case Enum::DsmsAzureStore:
                    return L"DsmsAzureStore";

                default:
                    return L"UnknownValue";
                }
            }

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Enum::Invalid: w << L"Invalid"; return;

                case Enum::FileShare: w << L"FileShare"; return;

                case Enum::AzureStore: w << L"AzureStore"; return;

                case Enum::DsmsAzureStore: w << L"DsmsAzureStore"; return;

                default:
                    w << "BackupStoreType(" << static_cast<int>(e) << ')';
                }
            }
        }
    }
}
