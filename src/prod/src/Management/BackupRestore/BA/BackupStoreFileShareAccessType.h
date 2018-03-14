// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupStoreFileShareAccessType
        {
            enum Enum
            {
                None = 0,
                DomainUser = 1
            };

            Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result);
            Common::ErrorCode FromPublic(::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE publicEnum, BackupStoreFileShareAccessType::Enum & internalEnum);
            ::FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE ToPublic(BackupStoreFileShareAccessType::Enum internalEnum);
            std::wstring EnumToString(BackupStoreFileShareAccessType::Enum value);
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
        }
    }
}
