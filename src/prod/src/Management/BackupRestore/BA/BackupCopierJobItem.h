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
            class BackupCopierJobItem
            {
                DEFAULT_COPY_ASSIGNMENT(BackupCopierJobItem)

            public:
                BackupCopierJobItem();

                BackupCopierJobItem(BackupCopierJobItem && other);

                BackupCopierJobItem(shared_ptr<BackupCopierAsyncOperationBase> && operation);

                bool ProcessJob(ComponentRoot &);

            private:
                shared_ptr<BackupCopierAsyncOperationBase> operation_;
            };
        }
    }
}
