// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricGetRestorePointDetailsResult :
        public Common::ComponentRoot
    {
        DENY_COPY(FabricGetRestorePointDetailsResult)

    public:
        FabricGetRestorePointDetailsResult();
        FabricGetRestorePointDetailsResult(__in Management::BackupRestoreAgentComponent::RestorePointDetails &restorePointDetails);
        virtual ~FabricGetRestorePointDetailsResult();

        // 
        // IFabricGetRestorePointDetailsResult methods
        // 
        const FABRIC_RESTORE_POINT_DETAILS *STDMETHODCALLTYPE get_RestorePointDetails(void);

    private:
        Management::BackupRestoreAgentComponent::RestorePointDetails restorePointDetails_;
        Common::ScopedHeap heap_;
    };
}
