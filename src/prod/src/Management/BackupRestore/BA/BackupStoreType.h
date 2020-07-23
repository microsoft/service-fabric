// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupStoreType
        {
            enum Enum
            {
                Invalid = 0,
                FileShare = 1,
                AzureStore = 2,
                DsmsAzureStore = 3
            };

            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result);
            Common::ErrorCode FromPublic(::FABRIC_BACKUP_STORE_TYPE publicEnum, BackupStoreType::Enum & internalEnum);
            ::FABRIC_BACKUP_STORE_TYPE ToPublic(BackupStoreType::Enum internalEnum);
            std::wstring EnumToString(BackupStoreType::Enum value);
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        }
    }
}
