// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace StoreBackupOption
    {
        Common::ErrorCode ToPublicApi(
            __in StoreBackupOption::Enum storeBackupOption,
            __out FABRIC_STORE_BACKUP_OPTION & backupOption)
        {
            switch (storeBackupOption)
            {
            case StoreBackupOption::Enum::Full:
                backupOption = FABRIC_STORE_BACKUP_OPTION_FULL;
                return Common::ErrorCodeValue::Success;

            case StoreBackupOption::Enum::Incremental:
                backupOption = FABRIC_STORE_BACKUP_OPTION_INCREMENTAL;
                return Common::ErrorCodeValue::Success;

            case StoreBackupOption::Enum::TruncateLogsOnly:
                backupOption = FABRIC_STORE_BACKUP_OPTION_TRUNCATE_LOGS_ONLY;
                return Common::ErrorCodeValue::Success;

            default:
                storeBackupOption = StoreBackupOption::Enum::Full;
                return Common::ErrorCodeValue::InvalidArgument;
            }
        }

        Common::ErrorCode FromPublicApi(
            __in FABRIC_STORE_BACKUP_OPTION backupOption,
            __out StoreBackupOption::Enum & storeBackupOption)
        {
            switch (backupOption)
            {
            case FABRIC_STORE_BACKUP_OPTION_FULL:
                storeBackupOption = StoreBackupOption::Full;
                return Common::ErrorCodeValue::Success;

            case FABRIC_STORE_BACKUP_OPTION_INCREMENTAL:
                storeBackupOption = StoreBackupOption::Incremental;
                return Common::ErrorCodeValue::Success;

            case FABRIC_STORE_BACKUP_OPTION_TRUNCATE_LOGS_ONLY:
                storeBackupOption = StoreBackupOption::TruncateLogsOnly;
                return Common::ErrorCodeValue::Success;

            default:
                storeBackupOption = StoreBackupOption::Full;
                return Common::ErrorCodeValue::InvalidArgument;
            }
        }

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e)
        {			
            // not very foolproof, but sufficient since developers usually update the enum 
            // correctly but may forget to update this method.
            static_assert(
                (Full == FirstValidEnum) && (TruncateLogsOnly == LastValidEnum) && (LastValidEnum - FirstValidEnum == 2), 
                "Please update this method if you add/modify StoreBackupOption::Enum");

            switch (e)
            {
            case Full: w << "Full"; return;
            case Incremental: w << "Incremental"; return;
            case TruncateLogsOnly: w << "TruncateLogsOnly"; return;

            default: 
                // we should never hit this line. Not prefering to do an Assert::CodingError
                // since this is just an enum-to-string dumping method				
                w << "StoreBackupOption(" << static_cast<ULONG>(e) << ')';
            }			
        }

        ENUM_STRUCTURED_TRACE(StoreBackupOption, FirstValidEnum, LastValidEnum)
    }
}
