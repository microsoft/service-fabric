// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreService
    {
        class BackupRestoreServiceAgentFactory
            : public Api::IBackupRestoreServiceAgentFactory
            , public Common::ComponentRoot
        {
        public:
            virtual ~BackupRestoreServiceAgentFactory();

            static Api::IBackupRestoreServiceAgentFactoryPtr Create();

            virtual Common::ErrorCode CreateBackupRestoreServiceAgent(__out Api::IBackupRestoreServiceAgentPtr &);

        private:
            BackupRestoreServiceAgentFactory();
        };
    }
}

