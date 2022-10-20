// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricGetBackupSchedulePolicyResult :
        public Common::ComponentRoot
    {
        DENY_COPY(FabricGetBackupSchedulePolicyResult)

    public:
        FabricGetBackupSchedulePolicyResult();
        FabricGetBackupSchedulePolicyResult(__in Management::BackupRestoreAgentComponent::BackupPolicy &policy);
        virtual ~FabricGetBackupSchedulePolicyResult();

        // 
        // IFabricGetBackupSchedulePolicyResult methods
        // 
        const FABRIC_BACKUP_POLICY *STDMETHODCALLTYPE get_BackupSchedulePolicy(void);

    private:
        Management::BackupRestoreAgentComponent::BackupPolicy backupPolicy_;
        Common::ScopedHeap heap_;
    };
}
