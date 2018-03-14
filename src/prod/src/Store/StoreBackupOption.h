// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace StoreBackupOption
    {
        /// <summary>
        /// Backup options for the store.
        /// </summary>		
        enum Enum : ULONG
        {
            Full = 1,
            Incremental = 2,
            TruncateLogsOnly = 3,			
            FirstValidEnum = Full,
            LastValidEnum = TruncateLogsOnly
        };

        Common::ErrorCode ToPublicApi(
            __in StoreBackupOption::Enum storeBackupOption,
            __out FABRIC_STORE_BACKUP_OPTION & backupOption);

        Common::ErrorCode FromPublicApi(
            __in FABRIC_STORE_BACKUP_OPTION backupOption,
            __out StoreBackupOption::Enum & storeBackupOption);

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(StoreBackupOption)
    };
};
