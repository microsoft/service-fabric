// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupCopier
        {
            class UploadBackupJobQueue : public BackupCopierJobQueueBase
            {
            public:
                UploadBackupJobQueue(__in ComponentRoot & root);

            protected:

                ConfigEntryBase & GetThrottleConfigEntry() override;

                int GetThrottleConfigValue() override;
            };

        }
    }
}
